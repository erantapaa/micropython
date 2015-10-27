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
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "py/nlr.h"
#include "py/runtime.h"
#include "py/gc.h"

#include "osapi.h"
#include "os_type.h"
#include "utils.h"
#include "user_interface.h"

#include "mod_esp_I2C.h"

STATIC ICACHE_FLASH_ATTR void mod_esp_I2C_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    esp_I2C_obj_t *self = self_in;

    mp_printf(print, "I2C.pin=%d", self->sda_pin, self->scl_pin);
}



STATIC const mp_arg_t mod_esp_I2C_init_args[] = {
    { MP_QSTR_sda_pin, MP_ARG_REQUIRED | MP_ARG_INT},
    { MP_QSTR_scl_pin, MP_ARG_REQUIRED | MP_ARG_INT},
};
#define ESP_I2C_INIT_NUM_ARGS MP_ARRAY_SIZE(mod_esp_I2C_init_args)

STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_I2C_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    mp_arg_val_t vals[ESP_I2C_INIT_NUM_ARGS];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, ESP_I2C_INIT_NUM_ARGS, mod_esp_I2C_init_args, vals);

    esp_I2C_obj_t *self = m_new_obj(esp_I2C_obj_t);
    self->base.type = &esp_I2C_type;
    self->sda_pin = vals[0].u_int;
    self->scl_pin = vals[1].u_int;
    return (mp_obj_t)self;
}


STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_I2C_send(mp_obj_t self_in) {
 //   esp_I2C_obj_t *self = self_in;
    printf("send\n");
    return mp_obj_new_bool(1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_I2C_send_obj, mod_esp_I2C_send);

STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_I2C_recv(mp_obj_t self_in) {
//    esp_I2C_obj_t *self = self_in;

    printf("receive\n");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_I2C_recv_obj, mod_esp_I2C_recv);

STATIC const mp_map_elem_t mod_esp_I2C_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_send), (mp_obj_t)&mod_esp_I2C_send_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_recv), (mp_obj_t)&mod_esp_I2C_recv_obj },
};
STATIC MP_DEFINE_CONST_DICT(mod_esp_I2C_locals_dict, mod_esp_I2C_locals_dict_table);

const mp_obj_type_t esp_I2C_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = mod_esp_I2C_print,
    .make_new = mod_esp_I2C_make_new,
    .locals_dict = (mp_obj_t)&mod_esp_I2C_locals_dict,
};
