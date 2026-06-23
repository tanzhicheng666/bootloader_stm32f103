#include "ring_buffer.h"

/* ------------------------------------------------------------------ */
/*  内部工具                                                            */
/* ------------------------------------------------------------------ */

/**
 * 判断一个数是否是 2 的幂次
 * 经典位运算技巧：2^n 的二进制只有一位是 1，减 1 后所有低位变 1，AND 结果为 0
 * 例：8 = 1000b，7 = 0111b，8 & 7 = 0
 */
static inline bool is_power_of_two(uint32_t n)
{
    return (n > 0) && ((n & (n - 1)) == 0);
}

/* ------------------------------------------------------------------ */
/*  初始化                                                              */
/* ------------------------------------------------------------------ */

rb_err_t rb_init(ring_buffer_t *rb, rb_elem_t *buf, uint32_t capacity)
{
    if (rb == NULL || buf == NULL) {
        return RB_INVAL;
    }
    if (!is_power_of_two(capacity)) {
        return RB_INVAL;
    }

    rb->buf      = buf;
    rb->capacity = capacity;
    rb->mask     = capacity - 1;
    rb->head     = 0;
    rb->tail     = 0;

    return RB_OK;
}

/* ------------------------------------------------------------------ */
/*  状态查询                                                            */
/* ------------------------------------------------------------------ */

/*
 * 核心设计：head 和 tail 永远单调递增，不对 capacity 取模
 * 真实的存储位置 = index & mask
 *
 * 好处：
 *   1. size = head - tail，永远正确，不需要区分"满"和"空"的边界情况
 *   2. 满的判断是 size == capacity，空的判断是 head == tail，逻辑清晰
 *   3. 即使溢出（uint32_t 回绕），head - tail 依然正确（无符号运算性质）
 */

uint32_t rb_size(const ring_buffer_t *rb)
{
    return rb->head - rb->tail;
}

uint32_t rb_free(const ring_buffer_t *rb)
{
    return rb->capacity - rb_size(rb);
}

bool rb_is_empty(const ring_buffer_t *rb)
{
    return rb->head == rb->tail;
}

bool rb_is_full(const ring_buffer_t *rb)
{
    return rb_size(rb) == rb->capacity;
}

/* ------------------------------------------------------------------ */
/*  单元素读写                                                          */
/* ------------------------------------------------------------------ */

rb_err_t rb_put(ring_buffer_t *rb, rb_elem_t data)
{
    if (rb == NULL) return RB_INVAL;
    if (rb_is_full(rb)) return RB_FULL;

    rb->buf[rb->head & rb->mask] = data;

    /*
     * 写屏障（memory barrier）说明：
     * 在 ARM Cortex-M 的实际使用中，如果 put 在中断里调用，
     * 需要在这里加 __DMB()，确保数据写入 buf 后 head 才递增，
     * 防止编译器或 CPU 乱序。此处为简洁省略，移植时按需添加。
     */
    rb->head++;

    return RB_OK;
}

rb_err_t rb_get(ring_buffer_t *rb, rb_elem_t *data)
{
    if (rb == NULL || data == NULL) return RB_INVAL;
    if (rb_is_empty(rb)) return RB_EMPTY;

    *data = rb->buf[rb->tail & rb->mask];
    rb->tail++;

    return RB_OK;
}

bool rb_peek(ring_buffer_t *rb, uint8_t *out) {
    if (rb_is_empty(rb)) return false;
    *out = rb->buf[rb->tail & rb->mask]; // 不移动 read_idx
    return true;
}

/* ------------------------------------------------------------------ */
/*  批量读写                                                            */
/* ------------------------------------------------------------------ */

uint32_t rb_write(ring_buffer_t *rb, const rb_elem_t *src, uint32_t len)
{
    if (rb == NULL || src == NULL) return 0;

    /* 实际能写的数量不超过剩余空间 */
    uint32_t space = rb_free(rb);
    if (len > space) {
        len = space;
    }

    /*
     * 环形缓冲区的数据在物理存储上可能被"切成两段"：
     *
     *   capacity = 8，head & mask = 6，len = 4
     *   物理布局：[ _ _ _ _ _ _ W W | W W _ _ _ _ _ _ ]
     *                                ^wrap
     *   第一段：buf[6], buf[7]  （到末尾，共 2 个）
     *   第二段：buf[0], buf[1]  （从头开始，共 2 个）
     *
     * 所以分两次拷贝：
     */
    uint32_t head_pos  = rb->head & rb->mask;          /* 当前写位置     */
    uint32_t to_end    = rb->capacity - head_pos;       /* 到末尾的空间   */
    uint32_t first     = (len < to_end) ? len : to_end; /* 第一段长度     */
    uint32_t second    = len - first;                    /* 第二段长度     */

    /* 第一段拷贝 */
    for (uint32_t i = 0; i < first; i++) {
        rb->buf[head_pos + i] = src[i];
    }
    /* 第二段拷贝（如果有折回） */
    for (uint32_t i = 0; i < second; i++) {
        rb->buf[i] = src[first + i];
    }

    rb->head += len;
    return len;
}

uint32_t rb_read(ring_buffer_t *rb, rb_elem_t *dst, uint32_t len)
{
    if (rb == NULL || dst == NULL) return 0;

    /* 实际能读的数量不超过已有数据 */
    uint32_t avail = rb_size(rb);
    if (len > avail) {
        len = avail;
    }

    /* 同理，读取也可能折回 */
    uint32_t tail_pos = rb->tail & rb->mask;
    uint32_t to_end   = rb->capacity - tail_pos;
    uint32_t first    = (len < to_end) ? len : to_end;
    uint32_t second   = len - first;

    for (uint32_t i = 0; i < first; i++) {
        dst[i] = rb->buf[tail_pos + i];
    }
    for (uint32_t i = 0; i < second; i++) {
        dst[first + i] = rb->buf[i];
    }

    rb->tail += len;
    return len;
}

/**
 * @brief  批量窥视（偷看）缓冲区的数据，但不消费（不移动 tail 指针）
 * @param  rb:  环形缓冲区控制块指针
 * @param  dst: 接收偷看数据的目标数组
 * @param  len: 想要偷看的字节数（比如 12 字节）
 * @return uint32_t: 实际成功偷看出来的有效字节数
 */
uint32_t rb_peeks(const ring_buffer_t *rb, rb_elem_t *dst, uint32_t len)
{
    if (rb == NULL || dst == NULL || len == 0) return 0;

    /* 缓冲区里有多少就最多看多少 */
    uint32_t avail = rb_size(rb);
    if (len > avail) {
        len = avail;
    }

    /* * 核心优势：直接利用现成的物理两段式位置计算 
     * 注意：这里使用的是物理基准位置 tail_pos，但绝不修改 rb->tail 本身！
     */
    uint32_t tail_pos = rb->tail & rb->mask;
    uint32_t to_end   = rb->capacity - tail_pos;
    uint32_t first    = (len < to_end) ? len : to_end; /* 第一段（到缓冲区末尾） */
    uint32_t second   = len - first;                    /* 第二段（从缓冲区开头折回） */

    /* 批量高速内存拷贝（单片机上可用 memcpy 代替，这里用你原版的一致性 for） */
    for (uint32_t i = 0; i < first; i++) {
        dst[i] = rb->buf[tail_pos + i];
    }
    for (uint32_t i = 0; i < second; i++) {
        dst[first + i] = rb->buf[i];
    }

    return len; /* 返回实际可以看齐的长度 */
}

/* ------------------------------------------------------------------ */
/*  其他                                                                */
/* ------------------------------------------------------------------ */

rb_err_t rb_flush(ring_buffer_t *rb)
{
    if (rb == NULL) return RB_INVAL;
    rb->head = 0;
    rb->tail = 0;
    return RB_OK;
}
