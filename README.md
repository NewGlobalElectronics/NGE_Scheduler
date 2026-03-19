# NGE_Scheduler

**New Global Electronics – Portable Cooperative Scheduler**

A lightweight, MISRA-C:2012 compliant, fully MCU-agnostic cooperative event
scheduler targeting automotive, industrial, and embedded systems.

---

## Feature summary

| Feature | Detail |
|---|---|
| Language standard | C99 (ISO/IEC 9899:1999) |
| MISRA-C compliance | MISRA-C:2012 – all Required rules; documented deviations for Advisory |
| Integer types | `<stdint.h>` fixed-width only; no `int` / `long` size assumptions |
| Endian neutrality | All multi-byte fields accessed through named struct members only |
| Struct padding | Fields ordered largest-alignment-first; no internal holes on 8/16/32/64-bit |
| Allocation mode | **Static** (default, zero heap) or **Dynamic** (`#define DYNAMIC_ALLOCATION`) |
| Event model | `uMsg`, `lTO`, `lTimer` fields fully describe every behaviour – no type tag enum |
| Tick abstraction | Platform writes `uSchTic = 1U` in its ISR; tick period is BSP-defined |
| Critical section | Override `SCH_ENTER_CRITICAL` / `SCH_EXIT_CRITICAL` before including the header |
| MCU dependencies | Zero – no MCU SDK header appears in `core/` |

---

## Repository layout

```
NGE_Scheduler/
├── core/                         Scheduler kernel + shared application layer
│   ├── NGE_Scheduler.h           Public API, event model, deviation record
│   ├── NGE_Scheduler.c           Scheduler implementation
│   ├── NewType.h                 Minimal fixed-width type aliases
│   ├── utility_macros.h          Preprocessor utilities stub
│   ├── app_tasks.h               Shared demo task declarations
│   └── app_tasks.c               Shared demo task implementations
│
├── ports/                        One sub-directory per supported platform
│   ├── x86_linux/                x86-64, Linux (POSIX SIGALRM tick)
│   │   ├── bsp/bsp.h
│   │   ├── bsp/bsp.c
│   │   └── main.c
│   ├── x86_windows/              x86-64, Windows (Win32 multimedia timer)
│   │   ├── bsp/bsp.h
│   │   ├── bsp/bsp.c
│   │   └── main.c
│   ├── rp2040/                   Raspberry Pi RP2040 (Pico SDK)
│   │   ├── bsp/bsp.h
│   │   ├── bsp/bsp.c
│   │   ├── main.c
│   │   └── CMakeLists.txt
│   ├── nxp_s32k3xx/              NXP S32K3xx – Cortex-M7 (LPIT tick)
│   │   ├── bsp/bsp.h
│   │   ├── bsp/bsp.c
│   │   └── main.c
│   ├── renesas_rh850/            Renesas RH850/U2A – G4MH (OSTM0 tick)
│   │   ├── bsp/bsp.h
│   │   ├── bsp/bsp.c
│   │   └── main.c
│   ├── renesas_rl78/             Renesas RL78/G14 – 16-bit (TAU0 tick)
│   │   ├── bsp/bsp.h
│   │   ├── bsp/bsp.c
│   │   └── main.c
│   ├── infineon_tc3xx/           Infineon AURIX TC3xx – TriCore (STM0 tick)
│   │   ├── bsp/bsp.h
│   │   ├── bsp/bsp.c
│   │   └── main.c
│   ├── st_stm32/                 STMicroelectronics STM32 – Cortex-M (SysTick)
│   │   ├── bsp/bsp.h
│   │   ├── bsp/bsp.c
│   │   └── main.c
│   ├── ti_tms570/                Texas Instruments TMS570 – Cortex-R4F (RTI tick)
│   │   ├── bsp/bsp.h
│   │   ├── bsp/bsp.c
│   │   └── main.c
│   └── microchip_pic32/          Microchip PIC32MK – MIPS32 (Core Timer tick)
│       ├── bsp/bsp.h
│       ├── bsp/bsp.c
│       └── main.c
│
├── docs/
│   ├── architecture.md           Scheduler internals and design rationale
│   ├── porting_guide.md          Step-by-step guide to adding a new port
│   └── misra_deviations.md       Full MISRA-C:2012 deviation log
│
├── CMakeLists.txt                Host build (Linux / Windows)
├── .gitignore
├── CHANGELOG.md
└── README.md                     This file
```

---

## Quick start – Linux

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
...
```

## Quick start – Windows

```bat
mkdir build && cd build
cmake ..
cmake --build . --config Release
Release\nge_windows.exe
```

---

## Adding a new MCU port

See [`docs/porting_guide.md`](docs/porting_guide.md) for the full procedure.
The three mandatory steps are:

1. Create `ports/<target>/bsp/bsp.h` – define `SCH_ENTER_CRITICAL` /
   `SCH_EXIT_CRITICAL` and include `NGE_Scheduler.h`.
2. Create `ports/<target>/bsp/bsp.c` – implement the tick ISR that writes
   `uSchTic = 1U`, `BSP_ToggleLED()`, and `BSP_PrintLine()`.
3. Create `ports/<target>/main.c` – call `BSP_Init()`, register tasks, call
   `SchEventManager(aTaskArray)`.

The `core/` files are never modified.

---

## Event model cheat-sheet

```c
tEvent evt;
memset(&evt, 0, sizeof(evt));

/* One-shot, immediate */
evt.uMsg   = MSG_MY_EVENT;
evt.lTO    = 0U;
evt.lTimer = 0U;
ADD_EVENT(MyTask, &evt);

/* Delayed 200 ticks, one-shot */
evt.uMsg   = MSG_MY_EVENT;
evt.lTO    = 200U;
evt.lTimer = 0U;
ADD_EVENT(MyTask, &evt);

/* Periodic every 500 ticks */
evt.uMsg   = MSG_MY_EVENT;
evt.lTO    = 0U;
evt.lTimer = 500U;
ADD_EVENT(MyTask, &evt);
```

---

## MISRA-C:2012 compliance

All **Required** rules are satisfied without deviation.
Eight **Advisory** rules carry documented deviations (D_01 through D_08).
See [`docs/misra_deviations.md`](docs/misra_deviations.md) for the complete
deviation log including justification and compensating measures.

---

## Supported platforms

| Port | Core | Tick source | Toolchain |
|---|---|---|---|
| x86 Linux | x86-64 | POSIX SIGALRM | GCC / Clang |
| x86 Windows | x86-64 | Win32 multimedia timer | MSVC / MinGW |
| Raspberry Pi RP2040 | Cortex-M0+ | repeating_timer | arm-none-eabi-gcc + Pico SDK |
| NXP S32K3xx | Cortex-M7 | LPIT0 channel 0 | arm-none-eabi-gcc / S32DS |
| Renesas RH850/U2A | G4MH | OSTM0 | CC-RH / GHS Multi |
| Renesas RL78/G14 | RL78 (16-bit) | TAU0 channel 0 | CC-RL / IAR EW RL78 |
| Infineon AURIX TC3xx | TriCore | STM0 compare 0 | TASKING / Hightec GCC |
| STM32 (any) | Cortex-M | SysTick | arm-none-eabi-gcc / STM32CubeIDE |
| TI TMS570LC43xx | Cortex-R4F | RTI compare 0 | TI ARM CGT / CCS |
| Microchip PIC32MK | MIPS32 M5150 | Core Timer | XC32 / MPLAB X |

---

## License

MIT – see `LICENSE` file.
