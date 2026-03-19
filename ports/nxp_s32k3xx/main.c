/*******************************************************************************
 * ports/nxp_s32k3xx/main.c
 * NGE Scheduler – NXP S32K3xx entry point
 *
 * Toolchain : S32DS for ARM  /  arm-none-eabi-gcc
 * Device    : S32K344 (Cortex-M7, 160 MHz max)
 *
 * Build (S32DS CMake project – add to CMakeLists.txt):
 *   target_sources(${PROJECT_NAME} PRIVATE
 *       main.c
 *       bsp/bsp.c
 *       ../../core/NGE_Scheduler.c
 *       ../../core/app_tasks.c)
 *   target_include_directories(${PROJECT_NAME} PRIVATE ../../core bsp)
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
