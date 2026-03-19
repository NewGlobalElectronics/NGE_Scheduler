/*******************************************************************************
 * ports/microchip_pic32/main.c
 * NGE Scheduler – Microchip PIC32MK entry point
 *
 * Device    : PIC32MK1024MCM100 (MIPS32 M5150, 120 MHz)
 * Toolchain : MPLAB X IDE + XC32 v4.x
 *
 * Build: open the MPLAB X project or use xc32-gcc:
 *   xc32-gcc -mprocessor=32MK1024MCM100 \
 *             -I../../core -Ibsp \
 *             ../../core/NGE_Scheduler.c ../../core/app_tasks.c \
 *             bsp/bsp.c main.c \
 *             -o nge_pic32.elf
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
