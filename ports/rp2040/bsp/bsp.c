/*******************************************************************************
 * ports/rp2040/bsp/bsp.c
 * NGE Scheduler – BSP for Raspberry Pi RP2040 (Pico SDK)
 ******************************************************************************/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "bsp.h"

/* ── Tick callback (called from hardware timer IRQ) ─────────────────────── */
static bool Tick_Callback(struct repeating_timer *pTimer)
{
    (void)pTimer;
    uSchTic = 1U;
    return true;   /* keep repeating */
}

static struct repeating_timer sTimer;

/* ── LED ─────────────────────────────────────────────────────────────────── */
static bool sLedState = false;

void BSP_ToggleLED(void)
{
    sLedState = !sLedState;
    gpio_put(BSP_LED_PIN, sLedState ? 1U : 0U);
}

void BSP_PrintLine(const char *pStr)
{
    (void)puts(pStr);
}

/* ── Initialisation ──────────────────────────────────────────────────────── */
void BSP_Init(void)
{
    stdio_init_all();

    gpio_init(BSP_LED_PIN);
    gpio_set_dir(BSP_LED_PIN, GPIO_OUT);

    /* Negative delay means "fire every |delay| ms regardless of callback
     * execution time" (Pico SDK convention). */
    (void)add_repeating_timer_ms(-1, Tick_Callback, NULL, &sTimer);
}
