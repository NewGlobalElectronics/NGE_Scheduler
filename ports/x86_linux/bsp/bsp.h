/*******************************************************************************
 * ports/x86_linux/bsp/bsp.h
 * NGE Scheduler – BSP for x86-64 Linux (POSIX)
 *
 * Tick source : POSIX timer_create(CLOCK_MONOTONIC) firing every 1 ms.
 * Critical section : sigprocmask(SIG_BLOCK/SIG_UNBLOCK, SIGALRM).
 ******************************************************************************/
#ifndef BSP_H
#define BSP_H

#include <signal.h>

/* Critical-section hooks: block/unblock the SIGALRM tick signal. */
#define SCH_ENTER_CRITICAL() \
    do { sigset_t _s; sigemptyset(&_s); sigaddset(&_s, SIGALRM); \
         (void)sigprocmask(SIG_BLOCK,   &_s, NULL); } while (0)

#define SCH_EXIT_CRITICAL() \
    do { sigset_t _s; sigemptyset(&_s); sigaddset(&_s, SIGALRM); \
         (void)sigprocmask(SIG_UNBLOCK, &_s, NULL); } while (0)

#include "NGE_Scheduler.h"

void BSP_Init(void);
void BSP_ToggleLED(void);
void BSP_PrintLine(const char *pStr);

#endif /* BSP_H */
