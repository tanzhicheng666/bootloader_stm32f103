/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: algo_fifo.c
 *  Description: Algorithm layer — generic FIFO queue implementation
 *               Circular buffer, requires power-of-2 size for mask indexing
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "algo_fifo.h"

/*==============================================================================
 *  Function: void algo_fifo_init(stc_fifo_t *p_fifo, uint8_t *buf, uint16_t size)
 *  Description: Initialize a FIFO instance with external buffer.
 *               size must be power of 2 for efficient mask indexing.
 *============================================================================*/
void algo_fifo_init(stc_fifo_t *p_fifo, uint8_t *buf, uint16_t size)
{
    if (p_fifo == (stc_fifo_t *)0 || buf == (uint8_t *)0) {
        return;
    }

    p_fifo->buf  = buf;
    p_fifo->size = size;
    p_fifo->head = 0;
    p_fifo->tail = 0;
}

/*==============================================================================
 *  Function: uint8_t algo_fifo_put(stc_fifo_t *p_fifo, uint8_t byte)
 *  Return: 1 = success, 0 = FIFO full
 *============================================================================*/
uint8_t algo_fifo_put(stc_fifo_t *p_fifo, uint8_t byte)
{
    uint16_t next_head;

    if (p_fifo == (stc_fifo_t *)0) {
        return 0;
    }

    next_head = (p_fifo->head + 1) & (p_fifo->size - 1);
    if (next_head == p_fifo->tail) {
        return 0;   /* full */
    }

    p_fifo->buf[p_fifo->head] = byte;
    p_fifo->head = next_head;
    return 1;
}

/*==============================================================================
 *  Function: uint8_t algo_fifo_get(stc_fifo_t *p_fifo, uint8_t *p_byte)
 *  Return: 1 = success, 0 = FIFO empty
 *============================================================================*/
uint8_t algo_fifo_get(stc_fifo_t *p_fifo, uint8_t *p_byte)
{
    if (p_fifo == (stc_fifo_t *)0 || p_byte == (uint8_t *)0) {
        return 0;
    }

    if (p_fifo->head == p_fifo->tail) {
        return 0;   /* empty */
    }

    *p_byte = p_fifo->buf[p_fifo->tail];
    p_fifo->tail = (p_fifo->tail + 1) & (p_fifo->size - 1);
    return 1;
}

/*==============================================================================
 *  Function: uint16_t algo_fifo_available(stc_fifo_t *p_fifo)
 *  Return: number of bytes available to read
 *============================================================================*/
uint16_t algo_fifo_available(stc_fifo_t *p_fifo)
{
    if (p_fifo == (stc_fifo_t *)0) {
        return 0;
    }

    return (p_fifo->head - p_fifo->tail) & (p_fifo->size - 1);
}

/*==============================================================================
 *  Function: uint8_t algo_fifo_is_empty(stc_fifo_t *p_fifo)
 *============================================================================*/
uint8_t algo_fifo_is_empty(stc_fifo_t *p_fifo)
{
    if (p_fifo == (stc_fifo_t *)0) {
        return 1;
    }

    return (p_fifo->head == p_fifo->tail) ? 1 : 0;
}

/*==============================================================================
 *  Function: uint8_t algo_fifo_is_full(stc_fifo_t *p_fifo)
 *============================================================================*/
uint8_t algo_fifo_is_full(stc_fifo_t *p_fifo)
{
    uint16_t next_head;

    if (p_fifo == (stc_fifo_t *)0) {
        return 1;
    }

    next_head = (p_fifo->head + 1) & (p_fifo->size - 1);
    return (next_head == p_fifo->tail) ? 1 : 0;
}

/*==============================================================================
 *  Function: void algo_fifo_flush(stc_fifo_t *p_fifo)
 *  Description: Clear FIFO (reset head/tail to zero).
 *============================================================================*/
void algo_fifo_flush(stc_fifo_t *p_fifo)
{
    if (p_fifo == (stc_fifo_t *)0) {
        return;
    }

    p_fifo->head = 0;
    p_fifo->tail = 0;
}

// end of file
