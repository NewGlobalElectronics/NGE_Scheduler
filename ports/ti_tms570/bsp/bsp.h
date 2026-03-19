/*******************************************************************************
 * ports/ti_tms570/bsp/bsp.h
 * NGE Scheduler – BSP for TI TMS570LC43xx (Cortex-R4F, ASIL-D capable)
 *
 * Tick source    : RTI (Real-Time Interrupt) counter 0 compare 0, 1 ms.
 *                  RTICLK = HCLK/2 = 80 MHz → compare period = 80 000.
 * Critical section : ARM  CPSID i / CPSIE i  (disable/enable IRQ).
 * LED            : GIOA[0] (TMS570 LaunchPad LED1, active-low).
 *
 * Toolchain: TI ARM CGT (armcl) / TI Code Composer Studio
 * SDK:       HALCoGen or bare-metal register access
 ******************************************************************************/
#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include "reg_rti.h"    /* TI HALCoGen RTI register header  – MCU-specific */
#include "reg_gio.h"    /* TI HALCoGen GIO register header                 */
#include "sys_vim.h"    /* TI HALCoGen VIM (Vectored Interrupt Manager)     */

/* Cortex-R4 CPSID/CPSIE via TI CGT intrinsics. */
#if defined(__TI_ARM__)
#  define SCH_ENTER_CRITICAL()  _disable_IRQ()
#  define SCH_EXIT_CRITICAL()   _enable_IRQ()
#else
#  define SCH_ENTER_CRITICAL()  __asm volatile("cpsid i" ::: "memory")
#  define SCH_EXIT_CRITICAL()   __asm volatile("cpsie i" ::: "memory")
#endif

#include "NGE_Scheduler.h"

#define BSP_RTICLK_HZ    (80000000UL)
#define BSP_TICK_HZ      (1000UL)
#define BSP_RTI_COMPARE  (BSP_RTICLK_HZ / BSP_TICK_HZ)   /* 80 000 */

void BSP_Init(void);
void BSP_ToggleLED(void);
void BSP_PrintLine(const char *pStr);

#endif /* BSP_H */
