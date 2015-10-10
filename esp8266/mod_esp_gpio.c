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

#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "py/nlr.h"
#include "py/objtuple.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/builtin.h"

#include "esp_mphal.h"
#include "ets_sys.h"
#include "gpio.h"

#include "mem.h"
#include "ets_sys.h"
#include "osapi.h"
#include "uart.h"
#include "uart_register.h"
#include "etshal.h"
#include "c_types.h"
#include "user_interface.h"
#include "esp_mphal.h"
#include "os_type.h"
#include "utils.h"
#include "mod_esp_gpio.h"

extern void *pvPortMalloc(size_t xWantedSize);

#define TIME system_get_time

#define RAISE_ERRNO(err_flag, error_val) \
    { if (err_flag == -1) \
        { nlr_raise(mp_obj_new_exception_arg1(&mp_type_OSError, MP_OBJ_NEW_SMALL_INT(error_val))); } }


STATIC pmap_t pin_map[] = {
    {0, PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0, NULL, NULL, 0, NULL, 0, 0, -1}, 
    {1, PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1, NULL, NULL, 0, NULL, 0, 0, -1},
    {2, PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2, NULL, NULL, 0, NULL, 0, 0, -1},
    {3, PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3, NULL, NULL, 0, NULL, 0, 0, -1},
    {4, PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4, NULL, NULL, 0, NULL, 0, 0, -1},
    {5, PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5, NULL, NULL, 0, NULL, 0, 0, -1},
//    {9, PERIPHS_IO_MUX_SD_DATA2_U, FUNC_GPIO9, NULL, NULL},
//    {10, PERIPHS_IO_MUX_SD_DATA3_U, FUNC_GPIO10, NULL, NULL},
    {12, PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12, NULL, NULL, 0, NULL, 0, 0, -1},
    {13, PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13, NULL, NULL, 0, NULL, 0, 0, -1},
    {14, PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14, NULL, NULL, 0, NULL, 0, 0, -1},
    {15, PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15, NULL, NULL, 0, NULL, 0, 0, -1}
};

#define NPINS (sizeof (pin_map) / sizeof (pmap_t))

pmap_t *pmap(int inpin)
{
    // could bsearch as it's not sorted (and small)
    // I'll put the most often used pins at the top
    pmap_t *pmi = pin_map;
    int ii = 0;
    for (; ii < NPINS; ii++, pmi++) {
        if (pmi->pin == inpin) {
            return pmi;
        }
    }
    nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Invalid GPIO pin"));
}


STATIC void def_isr(void *arg)
{
    uint32_t intr_mask = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    uint32_t now = TIME();
    pmap_t *pmi = pin_map;
    for (int ii = 0; ii < NPINS; ii++, pmi++) {
        uint8_t bit = BIT(pmi->pin);
        if (intr_mask & bit) {
            if (pmi->isr) {
                uint8_t signal = GPIO_INPUT_GET(pmi->pin);
                if (pmi->ecount < pmi->n_events) {
                    pmi->events[pmi->ecount].period = now - pmi->start_time;
                    pmi->events[pmi->ecount].level = signal ^ 1;
                    pmi->ecount++;
                }
                pmi->start_time = now;
                pmi->last_result = pmi->isr(pmi->data, now, signal);
                // gpio_intr_ack(bit);
                GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, bit);
            } 
        }
    }
}

void esp_gpio_init() {
    ETS_GPIO_INTR_ATTACH(def_isr, NULL);
    printf("mod_esp_gpio\n");
}

STATIC mp_obj_t ICACHE_FLASH_ATTR esp_gpio_print(mp_obj_t self_in, mp_obj_t len_in) {
    const pmap_t *pmi = pin_map;
    for (int pin = 0; pin < NPINS; pin++, pmi++) {
        printf("pin %d handler %x last = %d\n", pin, (unsigned int)pmi->isr, pmi->last_result);
        for (int ii = 0; ii < pmi->ecount; ii++) {
            if (pmi->events[ii].level)
                printf("%u,%u,%d\n", pmi->events[ii].period, pmi->events[ii].level, pmi->events[ii].period > 30 ? 1 : 0);
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(esp_gpio_print_obj, esp_gpio_print);




bool ICACHE_FLASH_ATTR esp_gpio_isr_detach(uint8_t pin)
{
    pmap_t *pmp = pmap(pin);
    gpio_pin_intr_state_set(GPIO_ID_PIN(pmp->pin), GPIO_PIN_INTR_DISABLE);
    pmp->isr = NULL;
    pmp->data = NULL;      // TODO: what is sensible here if it is malloced in all cases?
    return true;
}

void ICACHE_FLASH_ATTR esp_gpio_isr_attach(pmap_t *pmp, isr_t vect, void *data, uint16_t n_events)
{
	ETS_GPIO_INTR_DISABLE();
    pmp->isr = vect;
    pmp->data = data;
    pmp->last_result = -1;
    if (n_events) {
        if (!pmp->events) {
            // os_free(pmp->events);
            pmp->events = (event_t *)os_malloc(n_events * sizeof (event_t));
            pmp->n_events = n_events;
        } else if (pmp->n_events != n_events) {
            printf("not new and a new event count. Keeping old size\n");
        }
        pmp->ecount = 0;
    }
    pmp->start_time = TIME();
	ETS_GPIO_INTR_ENABLE();
}


STATIC const mp_map_elem_t mo_module_esp_gpio_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_gpio) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_print), (mp_obj_t)&esp_gpio_print_obj},
};

STATIC MP_DEFINE_CONST_DICT(mo_module_esp_gpio_globals, mo_module_esp_gpio_globals_table);

const mp_obj_module_t mp_module_esp_gpio = {
    .base = { &mp_type_module },
    .name = MP_QSTR_gpio,
    .globals = (mp_obj_dict_t*)&mo_module_esp_gpio_globals,
};
