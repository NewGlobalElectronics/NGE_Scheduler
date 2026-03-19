# NGE\_Scheduler

> **"What if Events become the starting point?"**
>
> — The design question that drives every decision in this project.

**New Global Electronics – Event-Centered Cooperative Scheduler**

A lightweight, [MISRA-C:2012](#misra-c2012-compliance) compliant, fully
MCU-agnostic **event-centered** cooperative scheduler targeting automotive,
industrial, and safety-critical embedded systems.

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Standard: C99](https://img.shields.io/badge/Standard-C99-orange.svg)]()
[![MISRA-C:2012](https://img.shields.io/badge/MISRA--C-2012-green.svg)]()
[![Platforms](https://img.shields.io/badge/Platforms-10-lightgrey.svg)]()
[![Heap: zero](https://img.shields.io/badge/Heap-zero%20(static%20mode)-brightgreen.svg)]()
[![Event-Centered](https://img.shields.io/badge/Paradigm-Event--Centered-purple.svg)]()

---

## Table of contents

1. [What does "event-centered" mean?](#what-does-event-centered-mean)
2. [The three event types](#the-three-event-types)
3. [Multi-task event architecture](#multi-task-event-architecture)
4. [Internal call graph](#internal-call-graph)
5. [Feature summary](#feature-summary)
6. [Repository layout](#repository-layout)
7. [Quick start](#quick-start)
8. [Defining a task](#defining-a-task)
9. [Event model cheat-sheet](#event-model-cheat-sheet)
10. [Adding a new MCU port](#adding-a-new-mcu-port)
11. [MISRA-C:2012 compliance](#misra-c2012-compliance)
12. [Supported platforms](#supported-platforms)
13. [License](#license)
14. [Contributing](#contributing)

---

## What does "event-centered" mean?

Most cooperative schedulers are **task-centered**: tasks are the primary
citizens and events are secondary data attached to them. NGE\_Scheduler
inverts this relationship entirely.

```
Traditional (task-centered)              NGE Scheduler (event-centered)
─────────────────────────────            ──────────────────────────────
Task is defined first                    Event is defined first
Task owns an attached event list         Event carries its own timing policy
Scheduler iterates tasks                 Scheduler iterates events
Idle task = still scheduled every pass   Idle task = zero scheduler cost
Periodic re-arming is manual             Periodic events self-reload via lTimer
Event type stored in an enum field       Event type = values of lTO and lTimer
```

In NGE\_Scheduler the **Event is the primary work item**.
A task is simply the handler that processes it.
A task whose event queue is empty is **completely invisible** to the scheduler —
it costs zero iterations, zero branching, and zero CPU cycles.

### The four event fields

| Field    | Type     | Role |
|---|---|---|
| `uMsg`   | `u16`    | Application message ID. `EVT_EMPTY (0)` = free slot sentinel — never written by the application. |
| `lTO`    | `u32`    | Countdown in ticks. `> 0` on insertion → handler held until `lTO` reaches 0 (Delayed behaviour). |
| `lTimer` | `u32`    | Reload value. `> 0` → after each dispatch `lTO` is reset to `lTimer` (Periodic behaviour). |
| `pMsg`   | `void *` | Optional pointer to an extended payload struct, data frame, or message queue. |

### Cross-task and ISR event injection

Any task — or any ISR — can post an event to any other task's queue via
`ADD_EVENT(TargetTask, &evt)`. This is the primary inter-task communication
mechanism: no shared globals, no mutexes, no RTOS primitives required.

```c
/* From any context — inject a NORMAL event into OtherTask */
tEvent evt;
(void)memset(&evt, 0, sizeof(evt));
evt.uMsg = MSG_FRAME_READY;
evt.pMsg = &myDataStruct;   /* attach an extended payload */
(void)ADD_EVENT(OtherTask, &evt);
```

---

## The three event types

No enum tag is needed. Behaviour is fully encoded in `lTO` and `lTimer`:

### 🔴 Delayed Event — `lTO > 0`, `lTimer = 0`

The event is inserted into the queue but the handler is **not called** until
`lTO` counts down to 0. Once dispatched the slot is freed. Use this to
schedule a one-shot action N ticks in the future.

```
Insert → [lTO=200] → tick … → [lTO=1] → tick → [lTO=0] → dispatch → FREE
```

### 🔵 Periodic Event — `lTimer > 0` (any `lTO`)

After each successful dispatch `lTO` is reloaded from `lTimer`. The event
fires repeatedly forever without any application re-arming. Use this for
heartbeats, watchdog feeds, cyclic diagnostics.

```
Insert → [lTO=0] → dispatch → reload lTO=500 → tick … → dispatch → reload …
```

### 🟡 Normal Event — `lTO = 0`, `lTimer = 0`

The event is dispatched immediately on the next scheduler pass. The slot is
freed once the handler returns a status other than `IN_PROGRESS`. Use this
for reactive, message-driven one-shot processing.

```
Insert → [lTO=0] → dispatch → (IN_PROGRESS? re-dispatch : FREE)
```

### Message Queue payload (`pMsg`)

Any event type can carry a pointer to an extended payload via `pMsg`.
In the multi-task diagram below, `MsgA Queue`, `MsgB Queue`, and `MsgC Queue`
represent separate application-level message queues whose addresses are passed
through `pMsg`. The scheduler itself is agnostic to the payload structure.

---

## Multi-task event architecture

The diagram below faithfully represents the runtime state of a three-task
NGE\_Scheduler deployment. It shows event ring-buffers, message-queue payload
pointers, and the cross-task `ADD_EVENT` arrows.

```
ADD_EVENT(NewEvent_2, TASK2)
                │
                ▼
┌──────────────────┐        ┌────────────────────────┐        ┌──────────────────┐
│  Event Queue 1   │        │    Event Queue 2        │        │  Event Queue 3   │
│ ┌──────────────┐ │  MsgA  │ ┌──────────────────┐   │  MsgB  │ ┌──────────────┐ │
│ │Full Msg G  🟡│ │ Queue  │ │pMsgA1          🔵│   │ Queue  │ │pMsgB1      🔵│ │
│ │No Event      │ │◄───────┤ │pMsgB3          🔵│   │◄──────┤ │No Event      │ │
│ │pMsgA3      🔴│ │        │ │Full Message F  🟡│   │        │ │pMsgB2      🔵│ │
│ │No Event      │ │        │ │pMsgA2          🔵│   │  MsgC  │ │pMsgC2      🔵│ │
│ │No Event      │ │        │ │pMsgA3 [delay]  🔴│   │ Queue  │ │pMsgC3      🔵│ │
│ │No Event      │ │        │ │pMsgC1          🟡│◄──┤◄──────┤ │Full Msg D  🟡│ │
│ │No Event      │ │        │ │No Event          │   │        │ │Full Msg E  🟡│ │
│ └──────────────┘ │        │ └──────────────────┘   │        │ └──────────────┘ │
│      TASK1       │        │        TASK2             │        │      TASK3       │
│  (handler fn)    │        │    (handler fn)          │        │  (handler fn)    │
└──────────────────┘        └────────────────────────┘        └──────────────────┘
         ▲                                                                │
         └────────────────────────────────────────────────────────────────┘
                         ADD_EVENT(NewEvent_1, TASK1)

Legend
  🔴 Delayed Event  — lTO > 0 at insertion; handler held until lTO == 0
  🔵 Periodic Event — lTimer > 0; lTO reloaded automatically after dispatch
  🟡 Normal Event   — lTO == 0, lTimer == 0; dispatched once immediately
     No Event       — free ring-buffer slot; invisible to the scheduler
     MsgX Queue     — application payload pointer passed via pMsg field
```

Key observations from the diagram:

- **TASK2**'s queue contains all three event types simultaneously.
  The scheduler processes each slot independently according to its `lTO`/`lTimer` values.
- **MsgA Queue** feeds from TASK2 back to TASK1's handler via a payload pointer,
  demonstrating cross-task data flow without shared globals.
- **MsgB Queue** and **MsgC Queue** feed from TASK3's queue into TASK2's handler.
- `ADD_EVENT(NewEvent_2, TASK2)` (top arrow) injects from an external sender.
- `ADD_EVENT(NewEvent_1, TASK1)` (bottom arrow) is posted by TASK3's handler —
  a task writing to another task's queue is the standard inter-task mechanism.

---

## Internal call graph

```
BSP TICK ISR  ──uSchTic = 1U──►  SchEventManager(aTaskArray)
                                           │
                               ┌───────────▼──────────────┐
                               │  while taskIdx < N        │   (stops early if
                               │  && uSchTic == 0          │    a new tick arrives)
                               │  DispatchTask(pTask)      │
                               └───────────┬──────────────┘
                                           │
                          ┌────────────────▼────────────────┐
                          │  Static:  for each ring-buf slot  │
                          │  Dynamic: while pEvt != NULL      │
                          │  ProcessEvent(pEvt, Fnct, tick)   │
                          └──────┬─────────────────┬─────────┘
                                 │                 │
                           lTO > 0           lTO == 0
                                 │                 │
                          decrement lTO     call Fnct(pEvt)
                                                   │
                                     ┌─────────────┴──────────────┐
                                  lTimer > 0               lTimer == 0
                                     │                            │
                               lTO = lTimer              result == WAIT?
                               (periodic reload)          yes        no
                                                         FREE    re-dispatch
                                                         slot    next pass
```

**Static mode** (default): event slots are a flat ring-buffer inside `tTask`.
Zero heap. Suitable for all safety-critical targets.

**Dynamic mode** (`#define DYNAMIC_ALLOCATION`): event slots are heap-allocated
nodes in a singly-linked queue. O(1) tail-append via `pEventTail`.
Tombstone-based deletion. Requires D\_05 deviation.

---

## Feature summary

| Feature | Detail |
|---|---|
| **Paradigm** | Event-centered: the Event is the primary citizen; tasks are pure handlers |
| **Language standard** | C99 (ISO/IEC 9899:1999) — no compiler extensions, no VLAs |
| **MISRA-C compliance** | MISRA-C:2012 — all Required rules satisfied; eight documented Advisory deviations |
| **Integer types** | `<stdint.h>` fixed-width only; no implicit `int` / `long` width assumptions |
| **Endian neutrality** | All multi-byte fields accessed through named struct members — no byte casting |
| **Struct padding** | Fields ordered largest-alignment-first; no internal holes on 8/16/32/64-bit |
| **Allocation** | Static ring-buffer (default, zero heap) or Dynamic linked queue (`DYNAMIC_ALLOCATION`) |
| **Event types** | 🔴 Delayed · 🔵 Periodic · 🟡 Normal — encoded in `lTO` and `lTimer`; no enum tag |
| **Payload pointer** | `pMsg void*` carries application-defined payload without scheduler involvement |
| **Cross-task posting** | `ADD_EVENT(TargetTask, &evt)` callable from any task or ISR context |
| **Tick abstraction** | BSP writes `uSchTic = 1U` in its ISR; tick period is entirely BSP-defined |
| **Critical section** | Override `SCH_ENTER_CRITICAL` / `SCH_EXIT_CRITICAL` before `#include "NGE_Scheduler.h"` |
| **MCU dependencies** | **Zero** — no MCU SDK header appears anywhere in `core/` |
| **License** | **GNU General Public License v3.0** |

---

## Repository layout

```
NGE_Scheduler/
├── core/                              Scheduler kernel — never modified by ports
│   ├── NGE_Scheduler.h                Public API, event model, MISRA deviation record
│   ├── NGE_Scheduler.c                Scheduler implementation
│   ├── NewType.h                      Fixed-width type aliases (backed by <stdint.h>)
│   ├── utility_macros.h               TOKENPASTE stub (self-contained in NGE_Scheduler.h)
│   ├── app_tasks.h                    Demo task declarations (HeartbeatTask, CommTask,
│   │                                  MonitorTask) — shared by all ports
│   └── app_tasks.c                    Demo task implementations — MCU-agnostic
│
├── ports/                             One sub-directory per supported platform
│   ├── x86_linux/                     x86-64 Linux
│   │   ├── bsp/bsp.h                  SCH_ENTER/EXIT_CRITICAL via sigprocmask(SIGALRM)
│   │   ├── bsp/bsp.c                  Tick via setitimer, LED on stdout, puts()
│   │   └── main.c                     Build: gcc -std=c99 … -o nge_linux
│   │
│   ├── x86_windows/                   x86-64 Windows
│   │   ├── bsp/bsp.h                  SCH_ENTER/EXIT_CRITICAL via CRITICAL_SECTION
│   │   ├── bsp/bsp.c                  Tick via timeSetEvent (winmm), LED on stdout
│   │   └── main.c                     Build: cl /W4 … winmm.lib
│   │
│   ├── rp2040/                        Raspberry Pi RP2040 (Pico SDK)
│   │   ├── bsp/bsp.h                  SCH_ENTER/EXIT via save_and_disable_interrupts
│   │   ├── bsp/bsp.c                  add_repeating_timer_ms(-1, …), GPIO25 LED
│   │   ├── main.c
│   │   └── CMakeLists.txt             Pico SDK CMake flow
│   │
│   ├── nxp_s32k3xx/                   NXP S32K344 (Cortex-M7)
│   │   ├── bsp/bsp.h                  SCH_ENTER/EXIT via __disable_irq / __enable_irq
│   │   ├── bsp/bsp.c                  LPIT0 channel 0 IRQ, PTA24 LED (active-low)
│   │   └── main.c                     Toolchain: S32DS / arm-none-eabi-gcc
│   │
│   ├── renesas_rh850/                 Renesas RH850/U2A (G4MH core)
│   │   ├── bsp/bsp.h                  SCH_ENTER/EXIT via __DI / __EI (CC-RH / GHS)
│   │   ├── bsp/bsp.c                  OSTM0 interval IRQ, P10_0 LED (active-low)
│   │   └── main.c                     Toolchain: CC-RH / GHS Multi
│   │
│   ├── renesas_rl78/                  Renesas RL78/G14 (16-bit)
│   │   ├── bsp/bsp.h                  SCH_ENTER/EXIT via __DI/__EI (CC-RL) or
│   │   │                              __disable/__enable_interrupt (IAR)
│   │   ├── bsp/bsp.c                  TAU0 channel 0 interval IRQ, P7_0 LED
│   │   └── main.c                     Toolchain: CC-RL / IAR EW for RL78
│   │
│   ├── infineon_tc3xx/                Infineon AURIX TC397 (TriCore)
│   │   ├── bsp/bsp.h                  SCH_ENTER/EXIT via __disable/__enable (TASKING)
│   │   │                              or "disable"/"enable" asm (Hightec GCC)
│   │   ├── bsp/bsp.c                  STM0 compare-match ISR, P02.0 LED (active-low)
│   │   └── main.c                     Toolchain: TASKING VX / Hightec GCC
│   │
│   ├── st_stm32/                      STMicroelectronics STM32 (any Cortex-M)
│   │   ├── bsp/bsp.h                  SCH_ENTER/EXIT via __disable_irq / __enable_irq
│   │   ├── bsp/bsp.c                  SysTick_Handler, PA5 LED, USART2 PrintLine
│   │   └── main.c                     Toolchain: arm-none-eabi-gcc / STM32CubeIDE
│   │
│   ├── ti_tms570/                     Texas Instruments TMS570LC43xx (Cortex-R4F)
│   │   ├── bsp/bsp.h                  SCH_ENTER/EXIT via _disable_IRQ / _enable_IRQ
│   │   ├── bsp/bsp.c                  RTI compare 0 ISR, GIOA[0] LED (active-low)
│   │   └── main.c                     Toolchain: TI ARM CGT / Code Composer Studio
│   │
│   └── microchip_pic32/               Microchip PIC32MK (MIPS32 M5150)
│       ├── bsp/bsp.h                  SCH_ENTER/EXIT via __builtin_disable/enable_interrupts
│       ├── bsp/bsp.c                  Core Timer ISR (_CP0_SET_COMPARE), RD0 LED
│       └── main.c                     Toolchain: XC32 / MPLAB X IDE
│
├── docs/
│   ├── architecture.md                Event-centered design rationale, call graph, race analysis
│   ├── porting_guide.md               Step-by-step procedure + critical-section reference table
│   └── misra_deviations.md            Full deviation log: D_01–D_08 with compensating measures
│
├── CMakeLists.txt                     Host build (Linux / Windows via CMake)
├── .gitignore
├── CHANGELOG.md
├── CONTRIBUTING.md
├── LICENSE                            GNU General Public License v3.0
└── README.md                          This file
```

---

## Quick start

### Linux

```bash
git clone https://github.com/your-org/NGE_Scheduler.git
cd NGE_Scheduler
mkdir build && cd build
cmake ..
make
./nge_linux
```

Expected output (repeating):

```
[Heartbeat] tick
[LED] ON
[Monitor] self-test OK
[Heartbeat] tick
[LED] OFF
[RX 08B]
```

### Windows

```bat
git clone https://github.com/your-org/NGE_Scheduler.git
cd NGE_Scheduler
mkdir build && cd build
cmake ..
cmake --build . --config Release
Release\nge_windows.exe
```

### Raspberry Pi RP2040

```bash
export PICO_SDK_PATH=/path/to/pico-sdk
cd ports/rp2040
mkdir build && cd build
cmake -DPICO_BOARD=pico ..
make
# Flash nge_scheduler_rp2040.uf2 onto the board
```

---

## Defining a task

`SCH_TSK_CREATE2` generates:
- the ring-buffer array `EvtArray<Name>[N]`
- the `tTask *<Name>` pointer variable
- the `init<Name>()` registration function
- the task handler opening brace (close with `ENDTASK()`)

```c
/* ── Plain tEvent task (heartbeat every 500 ticks) ──────────────────── */
SCH_TSK_CREATE2(HeartbeatTask, 1U, tEvent)
{
    /* CurrentEvent is typed tEvent*; pParent is the owning tTask*. */
    (void)pParent;
    if (CurrentEvent->uMsg == MSG_HEARTBEAT)
    {
        BSP_ToggleLED();
        BSP_PrintLine("[Heartbeat] tick");
    }
ENDTASK()

eTskStatus initHeartbeatTask(void)
{
    tEvent evt;
    (void)memset(&evt, 0, sizeof(evt));
    evt.uMsg   = MSG_HEARTBEAT;
    evt.lTimer = 500U;   /* 🔵 Periodic: fires every 500 ticks, auto-reloads */
    evt.lTO    = 0U;     /* First dispatch: immediate                          */
    (void)ADD_EVENT(HeartbeatTask, &evt);
    return TOKENPASTE(init, HeartbeatTask)();
}
```

### Extended event type (with payload)

When a task needs to receive data alongside the event, declare a concrete
event type with `tEvent base` as the **mandatory first member**:

```c
typedef struct
{
    tEvent  base;          /* MUST be first member (Rule 18.1 / D_08) */
    uint8_t aData[16];     /* application payload                      */
    uint8_t uLen;
} tCommEvent;

SCH_TSK_CREATE2(CommTask, 4U, tCommEvent)
{
    if (CurrentEvent->base.uMsg == MSG_COMM_RX)
    {
        /* CurrentEvent->aData and CurrentEvent->uLen are available here */
        process_frame(CurrentEvent->aData, CurrentEvent->uLen);
    }
ENDTASK()
```

Inject from the receive ISR or any other task:

```c
void App_OnFrameReceived(const uint8_t *pData, uint8_t uLen)
{
    tCommEvent evt;
    (void)memset(&evt, 0, sizeof(evt));
    evt.base.uMsg = MSG_COMM_RX;
    /* lTO = 0, lTimer = 0  →  🟡 Normal: dispatched immediately */
    (void)memcpy(evt.aData, pData, uLen);
    evt.uLen = uLen;
    (void)ADD_EVENT(CommTask, (tEvent *)&evt);  /* cross-task injection */
}
```

---

## Event model cheat-sheet

```c
tEvent evt;
(void)memset(&evt, 0, sizeof(evt));

/* 🟡 Normal: dispatched once on the very next scheduler pass */
evt.uMsg   = MSG_MY_EVENT;
evt.lTO    = 0U;
evt.lTimer = 0U;
(void)ADD_EVENT(MyTask, &evt);

/* 🔴 Delayed 200 ticks, then dispatched once */
evt.uMsg   = MSG_MY_EVENT;
evt.lTO    = 200U;
evt.lTimer = 0U;
(void)ADD_EVENT(MyTask, &evt);

/* 🔵 Periodic: fires every 500 ticks, reloads automatically, forever */
evt.uMsg   = MSG_MY_EVENT;
evt.lTO    = 0U;
evt.lTimer = 500U;
(void)ADD_EVENT(MyTask, &evt);

/* Cross-task / ISR injection — same API */
(void)ADD_EVENT(OtherTask, &evt);

/* With payload pointer */
evt.pMsg = &myPayloadStruct;
(void)ADD_EVENT(MyTask, &evt);
```

### Reserved `uMsg` values

| Constant | Value | Meaning |
|---|---|---|
| `EVT_EMPTY` | `0` | Slot is free (internal sentinel — **never** written by the application) |
| `EVT_INIT` | `1` | One-shot start-up event |
| `EVT_APP_BASE` | `2` | First ID available to the application |

---

## Adding a new MCU port

See [`docs/porting_guide.md`](docs/porting_guide.md) for the full procedure
and a critical-section reference table covering all supported architectures.

The three mandatory files per port:

| File | Responsibility |
|---|---|
| `ports/<target>/bsp/bsp.h` | Define `SCH_ENTER_CRITICAL` and `SCH_EXIT_CRITICAL` **before** `#include "NGE_Scheduler.h"` |
| `ports/<target>/bsp/bsp.c` | Tick ISR writing `uSchTic = 1U`; `BSP_ToggleLED()`; `BSP_PrintLine()` |
| `ports/<target>/main.c` | `BSP_Init()` → `init*Task()` calls → `SchEventManager(aTaskArray)` |

> **Golden rule: `core/` files are never modified.**
> All platform specifics live exclusively in `bsp/`.

---

## MISRA-C:2012 compliance

All **Required** rules are satisfied without deviation.
Eight **Advisory** rules carry documented deviations.

| ID | Rule | Category | Summary |
|---|---|---|---|
| D\_01 | 20.10 | Advisory | `##` in `TOKENPASTE` — required for unique symbol generation |
| D\_02 | 15.5 | Advisory | Early return in `AddEventToEventArray` for SUSPENDED guard |
| D\_03 | 14.2 | Required | `for(;;)` in `SchEventManager` — intentional non-terminating loop |
| D\_04 | 11.5 | Advisory | `tEvent*` → `void*` at handler call site — generic signature required |
| D\_05 | 21.3 | Required | `malloc`/`free` in `DYNAMIC_ALLOCATION` mode — opt-in, bounded |
| D\_06 | 11.3 | Required | `char*` → `tEvent*` in `_GET_pEVT` — only portable variable-stride accessor |
| D\_07 | 18.4 | Advisory | Pointer arithmetic in `_GET_pEVT` and `DELETE_TASK` |
| D\_08 | 11.3 | Required | `EvtType*` → `tEvent*` in `SCH_TSK_CREATE2` — size enforced by `uDataTypeLength` |

Full justifications and compensating measures: [`docs/misra_deviations.md`](docs/misra_deviations.md).

---

## Supported platforms

| Port | Vendor | CPU core | ISA | Tick source | Critical section | Toolchain |
|---|---|---|---|---|---|---|
| `x86_linux` | — | x86-64 | x86-64 | POSIX `setitimer` / SIGALRM | `sigprocmask` | GCC ≥ 9 / Clang ≥ 12 |
| `x86_windows` | — | x86-64 | x86-64 | Win32 `timeSetEvent` (winmm) | `CRITICAL_SECTION` | MSVC 2019+ / MinGW-w64 |
| `rp2040` | Raspberry Pi | Cortex-M0+ | ARMv6-M | Pico SDK `add_repeating_timer_ms` | `save_and_disable_interrupts` | arm-none-eabi-gcc + Pico SDK |
| `nxp_s32k3xx` | NXP | Cortex-M7 | ARMv7E-M+FP | LPIT0 channel 0 | `__disable_irq` / `__enable_irq` | arm-none-eabi-gcc / S32DS |
| `renesas_rh850` | Renesas | G4MH | RH850 | OSTM0 interval mode | `__DI` / `__EI` | CC-RH / GHS Multi |
| `renesas_rl78` | Renesas | RL78 | RL78 (16-bit) | TAU0 channel 0 | `__DI` / `__EI` | CC-RL / IAR EW RL78 |
| `infineon_tc3xx` | Infineon | TriCore TC1.8 | TriCore | STM0 compare register 0 | `__disable` / `__enable` | TASKING VX / Hightec GCC |
| `st_stm32` | ST | Cortex-M (any) | ARMv7-M / ARMv8-M | SysTick | `__disable_irq` / `__enable_irq` | arm-none-eabi-gcc / STM32CubeIDE |
| `ti_tms570` | Texas Instruments | Cortex-R4F | ARMv7-R | RTI counter 0 compare 0 | `_disable_IRQ` / `_enable_IRQ` | TI ARM CGT / CCS |
| `microchip_pic32` | Microchip | MIPS32 M5150 | MIPS32r2 | CP0 Core Timer | `__builtin_disable_interrupts` | XC32 v4+ / MPLAB X |

---

## License

NGE\_Scheduler is free software: you can redistribute it and/or modify it
under the terms of the **GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version**.

NGE\_Scheduler is distributed in the hope that it will be useful, but
**WITHOUT ANY WARRANTY**; without even the implied warranty of
**MERCHANTABILITY** or **FITNESS FOR A PARTICULAR PURPOSE**.
See the GNU General Public License for more details.

A copy of the GNU General Public License is included as the [`LICENSE`](LICENSE)
file in this repository.  If not, see <https://www.gnu.org/licenses/>.

```
NGE_Scheduler – Event-Centered Cooperative Scheduler
Copyright (C) 2026  New Global Electronics

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```

> **GPL v3 and embedded distribution:**
> When distributing a product — firmware, binary, or otherwise — that
> incorporates NGE\_Scheduler (modified or unmodified), you must make the
> complete corresponding source code available to recipients under GPL v3
> terms (GNU GPL v3 §6).
> Contact New Global Electronics if a **commercial licence** with different
> obligations is required.

---

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for the full contributor guide.

Key rules:

- `core/` files are **never** modified by port contributors.
- Every new deviation must be added to [`docs/misra_deviations.md`](docs/misra_deviations.md)
  with full justification and compensating measures.
- Run a MISRA-C static analysis pass (PC-lint Plus, Polyspace, or
  `cppcheck --addon=misra`) before opening a pull request.
- No MCU SDK header may appear anywhere inside `core/`.
- New ports follow the three-file structure: `bsp/bsp.h`, `bsp/bsp.c`, `main.c`.
- All contributions are accepted under the terms of **GPL v3**.
  By submitting a pull request you certify that you have the right to licence
  your contribution under these terms.
