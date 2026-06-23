#include "delay.h"
#include "stm32f1xx_hal.h"

void delay_init(delay_t *d) {
    if (d) {
        d->start = 0;
        d->period = 0;
        d->active = false;
    }
}

void delay_start(delay_t *d, uint32_t ms) {
    if (d) {
        d->start = HAL_GetTick();
        d->period = ms;
        d->active = true;
    }
}

bool delay_ready(delay_t *d) {
    if (!d || !d->active) return true;
    
    if ((HAL_GetTick() - d->start) >= d->period) {
        d->active = false;
        return true;
    }
    return false;
}

void delay_stop(delay_t *d) {
    if (d) {
        d->active = false;
    }
}

bool delay_active(delay_t *d) {
    return d ? d->active : false;
}

uint32_t delay_remaining(delay_t *d) {
    if (!d || !d->active) return 0;
    
    uint32_t elapsed = HAL_GetTick() - d->start;
    return (elapsed >= d->period) ? 0 : (d->period - elapsed);
}
