/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: app_task.h
 *  Description: Application layer — simple task scheduler interface
 *               Callback-based periodic task execution, time base from bsp_tick
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __APP_TASK_H
#define __APP_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*==============================================================================
 * Constants
 *============================================================================*/
#define APP_TASK_MAX_NUM    8U

/*==============================================================================
 * Task Callback Type
 *============================================================================*/
typedef void (*app_task_cb_t)(void);

/*==============================================================================
 * Function Declarations
 *============================================================================*/
void    app_task_init(void);
int8_t  app_task_create(app_task_cb_t callback, uint32_t interval_ms);
void    app_task_run(void);
void    app_task_enable(uint8_t id);
void    app_task_disable(uint8_t id);

#ifdef __cplusplus
}
#endif

#endif /* __APP_TASK_H */

// end of file
