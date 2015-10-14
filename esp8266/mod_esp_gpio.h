/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Rob Fowler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _INCLUDED_MOD_ESP_GPIO_H_
#define _INCLUDED_MOD_ESP_GPIO_H_
// extern const mp_obj_type_t esp_os_timer_type;

typedef int (*isr_t)(void *, uint32_t now, uint8_t signal);

typedef struct {
    bool level;
    uint32_t period;
} event_t;

typedef struct {
    uint16_t pin;
    uint32_t periph;
    uint16_t func;
    isr_t isr;
    void *data;
    int last_result;
    event_t *events;
    uint16_t ecount;
    uint16_t n_events;
    uint32_t start_time;
} pmap_t;


extern pmap_t *pmap(int inpin);
extern void ICACHE_FLASH_ATTR esp_gpio_isr_attach(pmap_t *pmp, isr_t vect, void *data, uint16_t n_events);
extern bool ICACHE_FLASH_ATTR esp_gpio_isr_detach(uint8_t pin);
extern void ICACHE_FLASH_ATTR esp_gpio_init();

extern const mp_obj_module_t mp_module_esp_gpio;
#endif // _INCLUDED_MOD_ESP_GPIO_H_
