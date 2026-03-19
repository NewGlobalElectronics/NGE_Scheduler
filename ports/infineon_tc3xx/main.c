/*******************************************************************************
 * ports/infineon_tc3xx/main.c
 * NGE Scheduler – Infineon AURIX TC3xx entry point (CPU0)
 *
 * Device    : TC397 (AURIX™ TC3x Application Kit)
 * Toolchain : TASKING VX-toolset 6.x  or  Hightec GCC for TriCore
 *
 * Build (TASKING):
 *   cctc main.c bsp/bsp.c ../../core/NGE_Scheduler.c \
 *        ../../core/app_tasks.c \
 *        --include-directory=../../core \
 *        --include-directory=bsp \
 *        -o nge_tc3xx.elf
 ******************************************************************************/

#include "NGE_Scheduler.h"
#include "app_tasks.h"
#include "bsp/bsp.h"

int main(void)
{
    BSP_Init();

    (void)initHeartbeatTask();
    (void)initCommTask();
    (void)initMonitorTask();

    SchEventManager(aTaskArray);

    return 0;
}
