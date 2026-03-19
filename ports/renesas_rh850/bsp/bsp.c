/*******************************************************************************
 * ports/renesas_rh850/bsp/bsp.c
 * NGE Scheduler – BSP for Renesas RH850/U2A
 ******************************************************************************/

#include "bsp.h"

/* ── OSTM0 interval IRQ – mapped to channel 0 interrupt vector ───────────── */
#pragma interrupt OSTM0_IRQHandler(channel = 68, enable = false, callt = false)
void OSTM0_IRQHandler(void)
{
    uSchTic = 1U;
    /* OSTM interval mode clears the interrupt flag automatically on read. */
}

/* ── LED (P10_0, active-low example) ─────────────────────────────────────── */
static uint8_t sLedState = 0U;

void BSP_ToggleLED(void)
{
    sLedState ^= 1U;
    if (sLedState != 0U)
    {
        P10 &= ~(uint16_t)(1U << 0U);   /* clear bit = LED ON  */
    }
    else
    {
        P10 |= (uint16_t)(1U << 0U);    /* set bit   = LED OFF */
    }
}

void BSP_PrintLine(const char *pStr)
{
    /* In a full project this would write to RLIN3 UART.
     * For the demo we write to the debugger JTAG console via __write(). */
    (void)pStr;
}

/* ── Initialisation ──────────────────────────────────────────────────────── */
void BSP_Init(void)
{
    /* Configure P10_0 as output. */
    PM10  &= ~(uint16_t)(1U << 0U);   /* PM = 0 → output */
    P10   |=  (uint16_t)(1U << 0U);   /* initial state: LED off */

    /* OSTM0: stop, set interval mode (bit 1 = 0), enable count start IRQ. */
    OSTM0.OSTMnTT  = 0x01U;           /* stop */
    OSTM0.OSTMnCTL = 0x00U;           /* interval mode, no start-count IRQ */
    OSTM0.OSTMnCMP = BSP_OSTM0_RELOAD;

    /* Enable OSTM0 channel-0 interrupt at priority 5. */
    /* INTC1 EIC register for OSTM0 INT (channel 68): clear MK, set priority */
    INTC1.ICP68.UINT16 = 0x0005U;     /* priority 5, MK = 0 (enabled) */

    /* Start OSTM0. */
    OSTM0.OSTMnTS = 0x01U;
}
