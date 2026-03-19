/*******************************************************************************
 * NGE_Scheduler.c
 * New Global Electronics – Cooperative Scheduler
 *
 * Portability: standard C99 headers only.  No MCU SDK headers appear here.
 * Platform integration is limited to two BSP responsibilities:
 *   1. Write  uSchTic = 1U  inside the periodic tick interrupt handler.
 *   2. Optionally define SCH_ENTER_CRITICAL / SCH_EXIT_CRITICAL in the BSP
 *      header before including NGE_Scheduler.h (see header for details).
 *
 * Internal call graph
 * ───────────────────
 *  SchEventManager()        public  – iterates tasks, manages tick accumulator
 *    └─ DispatchTask()      static  – iterates the event container of one task
 *         └─ ProcessEvent() static  – handles a single live event slot / node
 *
 * See NGE_Scheduler.h for the full event-behaviour model and deviation record.
 ******************************************************************************/

/* Standard C99 headers only – no MCU-specific dependency */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#ifdef DYNAMIC_ALLOCATION
#include <stdlib.h>   /* malloc, free – deviation D_05 */
#endif
#include "NGE_Scheduler.h"

/*******************************************************************************
 * Module-scope variables
 *
 * uSchTic: volatile – written by the platform ISR, read and cleared by
 * SchEventManager().  Without volatile a compiler with optimisation enabled
 * is free to cache the value in a register, causing the scheduler loop to
 * never observe the ISR write.
 ******************************************************************************/
tTask          aTaskArray[MAX_SCH_TASKS];
tTask         *tskTaskArray     = &aTaskArray[0];
u16            uiIndexTaskArray = 0U;
u16           *pIndexTaskArray  = &uiIndexTaskArray;
volatile u08   uSchTic          = 0U;

/*******************************************************************************
 * Forward declarations of file-scope helpers  (Rule 8.2, Rule 8.7)
 ******************************************************************************/
static bool ProcessEvent(tEvent *pEvt, tActionFnct Fnct, u08 uDelayTic);
static void DispatchTask(tTask *pTask, u08 uDelayTic);

/*******************************************************************************
 * InitEVENTArray
 *
 * Static  – marks every ring-buffer slot free (uMsg = EVT_EMPTY).
 * Dynamic – sets head and tail to NULL (empty queue).
 ******************************************************************************/
void InitEVENTArray(tTask *tTaskToInitEvtArray)
{
#ifdef DYNAMIC_ALLOCATION
    tTaskToInitEvtArray->pEventHead = NULL;
    tTaskToInitEvtArray->pEventTail = NULL;
#else
    u16 i;
    for (i = 0U; i < tTaskToInitEvtArray->uEventArrayLength; i++)
    {
        _GET_uMsg(tTaskToInitEvtArray, i) = (u16)EVT_EMPTY;
    }
#endif
}

/*******************************************************************************
 * AddEventToEventArray
 *
 * Static  – advances write index past occupied slots, copies event into the
 *           next free slot.
 * Dynamic – allocates a node of uDataTypeLength bytes (D_05), copies the
 *           event, appends to the tail in O(1) via pEventTail.
 *
 * Returns the written event pointer, or NULL when:
 *   the task is SUSPENDED (D_02), or
 *   [static]  the ring-buffer is full, or
 *   [dynamic] malloc() returns NULL.
 *
 * uTaskNbPendingEvents is saturated at UINT8_MAX (Rule 4.1); counts only
 * one-shot events (lTimer == 0).
 *
 * evtEventToAdd is const: source data is read, never modified (Rule 8.13).
 ******************************************************************************/
tEvent *AddEventToEventArray(tTask *tTaskToAddEvt, const tEvent *evtEventToAdd)
{
    tEvent *pWrittenEvent = NULL;

    if (tTaskToAddEvt->tskStatus == SUSPENDED)   /* Rule 10.3 – same enum */
    {
        return NULL;  /* D_02 */
    }

#ifdef DYNAMIC_ALLOCATION
    {
        tEvent *pNew = (tEvent *)malloc(           /* D_05, D_08 */
                           (size_t)tTaskToAddEvt->uDataTypeLength);
        if (pNew != NULL)
        {
            (void)memcpy(pNew, evtEventToAdd,
                         (size_t)tTaskToAddEvt->uDataTypeLength);
            pNew->pNext = NULL;

            if (tTaskToAddEvt->pEventTail == NULL)
            {
                tTaskToAddEvt->pEventHead = pNew;   /* First node: also head */
            }
            else
            {
                tTaskToAddEvt->pEventTail->pNext = pNew;
            }
            tTaskToAddEvt->pEventTail = pNew;

            /* Saturating increment (Rule 4.1) for one-shot events only. */
            if ((pNew->lTimer == (u32)0U) &&
                (tTaskToAddEvt->uTaskNbPendingEvents < (u08)UINT8_MAX))
            {
                tTaskToAddEvt->uTaskNbPendingEvents++;
            }

            pWrittenEvent = pNew;
        }
    }

#else  /* static mode */
    {
        u16 slotsChecked = 0U;

        while (
            (_GET_uMsg(tTaskToAddEvt,
                       tTaskToAddEvt->uTaskWriteIndex) != (u16)EVT_EMPTY) &&
            (slotsChecked < tTaskToAddEvt->uEventArrayLength))
        {
            tTaskToAddEvt->uTaskWriteIndex =
                (u08)((tTaskToAddEvt->uTaskWriteIndex + 1U) %
                      tTaskToAddEvt->uEventArrayLength);
            slotsChecked++;
        }

        if (slotsChecked < tTaskToAddEvt->uEventArrayLength)
        {
            pWrittenEvent =
                _GET_pEVT(tTaskToAddEvt, tTaskToAddEvt->uTaskWriteIndex);
            (void)memcpy(pWrittenEvent, evtEventToAdd,
                         (size_t)tTaskToAddEvt->uDataTypeLength);

            if ((pWrittenEvent->lTimer == (u32)0U) &&
                (tTaskToAddEvt->uTaskNbPendingEvents < (u08)UINT8_MAX))
            {
                tTaskToAddEvt->uTaskNbPendingEvents++;
            }
        }
    }
#endif /* DYNAMIC_ALLOCATION */

    return pWrittenEvent;
}

/*******************************************************************************
 * DeleteEVENTFromEVENTArray
 *
 * Static  – clears uMsg to EVT_EMPTY, freeing the slot immediately.
 * Dynamic – stamps a tombstone (uMsg = EVT_EMPTY); DispatchTask() detects
 *           and frees the node on its next queue traversal.
 ******************************************************************************/
void DeleteEVENTFromEVENTArray(tEvent *eEVENTToDelete)
{
    eEVENTToDelete->uMsg = (u16)EVT_EMPTY;
}

/*******************************************************************************
 * ProcessEvent  [static – Rule 8.7]
 *
 * Handles one live event slot / node.  Caller guarantees pEvt->uMsg != 0.
 *
 * Returns true  – slot / node must be freed (one-shot done).
 * Returns false – event is still counting down, periodic, or IN_PROGRESS.
 *
 * All countdown arithmetic uses u32 operands on both sides (Rule 10.4):
 * no implicit widening to int / long regardless of architecture.
 ******************************************************************************/
static bool ProcessEvent(tEvent *pEvt, tActionFnct Fnct, u08 uDelayTic)
{
    bool bRemove = false;

    if (pEvt->lTO > (u32)0U)
    {
        /* Counting down: decrement only when a tick has elapsed. */
        if (uDelayTic > 0U)
        {
            pEvt->lTO = pEvt->lTO - (u32)1U;  /* Rule 10.4 – u32 op u32 */
        }
    }
    else
    {
        /* Countdown at zero: dispatch (D_04 – tEvent* passed as void*). */
        eTskStatus result = Fnct((void *)pEvt); /* D_04 */

        if (pEvt->lTimer > (u32)0U)
        {
            pEvt->lTO = pEvt->lTimer;           /* Periodic: reload counter */
        }
        else if (result != IN_PROGRESS)
        {
            bRemove = true;                     /* One-shot, done: remove   */
        }
        else
        {
            /* IN_PROGRESS, no reload: re-dispatch on the next pass. */
        }
    }

    return bRemove;
}

/*******************************************************************************
 * DispatchTask  [static – Rule 8.7]
 *
 * Dynamic mode  – walks the singly-linked queue with a while loop.
 *   Tombstones (uMsg == EVT_EMPTY) and completed one-shot nodes are unlinked
 *   and freed inline.  pEventTail is kept consistent so that
 *   AddEventToEventArray() can always perform O(1) tail-append.
 *
 * Static mode   – walks the flat ring-buffer with a for loop.
 *   Completed one-shot event slots are cleared to EVT_EMPTY.
 ******************************************************************************/
static void DispatchTask(tTask *pTask, u08 uDelayTic)
{
#ifdef DYNAMIC_ALLOCATION

    tEvent *pPrev = NULL;
    tEvent *pEvt  = pTask->pEventHead;

    while (pEvt != NULL)
    {
        tEvent *pNext   = pEvt->pNext; /* saved before any free             */
        bool    bRemove;

        if (pEvt->uMsg == (u16)EVT_EMPTY)
        {
            bRemove = true;            /* Tombstone: schedule for removal   */
        }
        else
        {
            bRemove = ProcessEvent(pEvt, pTask->Fnct, uDelayTic);
        }

        if (bRemove)
        {
            /* Unlink the node. */
            if (pPrev != NULL)
            {
                pPrev->pNext = pNext;
            }
            else
            {
                pTask->pEventHead = pNext;   /* Removed the head node       */
            }

            if (pNext == NULL)
            {
                pTask->pEventTail = pPrev;   /* Removed the tail node       */
            }

            free(pEvt);  /* D_05 */
            /* pPrev intentionally not advanced: successor is already pNext */
        }
        else
        {
            pPrev = pEvt;
        }

        pEvt = pNext;
    }

#else  /* static mode */

    u16     evtIdx;
    tEvent *pEvt;

    for (evtIdx = 0U; evtIdx < pTask->uEventArrayLength; evtIdx++)
    {
        pEvt = _GET_pEVT(pTask, evtIdx);     /* D_06 */

        if (pEvt->uMsg != (u16)EVT_EMPTY)
        {
            if (ProcessEvent(pEvt, pTask->Fnct, uDelayTic))
            {
                pEvt->uMsg = (u16)EVT_EMPTY; /* Free the ring-buffer slot   */
            }
        }
    }

#endif /* DYNAMIC_ALLOCATION */
}

/*******************************************************************************
 * SchEventManager  [public]
 *
 * Cooperative scheduler main loop.  Must never return (D_03).
 *
 * Tick accumulation
 *   uDelayTic accumulates ticks elapsed between loop iterations, preventing
 *   tick loss when the dispatch loop takes longer than one tick period.
 *   The tick read-clear pair is wrapped in SCH_ENTER/EXIT_CRITICAL hooks.
 *   With the default empty hooks one tick may be lost per loop iteration
 *   (see the race note in NGE_Scheduler.h).
 *
 * Loop structure
 *   Outer:  for(;;)           intentional infinite loop (D_03).
 *   Inner:  while             used instead of for because the condition tests
 *                             both taskIdx and uSchTic; Rule 14.2 constrains
 *                             only for-loop conditions.
 *
 * Variable scoping (Rule 8.9)
 *   taskIdx and pTask are declared at the narrowest scope that covers their
 *   uses (inside the for(;;) body).
 *
 * Arithmetic (Rule 10.4)
 *   Both operands of  uDelayTic + uSchTic  are u08; the result is cast back
 *   to u08 (Rule 10.3).  uSchTic is at most 1, so no overflow is possible
 *   after the preceding decrement.
 ******************************************************************************/
void SchEventManager(tTask *SchTaskArray)
{
    u08 uDelayTic = 0U;

    for (;;)  /* Intentional infinite loop – D_03 */
    {
        u16 taskIdx = 0U;

        while ((taskIdx < *pIndexTaskArray) && (uSchTic == 0U))
        {
            tTask *pTask = &SchTaskArray[taskIdx];

            if (pTask->Fnct != NULL)
            {
                DispatchTask(pTask, uDelayTic);
            }
            taskIdx++;
        }

        /*
         * Tick book-keeping (guarded by BSP-provided critical section).
         *
         * Step 1  Consume one accumulated tick from the previous iteration.
         * Step 2  Atomically read uSchTic, add to accumulator, clear flag.
         */
        if (uDelayTic > 0U)
        {
            uDelayTic--;                                      /* Step 1 */
        }

        SCH_ENTER_CRITICAL();                                 /* Step 2 */
        uDelayTic = (u08)(uDelayTic + uSchTic);               /* Rule 10.3 */
        uSchTic   = 0U;
        SCH_EXIT_CRITICAL();
    }
}
