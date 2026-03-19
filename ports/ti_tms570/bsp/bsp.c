/*******************************************************************************
 * ports/ti_tms570/bsp/bsp.c
 * NGE Scheduler – BSP for TI TMS570LC43xx
 ******************************************************************************/

#include "bsp.h"

/* ── RTI compare 0 ISR ───────────────────────────────────────────────────── */
/* HALCoGen VIM places this at vector 2 (RTI compare interrupt 0). */
void rtiCompare0Interrupt(void)
{
    /* Clear RTI compare 0 interrupt flag. */
    rtiREG1->INTFLAG = (uint32_t)(1UL << 0U);
    /* Reload: add one period to the compare value (free-running counter). */
    rtiREG1->CMP[0].COMPx += (uint32_t)BSP_RTI_COMPARE;
    uSchTic = 1U;
}

/* ── LED (GIOA[0], active-low) ───────────────────────────────────────────── */
static uint8_t sLedState = 0U;

void BSP_ToggleLED(void)
{
    sLedState ^= 1U;
    if (sLedState != 0U)
    {
        gioPORTA->DCLR = (uint32_t)(1UL << 0U);   /* clear = ON  */
    }
    else
    {
        gioPORTA->DSET = (uint32_t)(1UL << 0U);   /* set   = OFF */
    }
}

void BSP_PrintLine(const char *pStr)
{
    /* Route to SCI1 (UART) in a full project. */
    (void)pStr;
}

/* ── Initialisation ──────────────────────────────────────────────────────── */
void BSP_Init(void)
{
    /* GIO: enable, GIOA[0] as output, initial high (LED off). */
    gioREG->GCR0          = 1U;
    gioPORTA->DIR         |= (uint32_t)(1UL << 0U);
    gioPORTA->DSET         = (uint32_t)(1UL << 0U);

    /* RTI: counter 0 uses RTICLK (prescaled HCLK/2 = 80 MHz).
     * Free-running up-counter; compare 0 fires every 80 000 counts = 1 ms. */
    rtiREG1->GCTRL  = 0x00000000UL;   /* stop all counters       */
    rtiREG1->TBCTRL = 0x00000000UL;   /* use internal RTICLK     */
    rtiREG1->CAPCTRL = 0x00000000UL;
    rtiREG1->COMPCTRL = 0x00000000UL; /* counter 0 for compare 0 */

    rtiREG1->CNT[0].FRCx = 0UL;
    rtiREG1->CNT[0].UCx  = 0UL;
    rtiREG1->CNT[0].CPUCx = 0UL;     /* prescale = 1 (no division) */

    rtiREG1->CMP[0].COMPx  = (uint32_t)BSP_RTI_COMPARE;
    rtiREG1->CMP[0].UDCPx  = (uint32_t)BSP_RTI_COMPARE;

    rtiREG1->INTFLAG = 0x0007000FUL;  /* clear all flags         */
    rtiREG1->INTENAS = (uint32_t)(1UL << 0U); /* enable compare 0 IRQ */

    /* VIM: enable RTI compare 0 interrupt (channel 2). */
    vimREG->REQMASKSET0 = (uint32_t)(1UL << 2U);

    /* Start counter 0. */
    rtiREG1->GCTRL = 0x00000001UL;
}
