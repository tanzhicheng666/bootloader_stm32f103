/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_uart.c
 *  Description: BSP UART driver — USART2 interrupt-based TX/RX
 *               PA2=TX(AF push-pull), PA3=RX(input pull-up)
 *               RX ring buffer (256B), TX ring buffer (256B)
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "bsp_uart.h"
#include "stm32f1xx_hal.h"

/*==============================================================================
 * Local Constants
 *============================================================================*/
#define BSP_UART_INSTANCE           USART2
#define BSP_UART_BAUDRATE_DEFAULT   115200U

#define BSP_UART_TX_PORT            GPIOA
#define BSP_UART_TX_PIN             GPIO_PIN_2
#define BSP_UART_RX_PORT            GPIOA
#define BSP_UART_RX_PIN             GPIO_PIN_3

#define BSP_UART_RX_BUF_SIZE        256U
#define BSP_UART_TX_BUF_SIZE        256U
#define BSP_UART_BUF_MASK           (BSP_UART_RX_BUF_SIZE - 1U)  /* both same size */

/*==============================================================================
 * Local Variables
 *============================================================================*/
UART_HandleTypeDef g_uart_handle;

/* RX ring buffer */
static uint8_t  g_rx_buf[BSP_UART_RX_BUF_SIZE];
static volatile uint16_t g_rx_head;
static volatile uint16_t g_rx_tail;
static volatile uint8_t  g_rx_byte;

/* TX ring buffer (interrupt-based) */
static uint8_t  g_tx_buf[BSP_UART_TX_BUF_SIZE];
static volatile uint16_t g_tx_head;
static volatile uint16_t g_tx_tail;
static volatile uint8_t  g_tx_busy;

/*==============================================================================
 *  Internal: start next TX byte from ring buffer
 *============================================================================*/
static void bsp_uart_tx_start(void)
{
    uint16_t tail;

    if (g_tx_head != g_tx_tail) {
        tail = g_tx_tail;
        g_tx_busy = 1;
        HAL_UART_Transmit_IT(&g_uart_handle, &g_tx_buf[tail], 1);
    }
}

/*==============================================================================
 *  Function: void bsp_uart_init(uint32_t baudrate)
 *  Input:  baudrate - desired baud rate (e.g. 115200)
 *  Description: Initialize USART2. PA2=TX, PA3=RX.
 *               Starts interrupt-based reception of the first byte.
 *============================================================================*/
void bsp_uart_init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio_init = {0};

    if (baudrate == 0) {
        baudrate = BSP_UART_BAUDRATE_DEFAULT;
    }

    /* Enable clocks */
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* TX pin: PA2, Alternate Function Push-Pull */
    gpio_init.Pin       = BSP_UART_TX_PIN;
    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BSP_UART_TX_PORT, &gpio_init);

    /* RX pin: PA3, Input with pull-up */
    gpio_init.Pin       = BSP_UART_RX_PIN;
    gpio_init.Mode      = GPIO_MODE_INPUT;
    gpio_init.Pull      = GPIO_PULLUP;
    HAL_GPIO_Init(BSP_UART_RX_PORT, &gpio_init);

    /* Configure USART2 */
    g_uart_handle.Instance            = BSP_UART_INSTANCE;
    g_uart_handle.Init.BaudRate       = baudrate;
    g_uart_handle.Init.WordLength     = UART_WORDLENGTH_8B;
    g_uart_handle.Init.StopBits       = UART_STOPBITS_1;
    g_uart_handle.Init.Parity         = UART_PARITY_NONE;
    g_uart_handle.Init.Mode           = UART_MODE_TX_RX;
    g_uart_handle.Init.HwFlowCtl      = UART_HWCONTROL_NONE;
    g_uart_handle.Init.OverSampling   = UART_OVERSAMPLING_16;
    HAL_UART_Init(&g_uart_handle);

    /* Enable USART2 interrupt in NVIC */
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    /* Start reception of the first byte */
    HAL_UART_Receive_IT(&g_uart_handle, (uint8_t *)&g_rx_byte, 1);
}

/*==============================================================================
 *  Function: void bsp_uart_send(uint8_t *p_data, uint16_t len)
 *  Description: Send buffer via interrupt-based TX ring buffer.
 *               Returns immediately; data sent in background.
 *============================================================================*/
void bsp_uart_send(uint8_t *p_data, uint16_t len)
{
    uint16_t i;
    uint16_t next_head;

    if (p_data == (uint8_t *)0 || len == 0) {
        return;
    }

    for (i = 0; i < len; i++) {
        next_head = (g_tx_head + 1) & BSP_UART_BUF_MASK;
        while (next_head == g_tx_tail) {
            /* TX buffer full — spin-wait for space */
        }
        g_tx_buf[g_tx_head] = p_data[i];
        g_tx_head = next_head;
    }

    if (g_tx_busy == 0) {
        bsp_uart_tx_start();
    }
}

/*==============================================================================
 *  Function: void bsp_uart_send_byte(uint8_t byte)
 *  Description: Send a single byte. Non-blocking.
 *============================================================================*/
void bsp_uart_send_byte(uint8_t byte)
{
    uint16_t next_head;

    next_head = (g_tx_head + 1) & BSP_UART_BUF_MASK;
    while (next_head == g_tx_tail) {
    }
    g_tx_buf[g_tx_head] = byte;
    g_tx_head = next_head;

    if (g_tx_busy == 0) {
        bsp_uart_tx_start();
    }
}

/*==============================================================================
 *  Function: uint16_t bsp_uart_available(void)
 *  Return: number of bytes available in RX ring buffer
 *============================================================================*/
uint16_t bsp_uart_available(void)
{
    uint16_t head = g_rx_head;
    uint16_t tail = g_rx_tail;
    return (head - tail) & BSP_UART_BUF_MASK;
}


/*==============================================================================
 *  Function: uint8_t bsp_uart_read(void)
 *  Return: next byte from ring buffer (0 if empty)
 *  Description: Call bsp_uart_available() first to check for data.
 *============================================================================*/
uint8_t bsp_uart_read(void)
{
    uint8_t byte = 0;
    uint16_t tail;

    if (g_rx_head != g_rx_tail) {
        tail = g_rx_tail;
        byte = g_rx_buf[tail];
        g_rx_tail = (tail + 1) & BSP_UART_BUF_MASK;
    }
    return byte;
}

/*==============================================================================
 * HAL UART Callback Overrides
 *============================================================================*/

/*
 *  Function: void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
 *  Description: Called by HAL when a byte is received.
 *               Stores byte in RX ring buffer, restarts reception.
*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint16_t next_head;

    if (huart->Instance != BSP_UART_INSTANCE) {
        return;
    }

    next_head = (g_rx_head + 1) & BSP_UART_BUF_MASK;
    if (next_head != g_rx_tail) {
        g_rx_buf[g_rx_head] = g_rx_byte;
        g_rx_head = next_head;
    }
    /* else: buffer full — byte discarded */

    HAL_UART_Receive_IT(&g_uart_handle, (uint8_t *)&g_rx_byte, 1);
}

/*
 *  Function: void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
 *  Description: Called when a byte transmission completes.
 *               Advances TX tail, starts next byte if available.
*/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != BSP_UART_INSTANCE) {
        return;
    }

    g_tx_tail = (g_tx_tail + 1) & BSP_UART_BUF_MASK;

    if (g_tx_head != g_tx_tail) {
        HAL_UART_Transmit_IT(&g_uart_handle, &g_tx_buf[g_tx_tail], 1);
    }
    else {
        g_tx_busy = 0;
    }
}

/*
 *  Function: void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
 *  Description: Called on UART errors. Clears flags and restarts reception
 *               to prevent the UART from hanging.
*/
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != BSP_UART_INSTANCE) {
        return;
    }

    /* Read SR then DR to clear error flags (RM0008 mandatory sequence) */
    (void)huart->Instance->SR;
    (void)huart->Instance->DR;
    __HAL_UART_CLEAR_OREFLAG(huart);

    HAL_UART_Receive_IT(&g_uart_handle, (uint8_t *)&g_rx_byte, 1);
}

// end of file
