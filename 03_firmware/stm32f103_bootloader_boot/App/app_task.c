/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: app_task.c
 *  Description: Application layer - simple task scheduler implementation
 *               Uses HAL_GetTick() as the time base for periodic task execution.
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.20            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "app_task.h"
#include "stm32f1xx_hal.h"

/*==============================================================================
 * Local Variables
 *============================================================================*/
static app_task_t g_task_table[APP_TASK_MAX_NUM];
static uint8_t    g_task_count;

/*==============================================================================
 *  Function: void app_task_init(void)
 *  Input:  none
 *  Output: none
 *  Return: none
 *  Description: Initialize the task scheduler.
 *               Call once during system initialization.
 *============================================================================*/
void app_task_init(void)
{
    uint8_t i;

    for (i = 0; i < APP_TASK_MAX_NUM; i++)
    {
        g_task_table[i].callback    = (app_task_cb_t)0;
        g_task_table[i].interval_ms = 0;
        g_task_table[i].last_run    = 0;
        g_task_table[i].enabled     = 0;
    }
    g_task_count = 0;
}

/*==============================================================================
 *  Function: int8_t app_task_create(app_task_cb_t callback, uint32_t interval_ms)
 *  Input:  callback    - task function pointer
 *          interval_ms - execution interval in milliseconds
 *  Return: task ID (0 ~ APP_TASK_MAX_NUM-1) on success
 *          -1 if task table is full
 *  Description: Register a new periodic task.
 *               The task will first run after interval_ms from creation.
 *============================================================================*/
int8_t app_task_create(app_task_cb_t callback, uint32_t interval_ms)
{
    if (g_task_count >= APP_TASK_MAX_NUM)
    {
        return -1;
    }

    if (callback == (app_task_cb_t)0 || interval_ms == 0)
    {
        return -1;
    }

    g_task_table[g_task_count].callback    = callback;
    g_task_table[g_task_count].interval_ms = interval_ms;
    g_task_table[g_task_count].last_run    = HAL_GetTick();
    g_task_table[g_task_count].enabled     = 1;

    g_task_count++;
    return (int8_t)(g_task_count - 1);
}

/*==============================================================================
 *  Function: void app_task_run(void)
 *  Input:  none
 *  Output: none
 *  Return: none
 *  Description: Run the task scheduler for one cycle.
 *               Iterates all registered tasks and executes those whose
 *               interval has elapsed. Call frequently from main loop.
 *============================================================================*/
void app_task_run(void)
{
    uint8_t  i;
    uint32_t now;

    now = HAL_GetTick();

    for (i = 0; i < g_task_count; i++)
    {
        if (g_task_table[i].enabled == 0)
        {
            continue;
        }

        if ((now - g_task_table[i].last_run) >= g_task_table[i].interval_ms)
        {
            g_task_table[i].last_run = now;
            if (g_task_table[i].callback != (app_task_cb_t)0)
            {
                g_task_table[i].callback();
            }
        }
    }
}

/*==============================================================================
 *  Function: void app_task_enable(uint8_t id)
 *  Input:  id - task ID
 *  Output: none
 *  Return: none
 *  Description: Enable a previously disabled task.
 *============================================================================*/
void app_task_enable(uint8_t id)
{
    if (id < g_task_count)
    {
        g_task_table[id].enabled  = 1;
        g_task_table[id].last_run = HAL_GetTick();
    }
}

/*==============================================================================
 *  Function: void app_task_disable(uint8_t id)
 *  Input:  id - task ID
 *  Output: none
 *  Return: none
 *  Description: Disable a task (it will not run until re-enabled).
 *============================================================================*/
void app_task_disable(uint8_t id)
{
    if (id < g_task_count)
    {
        g_task_table[id].enabled = 0;
    }
}
