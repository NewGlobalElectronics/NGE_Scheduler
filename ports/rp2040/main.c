/*******************************************************************************
 * ports/rp2040/main.c
 * NGE Scheduler – Raspberry Pi RP2040 entry point
 *
 * Build: standard Pico SDK CMake flow.
 *   mkdir build && cd build
 *   cmake -DPICO_BOARD=pico .. && make
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
