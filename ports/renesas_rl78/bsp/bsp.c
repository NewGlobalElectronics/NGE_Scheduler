/*******************************************************************************
 * ports/renesas_rl78/bsp/bsp.c
 * NGE Scheduler – BSP for Renesas RL78/G14
 ******************************************************************************/

#include "bsp.h"

/* ── TAU0 channel-0 interval IRQ ─────────────────────────────────────────── */
#pragma interrupt INTTM00(vect = INTTM00)
void INTTM00(void)
{
    uSchTic = 1U;
}

/* ── LED (P7_0, active-low) ──────────────────────────────────────────────── */
static uint8_t sLedState = 0U;

void BSP_ToggleLED(void)
{
    sLedState ^= 1U;
    if (sLedState != 0U)
    {
        P7  &= (uint8_t)~(1U << 0U);   /* clear → LED ON  */
    }
    else
    {
        P7  |= (uint8_t) (1U << 0U);   /* set   → LED OFF */
    }
}

void BSP_PrintLine(const char *pStr)
{
    /* UART0 transmission would go here in a full project. */
    (void)pStr;
}

/* ── Initialisation ──────────────────────────────────────────────────────── */
void BSP_Init(void)
{
    /* P7_0 as push-pull output, initial high (LED off). */
    PM7  &= (uint8_t)~(1U << 0U);   /* PM7.0 = 0 → output */
    P7   |= (uint8_t) (1U << 0U);

    /* TAU0: supply clock, set SAR = HOCO (CKM0 = 00 → fCLK/1). */
    TAU0EN = 1U;                      /* supply clock to TAU0 */
    TPS0   = 0x0000U;                 /* CKM0 = fCLK/1 (32 MHz) */

    /* Channel 0: interval timer mode, count source CK00. */
    TMR00  = 0x0000U;                 /* CK00, interval mode */
    TDR00  = BSP_TAU0_CH0_RELOAD;

    /* Enable INTTM00, clear pending, start. */
    TMMK00 = 0U;                      /* unmask */
    TMIF00 = 0U;                      /* clear flag */
    TS0   |= (uint8_t)(1U << 0U);     /* start channel 0 */
}
