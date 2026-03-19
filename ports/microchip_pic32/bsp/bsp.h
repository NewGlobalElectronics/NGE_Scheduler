/*******************************************************************************
 * ports/microchip_pic32/bsp/bsp.h
 * NGE Scheduler – BSP for Microchip PIC32MK (MIPS32 M5150)
 *
 * Tick source    : Core Timer (CP0 Count/Compare), 1 ms period.
 *                  Assumes SYSCLK = 120 MHz, Core Timer increments at
 *                  SYSCLK/2 = 60 MHz → compare delta = 60 000.
 * Critical section : MIPS  DI / EI  instructions.
 * LED            : RD0 (PIC32MK MCM Curiosity Pro LED1, active-low).
 *
 * Toolchain: MPLAB X + XC32 compiler
 ******************************************************************************/
#ifndef BSP_H
#define BSP_H

#include <xc.h>          /* XC32 device header – MCU-specific */
#include <sys/attribs.h> /* __ISR macro                       */
#include <stdint.h>

/* MIPS DI/EI via XC32 built-ins. */
#define SCH_ENTER_CRITICAL()  __builtin_disable_interrupts()
#define SCH_EXIT_CRITICAL()   __builtin_enable_interrupts()

#include "NGE_Scheduler.h"

/* Core Timer fires at SYSCLK/2; delta for 1 ms. */
#define BSP_CT_HZ        (60000000UL)
#define BSP_TICK_HZ      (1000UL)
#define BSP_CT_DELTA     (BSP_CT_HZ / BSP_TICK_HZ)   /* 60 000 */

void BSP_Init(void);
void BSP_ToggleLED(void);
void BSP_PrintLine(const char *pStr);

#endif /* BSP_H */
