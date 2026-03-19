/*******************************************************************************
 * ports/x86_linux/bsp/bsp.c
 * NGE Scheduler – BSP for x86-64 Linux (POSIX)
 ******************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include "bsp.h"

/* ── Tick ISR ────────────────────────────────────────────────────────────── */
static void Tick_Handler(int signum)
{
    (void)signum;
    uSchTic = 1U;   /* signal the scheduler */
}

/* ── LED state (simulated on stdout) ─────────────────────────────────────── */
static int sLedState = 0;

void BSP_ToggleLED(void)
{
    sLedState ^= 1;
    (void)printf("[LED] %s\n", sLedState ? "ON" : "OFF");
}

void BSP_PrintLine(const char *pStr)
{
    (void)puts(pStr);
}

/* ── Initialisation ──────────────────────────────────────────────────────── */
void BSP_Init(void)
{
    struct sigaction sa;
    struct itimerval timer;

    /* Install SIGALRM handler for the 1 ms tick. */
    (void)memset(&sa, 0, sizeof(sa));
    sa.sa_handler = Tick_Handler;
    sigemptyset(&sa.sa_mask);
    (void)sigaction(SIGALRM, &sa, NULL);

    /* Fire every 1 ms = 1000 µs. */
    timer.it_value.tv_sec     = 0;
    timer.it_value.tv_usec    = 1000;
    timer.it_interval.tv_sec  = 0;
    timer.it_interval.tv_usec = 1000;
    (void)setitimer(ITIMER_REAL, &timer, NULL);
}
