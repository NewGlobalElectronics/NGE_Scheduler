/*******************************************************************************
 * ports/microchip_pic32/bsp/bsp.c
 * NGE Scheduler – BSP for Microchip PIC32MK (MIPS32)
 ******************************************************************************/

#include "bsp.h"

static uint32_t sNextCompare = 0UL;

/* ── Core Timer ISR  ─────────────────────────────────────────────────────── */
void __ISR(_CORE_TIMER_VECTOR, IPL2SOFT) CoreTimer_Handler(void)
{
    /* Reload compare by adding one delta to the current Compare register. */
    sNextCompare += (uint32_t)BSP_CT_DELTA;
    _CP0_SET_COMPARE(sNextCompare);
    IFS0CLR = _IFS0_CTIF_MASK;   /* clear Core Timer interrupt flag */
    uSchTic = 1U;
}

/* ── LED (RD0, active-low) ───────────────────────────────────────────────── */
static uint8_t sLedState = 0U;

void BSP_ToggleLED(void)
{
    sLedState ^= 1U;
    if (sLedState != 0U)
    {
        LATDCLR = _LATD_LATD0_MASK;   /* clear = LED ON  */
    }
    else
    {
        LATDSET = _LATD_LATD0_MASK;   /* set   = LED OFF */
    }
}

void BSP_PrintLine(const char *pStr)
{
    /* Route to UART1 in a full project. */
    (void)pStr;
}

/* ── Initialisation ──────────────────────────────────────────────────────── */
void BSP_Init(void)
{
    /* RD0 as digital output, initial high (LED off). */
    ANSELDbits.ANSD0 = 0;   /* digital mode */
    TRISDbits.TRISD0 = 0;   /* output       */
    LATDSET = _LATD_LATD0_MASK;

    /* Core Timer: load Compare, set priority, enable. */
    sNextCompare = _CP0_GET_COUNT() + (uint32_t)BSP_CT_DELTA;
    _CP0_SET_COMPARE(sNextCompare);

    IPC0bits.CTIP = 2U;   /* priority 2 */
    IPC0bits.CTIS = 0U;   /* sub-priority 0 */
    IFS0CLR = _IFS0_CTIF_MASK;
    IEC0SET = _IEC0_CTIE_MASK;

    /* Global interrupt enable. */
    __builtin_enable_interrupts();
}
