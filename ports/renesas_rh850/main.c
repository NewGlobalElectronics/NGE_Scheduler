/*******************************************************************************
 * ports/renesas_rh850/main.c
 * NGE Scheduler – Renesas RH850/U2A entry point
 *
 * Toolchain : CCRH850 (CC-RH) or GHS Multi
 * Device    : R7F702301 (RH850/U2A-EVB)
 *
 * Build (CC-RH):
 *   ccrh850 main.c bsp/bsp.c ../../core/NGE_Scheduler.c \
 *           ../../core/app_tasks.c \
 *           -I../../core -Ibsp \
 *           -o nge_rh850.abs
 ******************************************************************************/

#include "NGE_Scheduler.h"
#include "app_tasks.h"
#include "bsp/bsp.h"

void main(void)   /* RH850 startup does not pass argc/argv */
{
    BSP_Init();

    (void)initHeartbeatTask();
    (void)initCommTask();
    (void)initMonitorTask();

    SchEventManager(aTaskArray);
}
