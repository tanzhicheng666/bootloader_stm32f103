/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: main.h
 *  Description: Master header — includes all project headers
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __MAIN_H
#define __MAIN_H

/* SDK */
#include "stm32f1xx_hal.h"

/* Common */
#include "common_types.h"
#include "common_defines.h"

/* BSP */
#include "bsp_sysclk.h"
#include "bsp_tick.h"
#include "bsp_gpio.h"
#include "bsp_uart.h"
#include "bsp_flash.h"

/* App */
#include "app_task.h"
#include "app_button.h"
#include "app_comm.h"
#include "app_boot.h"
#include "app_cmd.h"

/* Algorithm */
#include "algo_crc.h"

#endif /* __MAIN_H */

// end of file
