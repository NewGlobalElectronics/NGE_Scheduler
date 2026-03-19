/*******************************************************************************
 * ports/x86_windows/bsp/bsp.c
 * NGE Scheduler – BSP for x86-64 Windows (Win32)
 ******************************************************************************/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>   /* timeSetEvent – link with winmm.lib */
#include <stdio.h>
#include "bsp.h"

/* Win32 CRITICAL_SECTION shared with the macros in bsp.h. */
CRITICAL_SECTION g_SchCritSec;

/* ── Tick callback ───────────────────────────────────────────────────────── */
static void CALLBACK Tick_Callback(UINT uTimerID, UINT uMsg,
                                   DWORD_PTR dwUser, DWORD_PTR dw1,
                                   DWORD_PTR dw2)
{
    (void)uTimerID; (void)uMsg; (void)dwUser; (void)dw1; (void)dw2;
    uSchTic = 1U;
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
    TIMECAPS tc;

    InitializeCriticalSection(&g_SchCritSec);

    /* Request the finest available timer resolution. */
    if (timeGetDevCaps(&tc, sizeof(tc)) == MMSYSERR_NOERROR)
    {
        (void)timeBeginPeriod(tc.wPeriodMin);
    }

    /* Start a 1 ms periodic multimedia timer. */
    (void)timeSetEvent(1U,          /* delay  ms */
                       1U,          /* resolution ms */
                       Tick_Callback,
                       (DWORD_PTR)0,
                       TIME_PERIODIC);
}
