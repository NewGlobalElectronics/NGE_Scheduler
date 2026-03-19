/*******************************************************************************
 * ports/st_stm32/main.c
 * NGE Scheduler – STM32 entry point (Nucleo-64 or any Cortex-M board)
 *
 * Tested on : STM32F411RE Nucleo-64 (168 MHz, 512 KB Flash, 128 KB SRAM)
 * Toolchain : arm-none-eabi-gcc  /  STM32CubeIDE
 *
 * Build (Makefile):
 *   arm-none-eabi-gcc -std=c99 -Wall -Wextra \
 *       -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
 *       -I../../core -Ibsp \
 *       ../../core/NGE_Scheduler.c ../../core/app_tasks.c \
 *       bsp/bsp.c main.c startup_stm32f411xe.s \
 *       -TSTM32F411RETx_FLASH.ld \
 *       -o nge_stm32.elf
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
