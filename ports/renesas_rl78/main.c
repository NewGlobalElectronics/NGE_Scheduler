/*******************************************************************************
 * ports/renesas_rl78/main.c
 * NGE Scheduler – Renesas RL78/G14 entry point
 *
 * Device    : R5F104xA (RL78/G14, 32 MHz HOCO)
 * Toolchain : CC-RL or IAR EW for RL78
 *
 * Build (CC-RL):
 *   ccrl main.c bsp/bsp.c ../../core/NGE_Scheduler.c \
 *        ../../core/app_tasks.c \
 *        -I../../core -Ibsp \
 *        -o nge_rl78.mot
 ******************************************************************************/

#include "NGE_Scheduler.h"
#include "app_tasks.h"
#include "bsp/bsp.h"

void main(void)
{
    BSP_Init();

    (void)initHeartbeatTask();
    (void)initCommTask();
    (void)initMonitorTask();

    SchEventManager(aTaskArray);
}
