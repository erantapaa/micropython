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

#include "mem.h"
#include "ets_sys.h"
#include "osapi.h"
#include "uart.h"
#include "uart_register.h"
#include "etshal.h"
#include "c_types.h"
#include "user_interface.h"
#include "esp_mphal.h"

#include "utils.h"
#include "mod_esp_gpio.h"
#include "mod_esp_dht.h"



#define TIME system_get_time

#define RAISE_ERRNO(err_flag, error_val) \
    { if (err_flag == -1) \
        { nlr_raise(mp_obj_new_exception_arg1(&mp_type_OSError, MP_OBJ_NEW_SMALL_INT(error_val))); } }

extern void ets_timer_disarm(ETSTimer *ptimer);
extern void ets_timer_setfn(ETSTimer *ptimer, ETSTimerFunc *pfunction, void *parg);
extern void ets_timer_arm_new(ETSTimer *ptimer,uint32_t milliseconds, bool repeat_flag, bool);

extern void ets_delay_us(int us);
extern unsigned xthal_get_ccount(void);
extern unsigned ets_get_cpu_frequency();



const mp_obj_type_t esp_dht_type;

#define DHT_BYTES   5

typedef struct _mp_obj_dht_t {
    mp_obj_base_t base;
    pmap_t *pmp;
    int state;
    char *error;
    os_timer_t timer;
    uint32_t start;
    uint32_t inters;
    uint32_t last_read_time;
    uint16_t bits;
    uint8_t bytes[DHT_BYTES];
    uint8_t current[DHT_BYTES];
    mp_obj_t callback;
    mp_obj_t callback_arg;
} mp_obj_dht_t;

//TODO make a class to pass the final result

#define DHT_ERROR -1
#define DHT_STATE_NEW 0
#define DHT_STATE_HPL 1
#define DHT_STATE_HPH 2
#define DHT_STATE_SPL 3
#define DHT_STATE_SPH 4
#define DHT_STATE_SDT 5
#define DHT_STATE_DATA_START 6
#define DHT_STATE_DATA_DATA 7
#define DHT_STATE_ENDED 8

#define LOW 0
#define HIGH 1

STATIC void    ICACHE_FLASH_ATTR dhtx_clear(mp_obj_dht_t *self) {
    self->inters = 0;
    self->bits = 0;
    self->state = DHT_STATE_NEW;
    self->error = "None";
    self->last_read_time = 0;
    memset(self->bytes, 0, DHT_BYTES);
}

STATIC int dhtx(void *args, uint32_t now, uint8_t signal);

STATIC mp_obj_dht_t ICACHE_FLASH_ATTR *dht_new(pmap_t *pmp) {
    mp_obj_dht_t *dhto = m_new_obj(mp_obj_dht_t);
    dhto->base.type = &esp_dht_type;
    dhto->pmp = pmp;
    dhtx_clear(dhto);
    esp_gpio_isr_attach(pmp, dhtx, dhto, 50);
    return dhto;
}

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0)

STATIC void ICACHE_FLASH_ATTR dht_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    mp_obj_dht_t *self = self_in;
        mp_printf(print, "<_dht %d %s inters %d bits %d callback %x>\n",
            self->state,
            self->error,
            self->inters,
            self->bits,
            (unsigned)self->callback);
    for (int ii = 0; ii < DHT_BYTES; ii++) {
        printf("%d,\'%2x\',%u,\'"BYTETOBINARYPATTERN"\'\n", ii, self->bytes[ii], self->bytes[ii], BYTETOBINARY(self->bytes[ii]));
    }
    printf("%d %d %d = %d", self->bytes[0] * 256 + self->bytes[1],
                    self->bytes[2] * 256 + self->bytes[3],
                    (self->bytes[0] + self->bytes[1] + self->bytes[2] + self->bytes[3]) & 0xFF, self->bytes[4]);
}

STATIC const mp_arg_t esp_dht_init_args[] = {
    { MP_QSTR_callback,    MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_arg,         MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} }
};
#define ESP_DHT_INIT_NUM_ARGS MP_ARRAY_SIZE(esp_dht_init_args)

STATIC mp_obj_dht_t *esp_dht_init_helper(mp_obj_dht_t *self, uint n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    mp_arg_val_t vals[ESP_DHT_INIT_NUM_ARGS];
    mp_arg_parse_all(n_args, args, kw_args, ESP_DHT_INIT_NUM_ARGS, esp_dht_init_args, vals);
    self->callback = vals[0].u_obj;
    self->callback_arg = vals[1].u_obj;
    return self;
}


// takes (pin=x, callback=yyy)
STATIC mp_obj_t ICACHE_FLASH_ATTR dht_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    int gpio_pin = mp_obj_get_int(args[0]);
    pmap_t *pmp = pmap(gpio_pin);
    mp_obj_dht_t *self  =  dht_new(pmp);
    self->callback = NULL;
    self->callback_arg = NULL;
    memset(self->current, 0, DHT_BYTES);
    if (n_args > 1 || n_kw > 0) {
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        self = esp_dht_init_helper(self, n_args - 1, args + 1, &kw_args);
    }
    GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, (1<<gpio_pin));
    gpio_pin_intr_state_set(GPIO_ID_PIN(pmp->pin), GPIO_PIN_INTR_ANYEDGE);  
    GPIO_OUTPUT_SET(pmp->pin, 1);
    return self;
}


// don't try macros like this at home :)
#define  dht_error(dht, error_str) dht->error = error_str, dht->state = DHT_ERROR; return -1


STATIC int dhtx(void *args, uint32_t now, uint8_t signal)
{
    mp_obj_dht_t *self = (mp_obj_dht_t *)(args);
    uint32_t elapsed = now - self->start;

    self->start = now;
    self->inters++;

    switch (self->state) {
    case DHT_STATE_NEW:
    case DHT_STATE_HPL:
    case DHT_STATE_HPH:
        break;
    case DHT_ERROR:
        return -1;
    case DHT_STATE_SPL:     // from SPL -> SPH now high, previous low was for 80uS, -> expecting 80 uS high
        if (signal != HIGH) {
            dht_error(self, "wrong state from SPL");
        }
        self->state = DHT_STATE_SPH; // no timer here because we are from outside the interrupt handler here (usually about 68uS)
        break;
    case DHT_STATE_SPH:
        if (signal != LOW) {
            dht_error(self, "wrong state from SPL");
        }
        if (elapsed > 70 && elapsed < 90) { // nominal 81
            self->state = DHT_STATE_DATA_START;
        } else {
            dht_error(self, "wrong START");
        }
        break;
    case DHT_STATE_DATA_START:  // been in high from sensor pulled high or previous data bit, now in low for 50uS
        if (signal != HIGH) {
            dht_error(self, "wrong state from sensor  in DATA START");
        }
        if (elapsed > 45 && elapsed <= 55) {
            self->state = DHT_STATE_DATA_DATA;
        } else if (elapsed > 55 && elapsed < 80) {     // word end
            self->state = DHT_STATE_DATA_DATA;
        } else {
            dht_error(self, "wrong DATA START time");
        }
        break;
    case DHT_STATE_DATA_DATA:
        if (signal != LOW) {
            dht_error(self, "wrong state from sensor in DATA DATA");
        }
        if (elapsed >= 20 && elapsed <= 30) {
            self->state = DHT_STATE_DATA_START;
            self->bits++;
        } else if (elapsed > 65 && elapsed < 80) {
            self->state = DHT_STATE_DATA_START;
            self->bytes[self->bits >> 3] |= 1 << (7 - (self->bits & 7));
            self->bits++;
        } else {
            dht_error(self, "bad data size");
        }
        if (self->bits == 40) {
            self->state = DHT_STATE_ENDED;
            memcpy(self->current, self->bytes, DHT_BYTES);
            // system_os_post(SENSOR_TASK_ID, 1, (os_param_t)self);
            return 0;
        }
        break;
    }
    return 0;
}


STATIC void  delay_us(int delay) {
    uint32_t count0 = xthal_get_ccount(); 
    uint32_t delayCount = delay * ets_get_cpu_frequency();
    while (xthal_get_ccount() - count0 < delayCount)
        ;
}

STATIC void dht_host_up(void *parg)
{
    mp_obj_dht_t *self = (mp_obj_dht_t *)parg;
    os_timer_disarm(&self->timer);
    
    GPIO_OUTPUT_SET(self->pmp->pin, 1);
    delay_us(30);
    GPIO_OUTPUT_SET(self->pmp->pin, 0);
    GPIO_REG_WRITE(GPIO_ENABLE_W1TC_ADDRESS, (1 << self->pmp->pin));    // set to input
    self->start = TIME();
    self->state = DHT_STATE_SPL;    // host pulls up then puts in high Z, transitioning to sensor pull low
    // GPIO_DIS_OUTPUT(GPIO_ID_PIN(pmp->pin));
} 


STATIC mp_obj_t ICACHE_FLASH_ATTR mod_esp_dht_state(mp_obj_t self_in, mp_obj_t len_in) {
    mp_obj_dht_t *self = self_in;
    return mp_obj_new_int(self->state);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_dht_state_obj, mod_esp_dht_state);


STATIC mp_obj_t ICACHE_FLASH_ATTR mod_esp_dht_values(mp_obj_t self_in, mp_obj_t len_in) {
    mp_obj_dht_t *self = self_in;
    mp_obj_t tuple[DHT_BYTES];
    for (int ii = 0; ii < DHT_BYTES; ii++)
        tuple[ii] = mp_obj_new_int(self->current[ii]);
    return mp_obj_new_tuple(DHT_BYTES, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_dht_values_obj, mod_esp_dht_values);

#define READ_WAIT 2000

STATIC mp_obj_t ICACHE_FLASH_ATTR  mod_esp_dht_recv(mp_obj_t self_in, mp_obj_t len_in) {
    mp_obj_dht_t *self = self_in;

    uint32_t elapsed =  (system_get_time() / 1000 - self->last_read_time);
    if (elapsed < READ_WAIT) {
        return mp_obj_new_int(READ_WAIT - elapsed);   // call back in 2 seconds
    }
    // this is not useful as the chip can often crash
    //  if ((self->state != DHT_STATE_ENDED && self->state != DHT_STATE_NEW))
    //     return mp_obj_new_int(2000);   // call back in 2 seconds

    dhtx_clear(self);
    GPIO_OUTPUT_SET(self->pmp->pin, 0);
    self->state = DHT_STATE_HPL;
    self->last_read_time = system_get_time() / 1000;
#if 0
    os_delay_us(1000);
    dht_host_up(self);
#else
    os_timer_setfn(&self->timer, dht_host_up, self);
    os_timer_arm(&self->timer, 1, 1);
#endif
    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_dht_recv_obj, mod_esp_dht_recv);


STATIC  mp_obj_t mod_esp_test(mp_obj_t start) {
    // uint32_t delay = mp_obj_get_int(start);
    return mp_obj_new_int(system_get_time() / 1000);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_test_obj, mod_esp_test);



STATIC const mp_map_elem_t dht_locals_dict_table[] = {
//    { MP_OBJ_NEW_QSTR(MP_QSTR___del__), (mp_obj_t)&mod_esp_dht___del___obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_values), (mp_obj_t)&mod_esp_dht_values_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_state), (mp_obj_t)&mod_esp_dht_state_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_recv), (mp_obj_t)&mod_esp_dht_recv_obj}
};

STATIC MP_DEFINE_CONST_DICT(dht_locals_dict, dht_locals_dict_table);

const mp_obj_type_t esp_dht_type = {
    { &mp_type_type },
    .name = MP_QSTR_dht,
    .print = dht_print,
    .make_new = dht_make_new,
    .getiter = NULL,
    .iternext = NULL,
    .locals_dict = (mp_obj_t)&dht_locals_dict,
};
