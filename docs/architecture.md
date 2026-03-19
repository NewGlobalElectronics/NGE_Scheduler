# NGE Scheduler – Architecture

## Overview

NGE_Scheduler is a **cooperative, event-driven scheduler**.  Tasks are never
preempted; each handler runs to completion before the next event is dispatched.
This makes shared-state concurrency trivial and formal analysis tractable for
MISRA / AUTOSAR / ISO 26262 contexts.

---

## Internal call graph

```
main()
 └─ SchEventManager(aTaskArray)          [infinite loop]
      └─ DispatchTask(pTask, uDelayTic)  [per task]
           └─ ProcessEvent(pEvt, Fnct, uDelayTic)  [per live event]
                └─ pTask->Fnct(pEvt)    [application handler]
```

---

## Event container

### Static mode (default)

Each `tTask` owns a **flat ring-buffer** of `uEventArrayLength` slots.
Every slot is exactly `uDataTypeLength` bytes (the sizeof of the concrete
event type, which must be ≥ sizeof(tEvent)).

```
pEventArray ──► [ slot 0 ][ slot 1 ][ slot 2 ] … [ slot N-1 ]
                   ▲                                    ▲
                   │                                    │
             uTaskWriteIndex                    (wraps around)
```

A slot is free when `uMsg == EVT_EMPTY (0)`.
`_GET_pEVT(task, i)` accesses slot `i` using byte-pointer arithmetic with
`size_t`-typed stride, making the access endian-neutral and width-portable.

### Dynamic mode (`#define DYNAMIC_ALLOCATION`)

Each `tTask` owns the **head and tail pointers** of a singly-linked queue.
Every node is a heap-allocated block of `uDataTypeLength` bytes with an
intrusive `pNext` pointer prepended (grouped with `pMsg` to avoid padding).

```
pEventHead ──► [node A] ──pNext──► [node B] ──pNext──► NULL
                                                         ▲
                                                   pEventTail
```

Tail-pointer caching guarantees O(1) append.
Deletion via `DeleteEVENTFromEVENTArray()` stamps a tombstone
(`uMsg = EVT_EMPTY`); `DispatchTask()` unlinks and frees tombstones
during its next traversal.

---

## Event dispatch rules

For every live slot / node (`uMsg != EVT_EMPTY`):

```
lTO > 0  ──►  decrement lTO by 1 tick (if a tick has elapsed)
              do NOT call the handler yet

lTO == 0 ──►  call handler: result = pTask->Fnct(pEvt)
              ├─ lTimer > 0          ──►  reload: lTO = lTimer   (periodic)
              ├─ lTimer == 0
              │   ├─ result != IN_PROGRESS ──►  free slot / unlink node
              │   └─ result == IN_PROGRESS ──►  re-dispatch next iteration
              └─ (lTimer > 0 always reloads, ignoring result)
```

---

## Tick accumulation

`uSchTic` is a `volatile u08` written to `1` by the platform ISR.

Inside `SchEventManager` the accumulator `uDelayTic` absorbs ticks:

```c
if (uDelayTic > 0U) { uDelayTic--; }     /* consume one stored tick  */
SCH_ENTER_CRITICAL();
uDelayTic = (u08)(uDelayTic + uSchTic);  /* absorb new ticks         */
uSchTic   = 0U;                          /* clear ISR flag           */
SCH_EXIT_CRITICAL();
```

This design ensures that **no tick is silently lost** even if the dispatch
loop takes longer than one tick period, at the cost of at most one-tick
jitter in countdown decrement (documented race window when critical-section
hooks are left empty).

---

## Task status machine

```
        ADD_EVENT()
SUSPENDED ──────────────────── (rejected)

SUSPENDED ──RESUME_TASK()──► WAIT
PAUSED    ──RESUME_TASK()──► WAIT

WAIT ─── event ready ──────► IN_PROGRESS (handler returns IN_PROGRESS)
WAIT ─── event ready ──────► WAIT        (handler returns WAIT, one-shot done)

IN_PROGRESS ── next pass ──► IN_PROGRESS (handler returns IN_PROGRESS again)
IN_PROGRESS ── next pass ──► WAIT        (handler returns WAIT)
```

Note: the scheduler does **not** set `tskStatus` itself.  `WAIT` and
`IN_PROGRESS` are the values returned by the handler; the scheduler uses them
only to decide whether to free the event slot.  `PAUSED` and `SUSPENDED` are
set by the application; the scheduler checks them before dispatching.

---

## Struct layout and endian neutrality

All multi-byte values are stored in named struct members and accessed only
through those member names.  No byte-level casting of multi-byte fields is
performed.  The compiler handles native-endian layout transparently, making
the scheduler binary-compatible with both little-endian and big-endian targets
without any `#ifdef` guards.

Field ordering in `tEvent` and `tTask` follows the **largest-alignment-first**
rule to eliminate internal padding holes on 8, 16, 32, and 64-bit architectures.

---

## MISRA-C:2012 deviation summary

| ID | Rule | Category | One-line justification |
|---|---|---|---|
| D_01 | 20.10 | Advisory | `##` required for unique symbol generation in task macros |
| D_02 | 15.5  | Advisory | Early return on SUSPENDED check; single-exit adds complexity |
| D_03 | 14.2  | Required | Intentional infinite loop `for(;;)` in scheduler main loop |
| D_04 | 11.5  | Advisory | `tEvent*` → `void*` required by generic handler signature |
| D_05 | 21.3  | Required | `malloc`/`free` in DYNAMIC_ALLOCATION mode; bounded by event rate |
| D_06 | 11.3  | Required | `char*` → `tEvent*` in `_GET_pEVT`; only portable variable-stride accessor |
| D_07 | 18.4  | Advisory | Pointer arithmetic in `_GET_pEVT` and `DELETE_TASK` |
| D_08 | 11.3  | Required | `EvtType*` → `tEvent*` in `SCH_TSK_CREATE2`; sizeof enforced by uDataTypeLength |

Full justifications and compensating measures are in
[`misra_deviations.md`](misra_deviations.md).
