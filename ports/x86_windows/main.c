/*******************************************************************************
 * ports/x86_windows/main.c
 * NGE Scheduler – x86-64 Windows entry point
 *
 * Build (MSVC):
 *   cl /W4 /std:c11 /I..\..\core \
 *      ..\..\core\NGE_Scheduler.c ..\..\core\app_tasks.c \
 *      bsp\bsp.c main.c \
 *      winmm.lib /Fe:nge_windows.exe
 *
 * Build (MinGW):
 *   gcc -std=c99 -Wall -Wextra -I../../core \
 *       ../../core/NGE_Scheduler.c ../../core/app_tasks.c \
 *       bsp/bsp.c main.c -lwinmm -o nge_windows.exe
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
