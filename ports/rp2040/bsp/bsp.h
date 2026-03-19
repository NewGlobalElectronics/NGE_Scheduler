/*******************************************************************************
 * ports/rp2040/bsp/bsp.h
 * NGE Scheduler – BSP for Raspberry Pi RP2040 (Pico SDK)
 *
 * Tick source    : RP2040 hardware repeating_timer at 1 ms.
 * Critical section : save_and_disable_interrupts / restore_interrupts.
 * LED            : GPIO 25 (on-board LED on Pico board).
 ******************************************************************************/
#ifndef BSP_H
#define BSP_H

#include "pico/stdlib.h"           /* Pico SDK – MCU-specific */
#include "hardware/irq.h"

/* Critical-section hooks using RP2040 interrupt save/restore. */
#define SCH_ENTER_CRITICAL()  \
    uint32_t _savedIrq = save_and_disable_interrupts()

#define SCH_EXIT_CRITICAL()   \
    restore_interrupts(_savedIrq)

#include "NGE_Scheduler.h"

#define BSP_LED_PIN   (25U)   /* on-board LED GPIO */

void BSP_Init(void);
void BSP_ToggleLED(void);
void BSP_PrintLine(const char *pStr);

#endif /* BSP_H */
