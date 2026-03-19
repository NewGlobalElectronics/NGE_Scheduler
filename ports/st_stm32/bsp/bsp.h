/*******************************************************************************
 * ports/st_stm32/bsp/bsp.h
 * NGE Scheduler – BSP for STM32 family (Cortex-M, HAL-free bare-metal)
 *
 * Covers : STM32F4, STM32G0, STM32H7, STM32U5 (and any Cortex-M derivative).
 *
 * Tick source    : SysTick exception at 1 ms.
 *                  SystemCoreClock is set by the PLL init before BSP_Init().
 * Critical section : CMSIS  __disable_irq / __enable_irq.
 * LED            : PA5 (Nucleo-64 boards LD2, active-high).
 *
 * Toolchain: arm-none-eabi-gcc / STM32CubeIDE / Keil MDK
 ******************************************************************************/
#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include "stm32_hal_wrapper.h"   /* thin wrapper: just exposes CMSIS core_cm*.h
                                    and the SystemCoreClock extern – no HAL.  */
#include "core_cm4.h"            /* or core_cm7.h / core_cm33.h for the target */

#define SCH_ENTER_CRITICAL()  __disable_irq()
#define SCH_EXIT_CRITICAL()   __enable_irq()

#include "NGE_Scheduler.h"

/* Nucleo-64: LD2 on PA5, active-high */
#define BSP_LED_PORT   GPIOA
#define BSP_LED_PIN    (5U)

extern uint32_t SystemCoreClock;

void BSP_Init(void);
void BSP_ToggleLED(void);
void BSP_PrintLine(const char *pStr);

#endif /* BSP_H */
