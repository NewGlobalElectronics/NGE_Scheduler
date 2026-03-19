# NGE Scheduler – Porting Guide

This guide describes the exact steps required to bring NGE_Scheduler up on a
new MCU or operating environment.  The `core/` directory is **never modified**.

---

## What the scheduler requires from the platform

| Requirement | Where to implement |
|---|---|
| A periodic interrupt that fires every N time units | `bsp/bsp.c` |
| That ISR writes `uSchTic = 1U;` | `bsp/bsp.c` – ISR body |
| `SCH_ENTER_CRITICAL()` disables that interrupt | `bsp/bsp.h` – `#define` |
| `SCH_EXIT_CRITICAL()` re-enables that interrupt | `bsp/bsp.h` – `#define` |
| C99 compiler with `<stdint.h>`, `<stdbool.h>`, `<string.h>` | Toolchain |

Everything else (LED, UART, specific timer peripheral) is only needed to run
the demo tasks in `core/app_tasks.c`.

---

## Step-by-step procedure

### 1 – Create the port directory

```
ports/
└── <vendor>_<device>/
    ├── bsp/
    │   ├── bsp.h
    │   └── bsp.c
    └── main.c
```

### 2 – Write `bsp/bsp.h`

```c
#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include "your_device_header.h"   /* MCU-specific – only file in the BSP */

/*
 * Define the critical-section hooks BEFORE including NGE_Scheduler.h.
 * Replace the bodies with your platform's interrupt-disable/enable mechanism.
 */
#define SCH_ENTER_CRITICAL()   your_disable_irq()
#define SCH_EXIT_CRITICAL()    your_enable_irq()

#include "NGE_Scheduler.h"      /* must come AFTER the hook definitions */

void BSP_Init(void);
void BSP_ToggleLED(void);
void BSP_PrintLine(const char *pStr);

#endif /* BSP_H */
```

**Rules:**
- The critical-section macros must guard the two-statement sequence
  `uDelayTic += uSchTic; uSchTic = 0U` against a concurrent ISR write to
  `uSchTic`.
- On single-core, non-preemptive platforms it is safe to leave them empty
  (accept at most one-tick jitter – see `docs/architecture.md`).
- Only MCU-specific headers belong in `bsp.h`.  Standard C99 headers are
  already included by `NGE_Scheduler.h`.

### 3 – Write `bsp/bsp.c`

Implement three functions:

#### 3a – Tick ISR

Configure any hardware timer to fire periodically and write `uSchTic = 1U`
inside the ISR.  The period defines one "tick"; all `TIME_*` constants in
`NGE_Scheduler.h` are in these tick units.

```c
void YourTimer_IRQHandler(void)
{
    ClearTimerFlag();          /* peripheral-specific */
    uSchTic = 1U;              /* signal the scheduler */
}
```

#### 3b – `BSP_ToggleLED(void)`

Toggle any available output (GPIO, stdout, SWO).  Used by `HeartbeatTask`.

#### 3c – `BSP_PrintLine(const char *pStr)`

Print a null-terminated string followed by a newline.  Used by all demo tasks.
Acceptable implementations: UART polling, SWO ITM, `puts()`, debugger console.

### 4 – Write `main.c`

```c
#include "NGE_Scheduler.h"
#include "app_tasks.h"
#include "bsp/bsp.h"

int main(void)
{
    BSP_Init();                  /* configure hardware, start tick timer  */

    (void)initHeartbeatTask();
    (void)initCommTask();
    (void)initMonitorTask();

    SchEventManager(aTaskArray); /* never returns */
    return 0;
}
```

### 5 – Build system

Add to your build system:

```
Source files:
  core/NGE_Scheduler.c
  core/app_tasks.c
  ports/<target>/bsp/bsp.c
  ports/<target>/main.c

Include paths:
  core/
  ports/<target>/bsp/
```

No special preprocessor defines are required for static mode.
For dynamic allocation add `-DDYNAMIC_ALLOCATION` to the compiler flags and
ensure the platform provides a working `malloc` / `free`.

---

## Critical-section reference table

| Architecture | Disable IRQ | Enable IRQ |
|---|---|---|
| ARM Cortex-M (CMSIS) | `__disable_irq()` | `__enable_irq()` |
| ARM Cortex-R (CMSIS) | `__disable_irq()` | `__enable_irq()` |
| TriCore (TASKING) | `__disable()` | `__enable()` |
| TriCore (GCC) | `__asm__("disable")` | `__asm__("enable")` |
| RH850 (CC-RH) | `__DI()` | `__EI()` |
| RL78 (CC-RL) | `__DI()` | `__EI()` |
| RL78 (IAR) | `__disable_interrupt()` | `__enable_interrupt()` |
| MIPS32 (XC32) | `__builtin_disable_interrupts()` | `__builtin_enable_interrupts()` |
| POSIX | `sigprocmask(SIG_BLOCK, …)` | `sigprocmask(SIG_UNBLOCK, …)` |
| Win32 | `EnterCriticalSection(…)` | `LeaveCriticalSection(…)` |

For platforms with a save-and-restore pattern (e.g., RP2040) the macro must
declare and use a local variable:

```c
#define SCH_ENTER_CRITICAL()  uint32_t _s = save_and_disable_interrupts()
#define SCH_EXIT_CRITICAL()   restore_interrupts(_s)
```

The local variable `_s` is visible across both macro expansions because they
are always used consecutively in the same block scope within
`SchEventManager()`.

---

## Checklist before declaring a port complete

- [ ] `uSchTic` is declared `volatile` in the build (it is in `NGE_Scheduler.c`).
- [ ] The ISR only writes `uSchTic = 1U`; it does not call any scheduler function.
- [ ] `SCH_ENTER_CRITICAL` / `SCH_EXIT_CRITICAL` protect the read-modify-write
      of `uSchTic` in `SchEventManager()`.
- [ ] The tick ISR priority is **lower** than any ISR that calls
      `AddEventToEventArray()` from interrupt context (if applicable).
- [ ] `BSP_PrintLine` is **not** called from interrupt context.
- [ ] All three demo tasks produce observable output on the target.
