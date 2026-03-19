/*******************************************************************************
 * ports/ti_tms570/main.c
 * NGE Scheduler – TI TMS570LC43xx entry point
 *
 * Device    : TMS570LC4357ZWT (Cortex-R4F, 300 MHz, Hercules™ platform)
 * Toolchain : TI ARM CGT 20.x  /  Code Composer Studio
 *
 * Build (armcl):
 *   armcl -mv7R4 --code_state=32 --float_support=vfpv3d16 \
 *         -I../../core -Ibsp \
 *         ../../core/NGE_Scheduler.c ../../core/app_tasks.c \
 *         bsp/bsp.c main.c \
 *         -z -m nge_tms570.map -o nge_tms570.out
 ******************************************************************************/

#include "NGE_Scheduler.h"
#include "app_tasks.h"
#include "bsp/bsp.h"

void main(void)   /* TI startup does not pass argc/argv by default */
{
    BSP_Init();

    (void)initHeartbeatTask();
    (void)initCommTask();
    (void)initMonitorTask();

    SchEventManager(aTaskArray);
}
