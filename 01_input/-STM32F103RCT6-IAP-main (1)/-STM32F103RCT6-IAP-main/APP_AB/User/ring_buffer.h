#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ------------------------------------------------------------------ */
/*  配置区：按需修改                                                    */
/* ------------------------------------------------------------------ */

/* 缓冲区存储的元素类型，改这里即可适配不同场景 */
typedef uint8_t rb_elem_t;

/* ------------------------------------------------------------------ */
/*  错误码                                                              */
/* ------------------------------------------------------------------ */

typedef enum {
    RB_OK       =  0,   /* 操作成功          */
    RB_EMPTY    = -1,   /* 缓冲区为空        */
    RB_FULL     = -2,   /* 缓冲区已满        */
    RB_INVAL    = -3,   /* 非法参数（空指针）*/
} rb_err_t;

/* ------------------------------------------------------------------ */
/*  控制块                                                              */
/* ------------------------------------------------------------------ */

/*
 * 设计决策：容量必须是 2 的幂次
 * 原因：取模运算 (index % capacity) 可以用位与替代 (index & mask)
 *       避免除法，MCU 上性能提升明显
 * 约束：调用者需保证传入的 buf 长度是 2 的幂次
 */
typedef struct {
    rb_elem_t  *buf;      /* 指向外部提供的存储区        */
    uint32_t    capacity; /* 缓冲区容量（必须是 2 的幂） */
    uint32_t    mask;     /* capacity - 1，用于快速取模  */
    uint32_t    head;     /* 写指针（下一个写入位置）    */
    uint32_t    tail;     /* 读指针（下一个读取位置）    */
} ring_buffer_t;

/* ------------------------------------------------------------------ */
/*  API                                                                 */
/* ------------------------------------------------------------------ */

/**
 * @brief  初始化环形缓冲区
 * @param  rb       控制块指针
 * @param  buf      外部提供的存储数组（长度必须是 2 的幂）
 * @param  capacity 存储数组的长度
 * @return RB_OK / RB_INVAL
 */
rb_err_t rb_init(ring_buffer_t *rb, rb_elem_t *buf, uint32_t capacity);

/**
 * @brief  写入单个元素
 * @return RB_OK / RB_FULL / RB_INVAL
 */
rb_err_t rb_put(ring_buffer_t *rb, rb_elem_t data);

/**
 * @brief  读取单个元素（消费）
 * @return RB_OK / RB_EMPTY / RB_INVAL
 */
rb_err_t rb_get(ring_buffer_t *rb, rb_elem_t *data);

/**
* @brief  读取单个元素（不消费）
 * @return false	/	true
 */
bool rb_peek(ring_buffer_t *rb, uint8_t *out);

/**
 * @brief  批量写入
 * @param  src  源数组
 * @param  len  要写入的元素个数
 * @return 实际写入的元素个数
 */
uint32_t rb_write(ring_buffer_t *rb, const rb_elem_t *src, uint32_t len);

/**
 * @brief  批量读取
 * @param  dst  目标数组
 * @param  len  最多读取的元素个数
 * @return 实际读取的元素个数
 */
uint32_t rb_read(ring_buffer_t *rb, rb_elem_t *dst, uint32_t len);

uint32_t rb_peeks(const ring_buffer_t *rb, rb_elem_t *dst, uint32_t len);

/**
 * @brief  查询当前存储的元素个数
 */
uint32_t rb_size(const ring_buffer_t *rb);

/**
 * @brief  查询剩余可写空间
 */
uint32_t rb_free(const ring_buffer_t *rb);

/**
 * @brief  是否为空
 */
bool rb_is_empty(const ring_buffer_t *rb);

/**
 * @brief  是否已满
 */
bool rb_is_full(const ring_buffer_t *rb);

/**
 * @brief  清空缓冲区（重置读写指针，不清存储区数据）
 */
rb_err_t rb_flush(ring_buffer_t *rb);

#endif /* RING_BUFFER_H */
