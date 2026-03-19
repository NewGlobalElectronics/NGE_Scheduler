# Changelog

All notable changes to NGE_Scheduler are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [1.0.0] – 2026-03-18

### Added
- `core/NGE_Scheduler.h` and `core/NGE_Scheduler.c` — MISRA-C:2012 compliant
  cooperative scheduler kernel.
- Dual allocation mode: static ring-buffer (default) and heap-allocated linked
  queue (`#define DYNAMIC_ALLOCATION`).
- Simplified event model: behaviour fully encoded in `uMsg`, `lTO`, `lTimer`
  fields; no type-tag enum required.
- `SCH_ENTER_CRITICAL` / `SCH_EXIT_CRITICAL` hook macros for portable
  tick-read atomicity.
- `core/app_tasks.h` and `core/app_tasks.c` — three shared demo tasks
  (HeartbeatTask, CommTask, MonitorTask) used by every port.
- Ten platform ports:
  - `ports/x86_linux` — POSIX, SIGALRM tick
  - `ports/x86_windows` — Win32, multimedia timer tick
  - `ports/rp2040` — Raspberry Pi RP2040, Pico SDK repeating timer
  - `ports/nxp_s32k3xx` — NXP S32K344, Cortex-M7, LPIT0 tick
  - `ports/renesas_rh850` — Renesas RH850/U2A, G4MH core, OSTM0 tick
  - `ports/renesas_rl78` — Renesas RL78/G14, 16-bit, TAU0 tick
  - `ports/infineon_tc3xx` — Infineon AURIX TC397, TriCore, STM0 tick
  - `ports/st_stm32` — STM32 Nucleo (any Cortex-M), SysTick
  - `ports/ti_tms570` — TI TMS570LC43xx, Cortex-R4F, RTI tick
  - `ports/microchip_pic32` — Microchip PIC32MK, MIPS32, Core Timer tick
- `docs/architecture.md` — scheduler internals and design rationale.
- `docs/porting_guide.md` — step-by-step guide for adding new MCU ports.
- `docs/misra_deviations.md` — complete MISRA-C:2012 deviation log (D_01–D_08).
- Root `CMakeLists.txt` for host builds (Linux / Windows).
- `ports/rp2040/CMakeLists.txt` for Pico SDK CMake flow.
