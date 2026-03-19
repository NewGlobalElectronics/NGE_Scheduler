/*******************************************************************************
 * ports/x86_linux/main.c
 * NGE Scheduler – x86-64 Linux entry point
 *
 * Build:
 *   gcc -std=c99 -Wall -Wextra -pedantic \
 *       -I../../core \
 *       ../../core/NGE_Scheduler.c \
 *       ../../core/app_tasks.c \
 *       bsp/bsp.c main.c \
 *       -o nge_linux
 ******************************************************************************/

#include "NGE_Scheduler.h"
#include "app_tasks.h"
#include "bsp/bsp.h"

int main(void)
{
    /* 1. Initialise the platform tick source. */
    BSP_Init();

    /* 2. Register all application tasks. */
    (void)initHeartbeatTask();
    (void)initCommTask();
    (void)initMonitorTask();

    /* 3. Hand control to the scheduler – never returns. */
    SchEventManager(aTaskArray);

    return 0;  /* unreachable; satisfies compiler */
}
