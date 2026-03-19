/*******************************************************************************
 * ports/nxp_s32k3xx/bsp/bsp.c
 * NGE Scheduler – BSP for NXP S32K3xx (Cortex-M7)
 *
 * Clock assumption: SPLLDIV2_CLK = 40 MHz → LPIT reload = 40 000 - 1.
 ******************************************************************************/

#include <string.h>
#include "bsp.h"

#define LPIT0_CH0_RELOAD   (39999UL)   /* 40 MHz / 40 000 = 1 kHz = 1 ms */

/* ── LPIT0 channel-0 IRQ handler ─────────────────────────────────────────── */
void LPIT0_Ch0_IRQHandler(void)
{
    /* Clear channel 0 timeout flag. */
    LPIT0->MSR = LPIT_MSR_TIF0_MASK;
    uSchTic = 1U;
}

/* ── LED ─────────────────────────────────────────────────────────────────── */
static uint8_t sLedState = 0U;

void BSP_ToggleLED(void)
{
    sLedState ^= 1U;
    if (sLedState != 0U)
    {
        BSP_LED_PORT->PCOR = (uint32_t)(1UL << BSP_LED_PIN); /* clear = ON  */
    }
    else
    {
        BSP_LED_PORT->PSOR = (uint32_t)(1UL << BSP_LED_PIN); /* set = OFF   */
    }
}

void BSP_PrintLine(const char *pStr)
{
    /* Route to LPUART0 configured at 115 200 baud in a full project.
     * For the demo we write to the debugger ITM stimulus port 0. */
    while (*pStr != '\0')
    {
        /* ITM stimulus port 0 (SWO output). */
        while (ITM->PORT[0].u32 == 0UL) { /* wait */ }
        ITM->PORT[0].u8 = (uint8_t)*pStr;
        pStr++;
    }
    while (ITM->PORT[0].u32 == 0UL) { /* wait */ }
    ITM->PORT[0].u8 = (uint8_t)'\n';
}

/* ── Initialisation ──────────────────────────────────────────────────────── */
void BSP_Init(void)
{
    /* Enable clocks: PCC_LPIT0, PCC_PORTA. */
    PCC->PCCn[PCC_LPIT0_INDEX]  = PCC_PCCn_CGC_MASK;
    PCC->PCCn[PCC_PORTA_INDEX]  = PCC_PCCn_CGC_MASK;

    /* Configure PTA24 as GPIO output, initially high (LED off). */
    BSP_LED_PORT->PSOR  = (uint32_t)(1UL << BSP_LED_PIN);
    PORTA->PCR[BSP_LED_PIN] = PORT_PCR_MUX(1U);   /* MUX = GPIO */
    BSP_LED_PORT->PDDR |= (uint32_t)(1UL << BSP_LED_PIN);

    /* LPIT0: enable module, configure channel 0 for 32-bit periodic counter. */
    LPIT0->MCR  = LPIT_MCR_M_CEN_MASK;
    LPIT0->TMR[0].TVAL = LPIT0_CH0_RELOAD;
    LPIT0->TMR[0].TCTRL = LPIT_TMR_TCTRL_T_EN_MASK;   /* start */

    /* Enable LPIT0 channel-0 interrupt in NVIC at priority 1. */
    NVIC_SetPriority(LPIT0_Ch0_IRQn, 1U);
    NVIC_EnableIRQ(LPIT0_Ch0_IRQn);
}
