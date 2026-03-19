/*******************************************************************************
 * ports/nxp_s32k3xx/bsp/bsp.h
 * NGE Scheduler – BSP for NXP S32K3xx (Cortex-M7)
 *
 * Tick source    : LPIT channel 0, 1 ms period, sourced from SPLLDIV2_CLK.
 * Critical section : ARM CMSIS  __disable_irq / __enable_irq.
 * LED            : PTA24 (S32K344-EVB green LED, active-low).
 *
 * Toolchain: S32DS for ARM / GCC arm-none-eabi
 * SDK:       RTD (Real-Time Drivers) or bare-metal register access.
 ******************************************************************************/
#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include "S32K344.h"   /* NXP device header – MCU-specific */
#include "core_cm7.h"  /* CMSIS Cortex-M7 core functions   */

#define SCH_ENTER_CRITICAL()  __disable_irq()
#define SCH_EXIT_CRITICAL()   __enable_irq()

#include "NGE_Scheduler.h"

/* S32K344-EVB: green LED on PTA24 (active-low) */
#define BSP_LED_PORT   PTA
#define BSP_LED_PIN    (24U)

void BSP_Init(void);
void BSP_ToggleLED(void);
void BSP_PrintLine(const char *pStr);

#endif /* BSP_H */
