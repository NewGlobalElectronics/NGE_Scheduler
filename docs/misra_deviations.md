# NGE Scheduler – MISRA-C:2012 Deviation Log

Document reference : NGE-SCH-MISRA-001  
Revision           : 1.0  
Applicable files   : `core/NGE_Scheduler.h`, `core/NGE_Scheduler.c`

---

## Deviation D_01

| Field | Value |
|---|---|
| **ID** | D_01 |
| **Rule** | 20.10 – The `#` and `##` preprocessor operators shall not be used |
| **Category** | Advisory |
| **Location** | `NGE_Scheduler.h` – `TOKENPASTE_` / `TOKENPASTE` macros; all uses of `TOKENPASTE` in `SCH_TSK_CREATE2` |
| **Justification** | The macro-based task-creation API requires token concatenation to generate unique symbol names (`EvtArray<TaskName>`, `init<TaskName>`, `<TaskName>FCT`) at the call site without requiring the application to declare separate boilerplate. No safe alternative exists within the C preprocessor. |
| **Compensating measure** | The two-level expansion idiom (`TOKENPASTE_` + `TOKENPASTE`) ensures all macro arguments are fully expanded before concatenation, preventing accidental token mismatches. The macro is guarded by `#ifndef` to avoid redefinition if the project provides its own. |

---

## Deviation D_02

| Field | Value |
|---|---|
| **ID** | D_02 |
| **Rule** | 15.5 – A function should have a single point of exit at the end |
| **Category** | Advisory |
| **Location** | `NGE_Scheduler.c` – `AddEventToEventArray()`, first `if` block |
| **Justification** | The early `return NULL` guards against adding events to a `SUSPENDED` task. Refactoring to a single-exit form requires a flag variable and an additional level of nesting across the entire function body, reducing readability with no safety benefit. The early return is a simple, reviewable guard clause. |
| **Compensating measure** | The deviation is limited to one site. The return value is `NULL`, which callers must check (function return value is not `void`). The `SUSPENDED` check is the first statement in the function, making the exit point immediately visible to reviewers. |

---

## Deviation D_03

| Field | Value |
|---|---|
| **ID** | D_03 |
| **Rule** | 14.2 – A for statement shall be well-formed |
| **Category** | Required |
| **Location** | `NGE_Scheduler.c` – `SchEventManager()`, outer `for(;;)` |
| **Justification** | The scheduler main loop is intentionally non-terminating. A `for(;;)` with an empty condition is the standard C idiom for an infinite loop. The loop never terminates by design; writing `while(true)` would be equivalent but would require `<stdbool.h>` in the `.c` file solely for this construct. The `for(;;)` form is recognised by all MISRA-aware static analysers as an intentional infinite loop. |
| **Compensating measure** | The loop is annotated with an inline comment `/* Intentional infinite loop – D_03 */`. The surrounding documentation in `SchEventManager` explicitly states the function must never return. The inner task-iteration loop uses `while` (not `for`) to satisfy Rule 14.2 for all non-infinite loops. |

---

## Deviation D_04

| Field | Value |
|---|---|
| **ID** | D_04 |
| **Rule** | 11.5 – A conversion should not be performed from pointer to void into pointer to object |
| **Category** | Advisory |
| **Location** | `NGE_Scheduler.h` – `SCH_TSK_CREATE2`, generated handler opening; `NGE_Scheduler.c` – `ProcessEvent()` call to `Fnct((void *)pEvt)` |
| **Justification** | The `tActionFnct` type is defined as `eTskStatus (*)(void *)` to provide a single, type-erased handler signature for all task types. At the call site `pEvt` (a `tEvent *`) is passed as `void *`. Inside the generated handler `hEvent` (a `void *`) is cast back to the concrete `EvtType *`. The round-trip is safe because `AddEventToEventArray()` always stores exactly `uDataTypeLength` bytes, which equals `sizeof(EvtType)`. |
| **Compensating measure** | `uDataTypeLength` is set to `sizeof(EvtType)` in `SCH_TSK_CREATE2` and is used as the `memcpy` size in `AddEventToEventArray()`, guaranteeing the allocation and copy match the concrete type. The cast site is annotated with `/* D_04 */`. |

---

## Deviation D_05

| Field | Value |
|---|---|
| **ID** | D_05 |
| **Rule** | 21.3 – The memory allocation and deallocation functions of `<stdlib.h>` shall not be used |
| **Category** | Required |
| **Location** | `NGE_Scheduler.c` – `AddEventToEventArray()` (`malloc`) and `DispatchTask()` (`free`), compiled only when `DYNAMIC_ALLOCATION` is defined |
| **Justification** | Dynamic allocation is an opt-in build mode disabled by default. It is intended for host platforms (Linux, Windows) and resource-rich MCUs where a deterministic allocator is available. Safety-critical targets must use static mode (no `#define DYNAMIC_ALLOCATION`). |
| **Compensating measure** | (1) `<stdlib.h>` is included only inside `#ifdef DYNAMIC_ALLOCATION`. (2) Every `malloc` return is checked for `NULL` before use. (3) Every allocated node is freed exactly once, either by `DispatchTask()` after dispatch or by tombstone collection. (4) Projects enabling this mode must raise a project-level deviation record and provide a verified allocator. |

---

## Deviation D_06

| Field | Value |
|---|---|
| **ID** | D_06 |
| **Rule** | 11.3 – A cast shall not be performed between a pointer to object type and a pointer to a different object type |
| **Category** | Required |
| **Location** | `NGE_Scheduler.h` – `_GET_pEVT` macro: cast from `char *` to `tEvent *` |
| **Justification** | Variable-stride ring-buffer access requires computing a byte offset from the base pointer. `char *` is the only pointer type the C standard (C99 §6.3.2.3) explicitly permits to alias any object type without undefined behaviour. Casting from `char *` to `tEvent *` after an exact multiple-of-alignment offset is defined behaviour given that the ring-buffer slots are allocated as an array of `EvtType` (which has alignment ≥ that of `tEvent`). |
| **Compensating measure** | Both the index and stride are cast to `size_t` before multiplication (Rule 10.4). The base pointer is the `pEventArray` field typed as `tEvent *`, so the arithmetic always starts at a correctly aligned address. The macro is annotated with a `cppcheck-suppress` comment and is excluded from DYNAMIC_ALLOCATION builds where it is not needed. |

---

## Deviation D_07

| Field | Value |
|---|---|
| **ID** | D_07 |
| **Rule** | 18.4 – The `+`, `-`, `+=` and `-=` operators should not be applied to an expression of pointer type |
| **Category** | Advisory |
| **Location** | `NGE_Scheduler.h` – `_GET_pEVT` macro (`+ offset`); `DELETE_TASK` macro (`pTaskToDelete + 1U`) |
| **Justification** | Both uses are arithmetic on a pointer to the first element of an array, stepping to a well-defined subsequent element. This is the canonical C idiom for array traversal and the only way to implement variable-stride element access without a loop. |
| **Compensating measure** | `_GET_pEVT` bounds the index with `uEventArrayLength` in all call sites (checked by the caller loop). `DELETE_TASK` steps exactly one `tTask` element forward, which is within the `aTaskArray` bounds because the task count is decremented immediately after. Both sites are annotated with `cppcheck-suppress` comments. |

---

## Deviation D_08

| Field | Value |
|---|---|
| **ID** | D_08 |
| **Rule** | 11.3 – A cast shall not be performed between a pointer to object type and a pointer to a different object type |
| **Category** | Required |
| **Location** | `NGE_Scheduler.h` – static-mode `SCH_TSK_CREATE2`: cast from `EvtType *` (array of concrete type) to `tEvent *` (stored in `pEventArray`) |
| **Justification** | The `pEventArray` field stores a pointer to a buffer whose elements are the concrete event type `EvtType`. Because `EvtType` is always declared as a struct whose **first member** is `tEvent base` (enforced by the application convention documented in `app_tasks.h`), the base address of any `EvtType` element is also a valid `tEvent *`. The cast is safe under the common-initial-sequence rule (C99 §6.5.2.3 ¶5). |
| **Compensating measure** | `uDataTypeLength` is set to `sizeof(EvtType)` so all `memcpy` and stride operations use the full concrete size, never the base size. The application documentation requires `tEvent base` to be the first member of any custom event type. The cast site is annotated with a `cppcheck-suppress` comment. |

---

*End of deviation log.*
