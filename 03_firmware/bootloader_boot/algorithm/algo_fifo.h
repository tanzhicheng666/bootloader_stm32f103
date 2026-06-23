/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: algo_fifo.h
 *  Description: Algorithm layer — generic FIFO queue interface
 *               Circular buffer, single producer / single consumer safe
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __ALGO_FIFO_H
#define __ALGO_FIFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*==============================================================================
 * Type Definitions
 *============================================================================*/
typedef struct {
    uint8_t *buf;
    uint16_t size;
    uint16_t head;
    uint16_t tail;
} stc_fifo_t;

/*==============================================================================
 * Function Declarations
 *============================================================================*/
void     algo_fifo_init(stc_fifo_t *p_fifo, uint8_t *buf, uint16_t size);
uint8_t  algo_fifo_put(stc_fifo_t *p_fifo, uint8_t byte);
uint8_t  algo_fifo_get(stc_fifo_t *p_fifo, uint8_t *p_byte);
uint16_t algo_fifo_available(stc_fifo_t *p_fifo);
uint8_t  algo_fifo_is_empty(stc_fifo_t *p_fifo);
uint8_t  algo_fifo_is_full(stc_fifo_t *p_fifo);
void     algo_fifo_flush(stc_fifo_t *p_fifo);

#ifdef __cplusplus
}
#endif

#endif /* __ALGO_FIFO_H */

// end of file
