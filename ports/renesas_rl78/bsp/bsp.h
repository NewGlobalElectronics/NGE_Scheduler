/*******************************************************************************
 * ports/renesas_rl78/bsp/bsp.h
 * NGE Scheduler – BSP for Renesas RL78/G14 (16-bit)
 *
 * Tick source    : Interval Timer (IT) at 1 ms.
 *                  fIL = 15 kHz → but we use HOCO at 32 MHz with TAU unit 0
 *                  channel 0 in interval mode: reload = 32 000 - 1.
 * Critical section : RL78 DI / EI instructions.
 * LED            : P7_0.
 *
 * Toolchain: CCRL (Renesas CC-RL) or IAR EW for RL78
 ******************************************************************************/
#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include "iodefine.h"   /* Renesas RL78 device register header */

#if defined(__CCRL__)
#  include <intrinsics.h>
#  define SCH_ENTER_CRITICAL()  __DI()
#  define SCH_EXIT_CRITICAL()   __EI()
#elif defined(__ICCRL78__)
#  include <intrinsics.h>
#  define SCH_ENTER_CRITICAL()  __disable_interrupt()
#  define SCH_EXIT_CRITICAL()   __enable_interrupt()
#else
#  define SCH_ENTER_CRITICAL()
#  define SCH_EXIT_CRITICAL()
#endif

#include "NGE_Scheduler.h"

/* TAU0 channel 0 reload for 1 ms @ HOCO 32 MHz */
#define BSP_TAU0_CH0_RELOAD   (0x7CFFU)   /* 32 000 - 1 = 31 999 = 0x7CFF */

void BSP_Init(void);
void BSP_ToggleLED(void);
void BSP_PrintLine(const char *pStr);

#endif /* BSP_H */
