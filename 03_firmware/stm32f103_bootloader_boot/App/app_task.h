/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: app_task.h
 *  Description: Application layer - simple task scheduler using callbacks and timer
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.20            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __APP_TASK_H
#define __APP_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

/*==============================================================================
 * Constants
 *============================================================================*/
#define APP_TASK_MAX_NUM    8U

/*==============================================================================
 * Task Callback Type
 *============================================================================*/
typedef void (*app_task_cb_t)(void);

/*==============================================================================
 * Task Structure
 *============================================================================*/
typedef struct {
    app_task_cb_t callback;
    uint32_t      interval_ms;
    uint32_t      last_run;
    uint8_t       enabled;
} app_task_t;

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
