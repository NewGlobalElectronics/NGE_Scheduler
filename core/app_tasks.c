/*******************************************************************************
 * app_tasks.c
 * NGE Scheduler – shared demo task implementations
 *
 * Three tasks demonstrate the three core scheduling patterns:
 *
 *  HeartbeatTask  Periodic:    lTimer = 500, lTO = 0.
 *                 Toggles the LED every 500 ticks automatically.
 *
 *  CommTask       One-shot event-driven:  lTimer = 0, lTO = 0.
 *                 Events injected by App_OnFrameReceived() (called from BSP).
 *
 *  MonitorTask    Delayed one-shot:  lTimer = 0, lTO = 200 (first), 1000 (re-arm).
 *                 Runs a self-test, then manually re-arms via ADD_EVENT.
 *
 * BSP hooks implemented in each port's bsp/bsp.c:
 *   void BSP_ToggleLED(void);
 *   void BSP_PrintLine(const char *pStr);
 ******************************************************************************/

#include <stdint.h>
#include <string.h>
#include "NGE_Scheduler.h"
#include "app_tasks.h"

/*******************************************************************************
 * BSP hooks (defined in the port's bsp/bsp.c)
 ******************************************************************************/
extern void BSP_ToggleLED(void);
extern void BSP_PrintLine(const char *pStr);

/*******************************************************************************
 * HeartbeatTask
 *
 * The event is inserted once in initHeartbeatTask() with lTimer = 500 so it
 * reloads automatically – no handler logic needed after the first fire.
 ******************************************************************************/
SCH_TSK_CREATE2(HeartbeatTask, 1U, tEvent)
{
    (void)pParent;
    if (CurrentEvent->uMsg == MSG_HEARTBEAT)
    {
        BSP_ToggleLED();
        BSP_PrintLine("[Heartbeat] tick");
    }
ENDTASK()

eTskStatus initHeartbeatTask(void)
{
    tEvent evt;
    eTskStatus status;

    /* Register the task in the scheduler table. */
    status = TOKENPASTE(init, HeartbeatTask)();

    /* Insert the periodic event: fires every 500 ticks, first fire immediate. */
    (void)memset(&evt, 0, sizeof(evt));
    evt.uMsg   = MSG_HEARTBEAT;
    evt.lTimer = 500U;
    evt.lTO    = 0U;
    (void)ADD_EVENT(HeartbeatTask, &evt);

    return status;
}

/*******************************************************************************
 * CommTask
 *
 * One-shot events are injected by App_OnFrameReceived(), which is called
 * from the BSP receive ISR or from the main loop (platform-dependent).
 ******************************************************************************/
SCH_TSK_CREATE2(CommTask, 4U, tCommEvent)
{
    (void)pParent;
    if (CurrentEvent->base.uMsg == MSG_COMM_RX)
    {
        /* Build a short "[RX NN B]" string without using printf. */
        char buf[12];
        buf[0]  = '[';
        buf[1]  = 'R';
        buf[2]  = 'X';
        buf[3]  = ' ';
        buf[4]  = (char)('0' + (char)(CurrentEvent->uLen / 10U));
        buf[5]  = (char)('0' + (char)(CurrentEvent->uLen % 10U));
        buf[6]  = 'B';
        buf[7]  = ']';
        buf[8]  = '\0';
        BSP_PrintLine(buf);
    }
ENDTASK()

eTskStatus initCommTask(void)
{
    return TOKENPASTE(init, CommTask)();
    /* No event pre-inserted; events arrive via App_OnFrameReceived(). */
}

void App_OnFrameReceived(const uint8_t *pData, uint8_t uLen)
{
    tCommEvent evt;
    uint8_t    copyLen;

    (void)memset(&evt, 0, sizeof(evt));
    evt.base.uMsg   = MSG_COMM_RX;
    evt.base.lTimer = 0U;
    evt.base.lTO    = 0U;

    copyLen = (uLen <= (uint8_t)sizeof(evt.aData))
              ? uLen
              : (uint8_t)sizeof(evt.aData);
    (void)memcpy(evt.aData, pData, (size_t)copyLen);
    evt.uLen = copyLen;

    (void)ADD_EVENT(CommTask, (tEvent *)&evt);
}

/*******************************************************************************
 * MonitorTask
 *
 * Fires 200 ticks after start-up, performs a self-test, then re-arms itself
 * for 1000 ticks.  Demonstrates that a one-shot handler can re-insert an event.
 ******************************************************************************/
SCH_TSK_CREATE2(MonitorTask, 1U, tEvent)
{
    (void)pParent;
    if (CurrentEvent->uMsg == MSG_MONITOR)
    {
        tEvent next;
        BSP_PrintLine("[Monitor] self-test OK");

        /* Re-arm for the next 1000-tick window. */
        (void)memset(&next, 0, sizeof(next));
        next.uMsg   = MSG_MONITOR;
        next.lTimer = 0U;
        next.lTO    = 1000U;
        (void)ADD_EVENT(MonitorTask, &next);
    }
ENDTASK()

eTskStatus initMonitorTask(void)
{
    tEvent evt;
    eTskStatus status;

    status = TOKENPASTE(init, MonitorTask)();

    (void)memset(&evt, 0, sizeof(evt));
    evt.uMsg   = MSG_MONITOR;
    evt.lTimer = 0U;
    evt.lTO    = 200U;   /* first fire 200 ticks after start-up */
    (void)ADD_EVENT(MonitorTask, &evt);

    return status;
}
