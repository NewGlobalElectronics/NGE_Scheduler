/*******************************************************************************
 * ports/x86_windows/bsp/bsp.h
 * NGE Scheduler – BSP for x86-64 Windows (Win32)
 *
 * Tick source    : Win32 timeSetEvent() multimedia timer, 1 ms period.
 * Critical section : Windows CRITICAL_SECTION to guard uSchTic access.
 ******************************************************************************/
#ifndef BSP_H
#define BSP_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/* Critical-section hooks wrapping a Win32 CRITICAL_SECTION.
 * The object is defined in bsp.c and declared extern here so the
 * macros can reference it from any translation unit that includes bsp.h. */
extern CRITICAL_SECTION g_SchCritSec;

#define SCH_ENTER_CRITICAL()  EnterCriticalSection(&g_SchCritSec)
#define SCH_EXIT_CRITICAL()   LeaveCriticalSection(&g_SchCritSec)

#include "NGE_Scheduler.h"

void BSP_Init(void);
void BSP_ToggleLED(void);
void BSP_PrintLine(const char *pStr);

#endif /* BSP_H */
