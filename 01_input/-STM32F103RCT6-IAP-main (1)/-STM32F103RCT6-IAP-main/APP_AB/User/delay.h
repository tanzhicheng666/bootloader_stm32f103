#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t start;
    uint32_t period;
    bool active;
} delay_t;

void delay_init(delay_t *d);
void delay_start(delay_t *d, uint32_t ms);
bool delay_ready(delay_t *d);
void delay_stop(delay_t *d);
bool delay_active(delay_t *d);
uint32_t delay_remaining(delay_t *d);

#endif

