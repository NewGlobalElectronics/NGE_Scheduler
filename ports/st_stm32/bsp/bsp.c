/*******************************************************************************
 * ports/st_stm32/bsp/bsp.c
 * NGE Scheduler – BSP for STM32 (bare-metal, no HAL)
 *
 * Register access uses the CMSIS device header directly.
 * Adapt the PLL configuration and UART baud-rate divisor for your exact part.
 ******************************************************************************/

#include <string.h>
#include "bsp.h"

/* ── SysTick handler ─────────────────────────────────────────────────────── */
void SysTick_Handler(void)
{
    uSchTic = 1U;
}

/* ── LED ─────────────────────────────────────────────────────────────────── */
static uint8_t sLedState = 0U;

void BSP_ToggleLED(void)
{
    sLedState ^= 1U;
    if (sLedState != 0U)
    {
        BSP_LED_PORT->BSRR = (uint32_t)(1UL << BSP_LED_PIN);         /* set */
    }
    else
    {
        BSP_LED_PORT->BSRR = (uint32_t)(1UL << (BSP_LED_PIN + 16U)); /* reset */
    }
}

/* ── USART2 print (PA2 TX, 115 200 baud @ 16 MHz HSI) ───────────────────── */
void BSP_PrintLine(const char *pStr)
{
    while (*pStr != '\0')
    {
        while ((USART2->SR & USART_SR_TXE) == 0U) { /* wait */ }
        USART2->DR = (uint32_t)((uint8_t)*pStr);
        pStr++;
    }
    while ((USART2->SR & USART_SR_TXE) == 0U) { /* wait */ }
    USART2->DR = (uint32_t)'\n';
}

/* ── Initialisation ──────────────────────────────────────────────────────── */
void BSP_Init(void)
{
    /* ---- Clocks ---------------------------------------------------------- */
    /* Enable GPIOA and USART2 clocks (AHB1 / APB1). */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* ---- PA5: push-pull output, LED off ---------------------------------- */
    GPIOA->MODER  &= ~(3UL << (BSP_LED_PIN * 2U));
    GPIOA->MODER  |=  (1UL << (BSP_LED_PIN * 2U));   /* output mode */
    GPIOA->OTYPER &= ~(1UL << BSP_LED_PIN);            /* push-pull  */
    GPIOA->BSRR    = (uint32_t)(1UL << (BSP_LED_PIN + 16U)); /* reset */

    /* ---- PA2: USART2 TX (AF7) ------------------------------------------- */
    GPIOA->MODER   &= ~(3UL << (2U * 2U));
    GPIOA->MODER   |=  (2UL << (2U * 2U));   /* alternate function */
    GPIOA->AFR[0]  &= ~(0xFUL << (2U * 4U));
    GPIOA->AFR[0]  |=  (7UL   << (2U * 4U)); /* AF7 = USART2       */

    /* ---- USART2 at 115 200 baud (HSI 16 MHz) ----------------------------- */
    USART2->BRR = (uint32_t)139U;   /* 16 000 000 / 115 200 ≈ 139 */
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;

    /* ---- SysTick at 1 ms ------------------------------------------------- */
    (void)SysTick_Config(SystemCoreClock / 1000UL);
}
