/*******************************************************************************
 * ports/infineon_tc3xx/bsp/bsp.c
 * NGE Scheduler – BSP for Infineon AURIX TC3xx (TriCore, CPU0)
 ******************************************************************************/

#include "bsp.h"

static uint32_t sNextCompare = 0UL;

/* ── STM0 compare-match ISR (CPU0 SRC: SRC_STM0SR0) ─────────────────────── */
/* TASKING: place into .text section, use __interrupt keyword. */
#if defined(__TASKING__)
__interrupt(0x10) __vector_table(0)
void STM0_SR0_ISR(void)
#else
void __attribute__((interrupt)) STM0_SR0_ISR(void)
#endif
{
    /* Advance the compare value by one period. */
    sNextCompare += (uint32_t)BSP_STM_COMPARE_INC;
    STM0_CMP0.U   = sNextCompare;
    STM0_ISCR.B.CMP0IRR = 1U;   /* clear interrupt request */
    uSchTic = 1U;
}

/* ── LED (P02.0, active-low) ─────────────────────────────────────────────── */
static uint8_t sLedState = 0U;

void BSP_ToggleLED(void)
{
    sLedState ^= 1U;
    if (sLedState != 0U)
    {
        P02_OMR.U = (uint32_t)(1UL << BSP_LED_PIN_IDX);          /* clear */
    }
    else
    {
        P02_OMR.U = (uint32_t)(1UL << (BSP_LED_PIN_IDX + 16U));  /* set   */
    }
}

void BSP_PrintLine(const char *pStr)
{
    /* In a full project write to ASC0 (UART).
     * For the demo, output via MCDS or debugger terminal. */
    (void)pStr;
}

/* ── Initialisation ──────────────────────────────────────────────────────── */
void BSP_Init(void)
{
    uint32_t stmCount;

    /* P02.0: output, push-pull, LED off (high). */
    P02_IOCR0.B.PC0 = 0x10U;           /* output push-pull */
    P02_OMR.U = (uint32_t)(1UL << (BSP_LED_PIN_IDX + 16U)); /* set = OFF */

    /* STM0: read current timer, set first compare value. */
    stmCount     = STM0_TIM0.U;
    sNextCompare = stmCount + (uint32_t)BSP_STM_COMPARE_INC;
    STM0_CMP0.U  = sNextCompare;
    STM0_CMCON.B.MSTART0 = 0U;         /* compare from bit 0 */
    STM0_CMCON.B.MSIZE0  = 31U;        /* compare all 32 bits */
    STM0_ICR.B.CMP0EN = 1U;            /* enable compare match 0 */

    /* Route STM0 SR0 to CPU0 SRC, priority 10. */
    SRC_STM0SR0.B.TOS  = 0U;           /* CPU0 */
    SRC_STM0SR0.B.SRPN = 10U;
    SRC_STM0SR0.B.SRE  = 1U;           /* enable */
}
