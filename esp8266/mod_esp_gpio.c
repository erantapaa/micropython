/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2014 Paul Sokolovsky
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

#include "ets_sys.h"
#include "osapi.h"
#include "uart.h"
#include "osapi.h"
#include "uart_register.h"
#include "etshal.h"
#include "c_types.h"
#include "user_interface.h"
#include "esp_mphal.h"



#define RAISE_ERRNO(err_flag, error_val) \
    { if (err_flag == -1) \
        { nlr_raise(mp_obj_new_exception_arg1(&mp_type_OSError, MP_OBJ_NEW_SMALL_INT(error_val))); } }

extern void ets_timer_disarm(ETSTimer *ptimer);
extern void ets_timer_setfn(ETSTimer *ptimer, ETSTimerFunc *pfunction, void *parg);
extern void ets_timer_arm_new(ETSTimer *ptimer,uint32_t milliseconds, bool repeat_flag, bool);
extern void ets_delay_us(int us);


typedef int (*isr_t)(void *);

static int def_called;


typedef struct {
    uint16_t pin;
    uint32_t periph;
    uint16_t func;
    isr_t isr;
    void *data;
    int def_count;
    bool state;
    int last_result;
} pmap_t;

STATIC pmap_t pin_map[] = {
    {0, PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0, NULL, NULL, 0, false, -1}, 
    {1, PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1, NULL, NULL, 0, false, -1},
    {2, PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2, NULL, NULL, 0, false, -1},
    {3, PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3, NULL, NULL, 0, false, -1},
    {4, PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4, NULL, NULL, 0, false, -1},
    {5, PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5, NULL, NULL, 0, false, -1},
//    {9, PERIPHS_IO_MUX_SD_DATA2_U, FUNC_GPIO9, NULL, NULL, 0, false, -1},
//    {10, PERIPHS_IO_MUX_SD_DATA3_U, FUNC_GPIO10, NULL, NULL, 0, false, -1},
    {12, PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12, NULL, NULL, 0, false, -1},
    {13, PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13, NULL, NULL, 0, false, -1},
    {14, PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14, NULL, NULL, 0, false, -1},
    {15, PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15, NULL, NULL, 0, false, -1}
};

#define NPINS (sizeof (pin_map) / sizeof (pmap_t))

STATIC pmap_t *pmap(int inpin)
{
    // could bsearch as it's sorted 
    pmap_t *pmi = pin_map;
    int ii = 0;
    for (; ii < NPINS; ii++, pmi++) {
        if (pmi->pin == inpin) {
            return pmi;
        }
    }
    nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "Invalid GPIO pin"));
}


void def_isr(void *arg)
{
    uint32_t intr_mask = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    pmap_t *pmi = pin_map;
    uint16_t bit = 1;
    for (int ii = 0; ii < NPINS; ii++, bit <<= 1, pmi++) {
        if (intr_mask & bit) {
            if (pmi->isr) {
                pmi->def_count++;
                pmi->last_result = pmi->isr(pmi->data);
                // gpio_intr_ack(bit);
                GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, bit);
            } else {
                def_called++;
            }
        }
    }
}


bool ICACHE_FLASH_ATTR isr_detach(uint8_t pin)
{
    pmap_t *pmp= pmap(pin);
    gpio_pin_intr_state_set(GPIO_ID_PIN(pmp->pin), GPIO_PIN_INTR_DISABLE);
    pmp->isr = NULL;
    pmp->data = NULL;      // TODO: what is sensible here if it is malloced in all cases?
    return true;
}

void ICACHE_FLASH_ATTR isr_attach(pmap_t *pmp, isr_t vect, void *data)
{
	ETS_GPIO_INTR_DISABLE();
    pmp->isr = vect;
    pmp->data = data;
    pmp->last_result = -1;

//	PIN_FUNC_SELECT(pmp->periph, pmp->func);

//    gpio_output_set(0, 0, 0, pp->pin_id);
    GPIO_DIS_OUTPUT(GPIO_ID_PIN(pmp->pin));
//    gpio_register_set(GPIO_PIN_ADDR(pmp->pin), GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE)
 //                   | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE)
  //                  | GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));

    // clear existing interrupt flag
    // GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pmp->pin));

    // enable interrupt
    gpio_pin_intr_state_set(GPIO_ID_PIN(pmp->pin), GPIO_PIN_INTR_NEGEDGE);  // TODO: pass Edge type
    ETS_GPIO_INTR_ATTACH(def_isr, NULL);
//    attachInterrupt(pmp->pin, def_isr, GPIO_PIN_INTR_NEGEDGE);
	ETS_GPIO_INTR_ENABLE();
}

static int xxx = 0;
static uint32 yyy = -1;

int ICACHE_FLASH_ATTR testx(void *arg)
{
    xxx += 1;
    yyy = (uint32)arg;
    return 0;
}

STATIC mp_obj_t esp_gpio_init(mp_obj_t self_in, mp_obj_t len_in) {
	
    mp_hal_stdout_tx_str("NEW module");
    pmap_t *pmp = pmap(4);
	PIN_FUNC_SELECT(pmp->periph, pmp->func);
    GPIO_DIS_OUTPUT(GPIO_ID_PIN(pmp->pin));
    PIN_PULLUP_EN(pmp->periph);
//    gpio_pin_intr_state_set(GPIO_ID_PIN(pmp->pin), GPIO_PIN_INTR_NEGEDGE);  // TODO: pass Edge type
 //   ETS_GPIO_INTR_ATTACH(testx, 1234);
//	ETS_GPIO_INTR_ENABLE();
    isr_attach(pmp, testx, NULL);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(esp_gpio_init_obj, esp_gpio_init);

STATIC mp_obj_t esp_gpio_print(mp_obj_t self_in, mp_obj_t len_in) {
    const pmap_t *pmi = pin_map;
    for (int pin = 0; pin < NPINS; pin++, pmi++) {
        printf("pin %d handler %x count = %d last = %d\n", pin, (unsigned int)pmi->isr, pmi->def_count, pmi->last_result);
    }
    printf("def called %d\n", def_called);
    printf("xxx  %d\n", xxx);
    printf("yyy  %d\n", yyy);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(esp_gpio_print_obj, esp_gpio_print);



STATIC mp_obj_t pyb_toggle(mp_obj_t usec_in) {
    mp_int_t gpio_pin = mp_obj_get_int(usec_in);
    pmap_t *pmp = pmap(gpio_pin);
    
    printf("togglin %d", pmp->pin);
//    GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, (1<<gpio_pin));
    if (pmp->state == 1) {
        GPIO_OUTPUT_SET(pmp->pin, 0);
        pmp->state = 0;
        printf("off\n");
    } else {
        GPIO_OUTPUT_SET(pmp->pin, 1);
        pmp->state = 1;
        printf("on\n");
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_toggle_obj, pyb_toggle);


STATIC const mp_map_elem_t mo_module_esp_gpio_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_esp_gpio) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_init), (mp_obj_t)&esp_gpio_init_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_print), (mp_obj_t)&esp_gpio_print_obj},
//    { MP_OBJ_NEW_QSTR(MP_QSTR_dht), (mp_obj_t)&dht_type },
    { MP_OBJ_NEW_QSTR(MP_QSTR_toggle), (mp_obj_t)&pyb_toggle_obj }
};

STATIC MP_DEFINE_CONST_DICT(mo_module_esp_gpio_globals, mo_module_esp_gpio_globals_table);

const mp_obj_module_t esp_gpio_module = {
    .base = { &mp_type_module },
    .name = MP_QSTR_esp_gpio,
    .globals = (mp_obj_dict_t*)&mo_module_esp_gpio_globals,
};
