/*******************************************************************************
 * ports/infineon_tc3xx/bsp/bsp.h
 * NGE Scheduler – BSP for Infineon AURIX TC3xx (TriCore)
 *
 * Tick source    : STM0 (System Timer 0) compare register 0, 1 ms period.
 *                  Assumes STM clock = 100 MHz → compare increment = 100 000.
 * Critical section : TriCore DISABLE / ENABLE instructions via CSFR.
 * LED            : P02.0 (AURIX TC397 Application Kit LED1, active-low).
 *
 * Toolchain: TASKING VX-toolset for TriCore / Hightec GCC
 * SDK:       iLLD (Infineon Low Level Drivers)
 ******************************************************************************/
#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include "IfxStm_reg.h"    /* iLLD STM register map – MCU-specific */
#include "IfxPort_reg.h"   /* iLLD PORT register map               */
#include "IfxSrc_reg.h"    /* iLLD SRC (Service Request) registers */

/* TriCore DISABLE/ENABLE via CSFR instruction (TASKING / Hightec intrinsics). */
#if defined(__TASKING__)
#  define SCH_ENTER_CRITICAL()  __disable()
#  define SCH_EXIT_CRITICAL()   __enable()
#elif defined(__GNUC__) && defined(__tricore__)
#  define SCH_ENTER_CRITICAL()  __asm__ volatile("disable" ::: "memory")
#  define SCH_EXIT_CRITICAL()   __asm__ volatile("enable"  ::: "memory")
#else
#  define SCH_ENTER_CRITICAL()
#  define SCH_EXIT_CRITICAL()
#endif

#include "NGE_Scheduler.h"

#define BSP_STM_CLOCK_HZ     (100000000UL)
#define BSP_TICK_HZ          (1000UL)
#define BSP_STM_COMPARE_INC  (BSP_STM_CLOCK_HZ / BSP_TICK_HZ)   /* 100 000 */

/* TC397 AK LED1 = P02.0, active-low */
#define BSP_LED_PORT_IDX   (2U)
#define BSP_LED_PIN_IDX    (0U)

void BSP_Init(void);
void BSP_ToggleLED(void);
void BSP_PrintLine(const char *pStr);

#endif /* BSP_H */
