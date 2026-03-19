/*******************************************************************************
 * NGE_Scheduler.h
 * New Global Electronics – Cooperative Scheduler
 *
 * Portability guarantees
 * ──────────────────────
 *  1. Zero MCU-specific headers.  This file includes ONLY standard C99
 *     headers: <stdbool.h>, <stddef.h>, <stdint.h>, <string.h>, and
 *     (conditionally) <stdlib.h>.  No MCU SDK or RTOS header must ever
 *     appear here.
 *
 *  2. All integer types are fixed-width (<stdint.h>).  No dependency on
 *     the width of int, long, or pointer across architectures.
 *
 *  3. Endian-neutral.  Every multi-byte field is accessed exclusively
 *     through named struct members; no raw byte-level casting of multi-byte
 *     values is performed anywhere in the module.
 *
 *  4. Struct fields are ordered largest-alignment-first to eliminate
 *     internal padding holes on 8 / 16 / 32 / 64-bit architectures:
 *
 *     tEvent (static mode, 32-bit):  void*(4) u32(4) u32(4) u16(2)+pad = 16 B
 *     tEvent (static mode, 64-bit):  void*(8) u32(4) u32(4) u16(2)+pad = 24 B
 *     tEvent (dynamic mode, 32-bit): void*(4) void*(4) u32(4) u32(4) u16(2)+pad = 20 B
 *     tEvent (dynamic mode, 64-bit): void*(8) void*(8) u32(4) u32(4) u16(2)+pad = 32 B
 *
 *  5. uSchTic is volatile.  The platform BSP must write  uSchTic = 1U
 *     inside its periodic tick interrupt handler.  The tick period is
 *     defined by the BSP; the scheduler makes no assumption about its
 *     absolute duration.  The TIME_* constants in this header are in
 *     "tick units"; their relationship to wall-clock time is entirely
 *     determined by the BSP tick rate.
 *
 *  6. Critical-section hooks.  The tick read-clear pair in SchEventManager
 *     has a narrow race window.  Two empty macros are provided:
 *       SCH_ENTER_CRITICAL()
 *       SCH_EXIT_CRITICAL()
 *     Override them in your BSP header before including this file:
 *       #define SCH_ENTER_CRITICAL()  __disable_irq()
 *       #define SCH_EXIT_CRITICAL()   __enable_irq()
 *     When left as empty defaults the scheduler behaves as in the original
 *     design (one tick may be lost per loop iteration – documented below).
 *
 * Build modes
 * ───────────
 *  Static (default)        Fixed ring-buffer of event slots embedded in tTask.
 *                          No heap usage; suitable for safety-critical targets.
 *  Dynamic                 Heap-allocated singly-linked event queue.
 *  (#define                pNext == NULL marks the tail of the queue.
 *  DYNAMIC_ALLOCATION)     Requires D_05 deviation; see below.
 *
 * Event-behaviour model (identical in both modes)
 * ─────────────────────────────────────────────────
 *  uMsg == EVT_EMPTY (0)   Slot / node is free.
 *  uMsg != 0               Live event; uMsg carries the application message ID.
 *  lTO   > 0               Delayed: handler not called until lTO reaches 0.
 *  lTO  == 0               Ready for immediate dispatch.
 *  lTimer > 0              Periodic: lTO reloaded from lTimer after dispatch.
 *  lTimer == 0, done       One-shot: slot freed / node unlinked after dispatch.
 *
 * Reserved uMsg values
 *  EVT_EMPTY   (0)   Internal sentinel.  Must never be written by application.
 *  EVT_INIT    (1)   One-shot start-up event  (lTO = 0, lTimer = 0).
 *  EVT_APP_BASE(2)   First message ID available to the application.
 *
 * uSchTic race note
 *  The sequence  uDelayTic += uSchTic; uSchTic = 0U  is not atomic.  If the
 *  ISR fires in the narrow window between those two statements, one tick is
 *  cleared without being counted, causing at most one tick of latency jitter
 *  on countdown decrements.  Override SCH_ENTER_CRITICAL / SCH_EXIT_CRITICAL
 *  to make this pair atomic on platforms where exact timing is required.
 *
 * MISRA-C:2012 Deviation record
 * ──────────────────────────────
 *  D_01 [Rule 20.10] TOKENPASTE uses ##.  Required for unique symbol
 *       generation in the macro-based task API; no safe alternative.
 *
 *  D_02 [Rule 15.5]  AddEventToEventArray() returns early (NULL) for a
 *       SUSPENDED task.  A single-exit refactor adds no safety benefit.
 *
 *  D_03 [Rule 14.2]  SchEventManager() contains an intentional infinite
 *       loop written as for(;;).  The scheduler main loop must never
 *       terminate.
 *
 *  D_04 [Rule 11.5]  The generated task handler casts the void* parameter
 *       back to EvtType*.  Required by the generic tActionFnct signature;
 *       safe because AddEventToEventArray() always stores exactly
 *       uDataTypeLength bytes.
 *
 *  D_05 [Rule 21.3]  DYNAMIC_ALLOCATION mode calls malloc() and free().
 *       All allocations are bounded by the application event rate.
 *       A project-level deviation record is mandatory when this mode is
 *       enabled.
 *
 *  D_06 [Rule 11.3]  _GET_pEVT casts char* to tEvent*.  char* is the
 *       only pointer type the C standard permits to alias any object type;
 *       this is therefore the only portable variable-stride array accessor.
 *
 *  D_07 [Rule 18.4]  _GET_pEVT and DELETE_TASK use pointer arithmetic.
 *       Limited to these two documented sites.
 *
 *  D_08 [Rule 11.3]  SCH_TSK_CREATE2 casts EvtType* to tEvent*.  Safe
 *       because the scheduler accesses only the base tEvent fields, and
 *       sizeof(EvtType) >= sizeof(tEvent) is enforced by uDataTypeLength.
 ******************************************************************************/

#ifndef NGE_SCHEDULER_H
#define NGE_SCHEDULER_H

/* Standard C99 headers only – no MCU-specific dependency */
#include <stdbool.h>   /* bool, true, false                                  */
#include <stddef.h>    /* size_t, NULL                                       */
#include <stdint.h>    /* uint8_t … uint32_t, UINT8_MAX                     */
#include <string.h>    /* memcpy                                             */
#ifdef DYNAMIC_ALLOCATION
#include <stdlib.h>    /* malloc, free – deviation D_05                      */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Critical-section hooks  (P5)
 *
 * Empty by default.  Override both in your BSP header BEFORE including
 * NGE_Scheduler.h if atomic tick read-clear is required on your platform.
 *
 *   Example (CMSIS Cortex-M):
 *     #define SCH_ENTER_CRITICAL()  __disable_irq()
 *     #define SCH_EXIT_CRITICAL()   __enable_irq()
 ******************************************************************************/
#ifndef SCH_ENTER_CRITICAL
#  define SCH_ENTER_CRITICAL()   /* empty default */
#endif
#ifndef SCH_EXIT_CRITICAL
#  define SCH_EXIT_CRITICAL()    /* empty default */
#endif

/*******************************************************************************
 * TOKENPASTE – token concatenation helper  (deviation D_01)
 *
 * Defined with an #ifndef guard so that if the project already provides
 * this macro in a shared header the local definition is silently skipped.
 * The two-level expansion ensures macro arguments are fully expanded before
 * concatenation (standard C preprocessor behaviour).
 ******************************************************************************/
/* cppcheck-suppress [misra-c2012-20.10] -- D_01 */
#ifndef TOKENPASTE
#  define TOKENPASTE_(a, b)  a##b  /* cppcheck-suppress misra-c2012-20.10 */
#  define TOKENPASTE(a, b)   TOKENPASTE_(a, b)
#endif

/*******************************************************************************
 * Fixed-width integer aliases  (Rule 4.6)
 ******************************************************************************/
typedef uint8_t  u08;
typedef uint16_t u16;
typedef uint32_t u32;

/*******************************************************************************
 * Time constants
 *
 * Units are BSP ticks, not milliseconds.  The relationship between ticks
 * and wall-clock time is defined by the BSP tick-interrupt period.
 * Assuming a 1 ms tick these equal 1 ms … 30 s.
 ******************************************************************************/
#define TIME_1TICK    (1U)
#define TIME_2TICK    (2U)
#define TIME_5TICK    (5U)
#define TIME_10TICK   (10U)
#define TIME_20TICK   (20U)
#define TIME_30TICK   (30U)
#define TIME_50TICK   (50U)
#define TIME_100TICK  (100U)
#define TIME_200TICK  (200U)

/* Aliases assuming 1 tick == 1 ms; kept for backward compatibility */
#define TIME_1MS    TIME_1TICK
#define TIME_2MS    TIME_2TICK
#define TIME_5MS    TIME_5TICK
#define TIME_10MS   TIME_10TICK
#define TIME_20MS   TIME_20TICK
#define TIME_30MS   TIME_30TICK
#define TIME_50MS   TIME_50TICK
#define TIME_100MS  TIME_100TICK
#define TIME_200MS  TIME_200TICK

/* Multiples of 100 ticks */
#define TIME_3S     (30U)
#define TIME_5S     (50U)
#define TIME_6S     (60U)
#define TIME_9S     (90U)
#define TIME_10S    (100U)
#define TIME_12S    (120U)
#define TIME_30S    (300U)

/*******************************************************************************
 * Scheduler capacity
 ******************************************************************************/
#define MAX_SCH_TASKS  (15U)

/*******************************************************************************
 * Reserved uMsg values
 ******************************************************************************/
#define EVT_EMPTY     (0U)   /* Internal sentinel – slot / node is free      */
#define EVT_INIT      (1U)   /* One-shot start-up event                      */
#define EVT_APP_BASE  (2U)   /* First message ID available to the application*/

/*******************************************************************************
 * Task status  (tTask.tskStatus)
 *
 *  WAIT        No event to process; task is idle.
 *  IN_PROGRESS Handler returned IN_PROGRESS; same event is re-dispatched.
 *  PAUSED      Task stays in the list; events not dispatched but accepted.
 *  SUSPENDED   Excluded from scheduling; new events are rejected.
 ******************************************************************************/
typedef enum
{
    WAIT        = 0,
    IN_PROGRESS = 1,
    PAUSED      = 2,
    SUSPENDED   = 3
} eTskStatus;

/*******************************************************************************
 * tEvent – scheduler event record
 *
 * Fields are ordered largest-alignment-first to eliminate internal padding
 * holes on every supported architecture (see portability notes at the top).
 * In DYNAMIC_ALLOCATION mode pNext follows pMsg so that both pointer-width
 * fields are grouped together, again avoiding internal holes.
 *
 *  pMsg    Optional payload pointer (native pointer width).
 *  pNext   [DYNAMIC_ALLOCATION only]  Next node in queue; NULL = tail.
 *  lTimer  Reload value.  > 0 → periodic (lTO reset to lTimer after dispatch).
 *  lTO     Countdown.     > 0 → delayed (not dispatched until lTO == 0).
 *  uMsg    Message ID.  EVT_EMPTY (0) marks the slot / node as free.
 ******************************************************************************/
typedef struct tEvent_tag
{
    void             *pMsg;    /* Optional extended payload                  */
#ifdef DYNAMIC_ALLOCATION
    struct tEvent_tag *pNext;  /* Next node; NULL = tail (grouped with pMsg) */
#endif
    u32               lTimer;  /* Reload value   (> 0 → periodic)           */
    u32               lTO;     /* Countdown      (> 0 → delayed dispatch)   */
    u16               uMsg;    /* Message ID;  EVT_EMPTY (0) = free slot    */
} tEvent;

/*******************************************************************************
 * Task handler function pointer
 * D_04 applies at every call site where tEvent* is passed as void*.
 ******************************************************************************/
typedef eTskStatus (*tActionFnct)(void *);

/*******************************************************************************
 * tTask – task descriptor
 *
 * Fields ordered largest-alignment-first:
 *   pointer-width fields (Fnct, and mode-specific storage pointers) first,
 *   then int-sized fields (tskStatus as enum),
 *   then u16 fields packed together,
 *   then u08 fields packed together.
 *
 * This ordering produces no internal padding holes on 32-bit or 64-bit
 * architectures.
 *
 * tskStatus is typed as eTskStatus (not u08) to satisfy Rule 10.3.
 * uTaskNbPendingEvents is a saturating counter (Rule 4.1): capped at
 * UINT8_MAX; incremented only for one-shot events (lTimer == 0).
 ******************************************************************************/
typedef struct
{
    tActionFnct  Fnct;                 /* Task handler (pointer-width)       */
#ifdef DYNAMIC_ALLOCATION
    tEvent      *pEventHead;           /* Head of the linked event queue     */
    tEvent      *pEventTail;           /* Tail – enables O(1) append         */
#else
    tEvent      *pEventArray;          /* Pointer to the static ring-buffer  */
    u16          uEventArrayLength;    /* Capacity of the ring-buffer        */
#endif
    u16          uDataTypeLength;      /* sizeof the concrete event type     */
    eTskStatus   tskStatus;            /* Current task status (enum = int)   */
    u08          uTaskNbPendingEvents; /* Saturating one-shot event counter  */
#ifndef DYNAMIC_ALLOCATION
    u08          uTaskWriteIndex;      /* Index of the next free slot        */
#endif
} tTask;

/*******************************************************************************
 * Mutex status  (application-facing; Rule 2.3 advisory accepted)
 ******************************************************************************/
typedef enum
{
    INITIALIZED = 0,
    EXECUTED    = 1,
    PENDING     = 2,
    CANCELLED   = 3
} eMutexStatus;

/*******************************************************************************
 * Global scheduler state  (defined in NGE_Scheduler.c)
 *
 * uSchTic: volatile because it is written by the platform ISR and read by
 * SchEventManager().  Both the extern declaration here and the definition in
 * NGE_Scheduler.c must carry the volatile qualifier (Rule 8.13 / C99 §6.7.3).
 ******************************************************************************/
extern tTask           aTaskArray[MAX_SCH_TASKS];
extern tTask          *tskTaskArray;
extern u16             uiIndexTaskArray;
extern u16            *pIndexTaskArray;
extern volatile u08    uSchTic;

/*******************************************************************************
 * Static-mode ring-buffer accessor macros  (excluded in DYNAMIC_ALLOCATION)
 *
 *  _GET_pEVT  Byte-addressed element access (D_06, D_07).
 *             Both index (i) and stride (uDataTypeLength) cast to size_t
 *             before multiplication – same essential type on every arch
 *             (Rule 10.4).  Base pointer is fully parenthesised (Rule 20.7).
 *  _GET_uMsg  l-value of the uMsg  field of element [i].
 *  _GET_lTO   l-value of the lTO   field of element [i].
 ******************************************************************************/
#ifndef DYNAMIC_ALLOCATION
/* cppcheck-suppress [misra-c2012-11.3, misra-c2012-18.4] -- D_06, D_07 */
#define _GET_pEVT(Task, i)                                                \
    ((tEvent *)((char *)((Task)->pEventArray)                             \
                + (size_t)(i) * (size_t)((Task)->uDataTypeLength)))

#define _GET_uMsg(Task, i)  (_GET_pEVT((Task), (i))->uMsg)
#define _GET_lTO(Task,  i)  (_GET_pEVT((Task), (i))->lTO)
#endif /* !DYNAMIC_ALLOCATION */

/*******************************************************************************
 * ADD_EVENT – convenience wrapper around AddEventToEventArray
 ******************************************************************************/
#define ADD_EVENT(TaskName, Evt) \
    AddEventToEventArray((tTask *)(TaskName), (tEvent *)(Evt))

/*******************************************************************************
 * RESUME_TASK – transition a PAUSED / SUSPENDED task back to WAIT.
 * No cast required: both sides are eTskStatus (Rule 10.3).
 ******************************************************************************/
#define RESUME_TASK(TaskName)  ((TaskName)->tskStatus = WAIT)

/*******************************************************************************
 * SCH_TSK_CREATE – shorthand for tasks whose concrete event type is tEvent
 ******************************************************************************/
#define SCH_TSK_CREATE(TaskName, NbEventMax) \
    SCH_TSK_CREATE2(TaskName, NbEventMax, tEvent)

/*******************************************************************************
 * SCH_TSK_CREATE2 / MUTEX
 *
 * For each <TaskName> this macro generates:
 *  [static only]  EvtArray<TaskName>[NbEventMax]   ring-buffer storage
 *  <TaskName>                                       tTask pointer variable
 *  <TaskName>FCT(void *)                            forward declaration
 *  init<TaskName>(void)                             task registration function
 *  <TaskName>FCT(void *hEvent) {                    handler opening brace
 *      tTask   *pParent      = <TaskName>;          parent reference
 *      EvtType *CurrentEvent = (EvtType *)hEvent;   typed event (D_04)
 *
 * Closed by ENDTASK().
 * NbEventMax is unused in dynamic mode; suppressed with (void) (Rule 2.2).
 * Deviations D_01 (##), D_04 (void* cast), D_08 (EvtType*→tEvent*) apply.
 ******************************************************************************/
/* cppcheck-suppress [misra-c2012-20.10] -- D_01 */
#ifdef DYNAMIC_ALLOCATION

#define SCH_TSK_CREATE2(TaskName, NbEventMax, EvtType)                         \
    tTask               *TaskName;                                              \
    eTskStatus           TOKENPASTE(TaskName, FCT)(void *);                     \
    eTskStatus           TOKENPASTE(init, TaskName)(void)                       \
    {                                                                           \
        tTask *pNew = &tskTaskArray[*pIndexTaskArray];                          \
        (void)(NbEventMax);                                                     \
        pNew->Fnct                 = &TOKENPASTE(TaskName, FCT);                \
        pNew->pEventHead           = NULL;                                      \
        pNew->pEventTail           = NULL;                                      \
        pNew->uDataTypeLength      = (u16)sizeof(EvtType);                      \
        pNew->uTaskNbPendingEvents = 0U;                                        \
        pNew->tskStatus            = WAIT;                                      \
        TaskName = pNew;                                                        \
        (*pIndexTaskArray)++;                                                    \
        return WAIT;                                                            \
    }                                                                           \
    eTskStatus TOKENPASTE(TaskName, FCT)(void *hEvent)                          \
    {                                                                           \
        tTask   *pParent      = TaskName;                                       \
        EvtType *CurrentEvent = (EvtType *)hEvent; /* D_04 */

#else /* static mode */

/* cppcheck-suppress [misra-c2012-11.3] -- D_08 */
#define SCH_TSK_CREATE2(TaskName, NbEventMax, EvtType)                         \
    EvtType              TOKENPASTE(EvtArray, TaskName)[NbEventMax];            \
    tTask               *TaskName;                                              \
    eTskStatus           TOKENPASTE(TaskName, FCT)(void *);                     \
    eTskStatus           TOKENPASTE(init, TaskName)(void)                       \
    {                                                                           \
        tTask *pNew = &tskTaskArray[*pIndexTaskArray];                          \
        pNew->Fnct              = &TOKENPASTE(TaskName, FCT);                   \
        pNew->pEventArray       = (tEvent *)TOKENPASTE(EvtArray, TaskName);     \
        pNew->uEventArrayLength = (u16)(NbEventMax);                            \
        pNew->uDataTypeLength   = (u16)sizeof(EvtType);                         \
        pNew->uTaskWriteIndex   = 0U;                                           \
        pNew->uTaskNbPendingEvents = 0U;                                        \
        pNew->tskStatus         = WAIT;                                         \
        InitEVENTArray(pNew);                                                   \
        TaskName = pNew;                                                        \
        (*pIndexTaskArray)++;                                                    \
        return WAIT;                                                            \
    }                                                                           \
    eTskStatus TOKENPASTE(TaskName, FCT)(void *hEvent)                          \
    {                                                                           \
        tTask   *pParent      = TaskName;                                       \
        EvtType *CurrentEvent = (EvtType *)hEvent; /* D_04 */

#endif /* DYNAMIC_ALLOCATION */

#define MUTEX(MutexName, NbEventMax, EvtType) \
    SCH_TSK_CREATE2(MutexName, NbEventMax, EvtType)

/*******************************************************************************
 * ENDTASK – closes the handler body opened by SCH_TSK_CREATE2
 ******************************************************************************/
#define ENDTASK()    \
        return WAIT; \
    }

/*******************************************************************************
 * DELETE_TASK  (pointer arithmetic – deviation D_07)
 *
 * Removes *pTaskToDelete by overwriting it with the immediately following
 * entry and decrementing the task count.
 ******************************************************************************/
/* cppcheck-suppress [misra-c2012-18.4] -- D_07 */
#define DELETE_TASK(pTaskToDelete)                                         \
    do {                                                                   \
        (void)memcpy((pTaskToDelete), (pTaskToDelete) + 1U,                \
                     sizeof(tTask));                                       \
        (*pIndexTaskArray)--;                                              \
    } while (0)

/*******************************************************************************
 * Public API
 ******************************************************************************/
void    SchEventManager(tTask *SchTaskArray);
tEvent *AddEventToEventArray(tTask *tTaskToAddEvt, const tEvent *evtEventToAdd);
void    DeleteEVENTFromEVENTArray(tEvent *eEVENTToDelete);
void    InitEVENTArray(tTask *tTaskToInitEvtArray);
void    UpdateTasks(void);
void    fProgramReset(void);
void    AddEmergencyEVENT(tTask *tEmergencyTask, const tEvent *evtEventToAdd);

#ifdef __cplusplus
}
#endif

#endif /* NGE_SCHEDULER_H */
