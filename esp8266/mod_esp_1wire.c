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
#include "py/obj.h"
#include "py/runtime.h"
#include "py/gc.h"

#include "osapi.h"
#include "os_type.h"
#include "utils.h"
#include "user_interface.h"
#include "gpio.h"

#include "mod_esp_gpio.h"
#include "esp_1wire.h"
#include "mod_esp_1wire.h"

STATIC int16_t esp_1wire_handler(pmap_t *pmp, uint32_t now, uint8_t signal)
{

    esp_1wire_obj_t *self = (esp_1wire_obj_t *)(pmp->data);
    self->ints++;
    return 0;
}

STATIC ICACHE_FLASH_ATTR void mod_esp_1wire_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    esp_1wire_obj_t *self = self_in;

    mp_printf(print, "pin=%d, ints=%d", self->gpio->pin, self->ints);
}

STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_1wire_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    STATIC const mp_arg_t mod_esp_1wire_init_args[] = {
        { MP_QSTR_pin, MP_ARG_INT|MP_ARG_REQUIRED},
        { MP_QSTR_edge, MP_ARG_INT | MP_ARG_KW_ONLY, {.u_int = GPIO_PIN_INTR_DISABLE}},     // 1
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(mod_esp_1wire_init_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(mod_esp_1wire_init_args), mod_esp_1wire_init_args, vals);

    esp_1wire_obj_t *self = m_new_obj(esp_1wire_obj_t);
    self->base.type = &esp_1wire_type;
    self->gpio = ds_init(vals[0].u_int);
    self->ints = 0;
    self->edge = vals[1].u_int;
    printf("edge is %d\n", self->edge);
    esp_gpio_isr_attach(self->gpio, esp_1wire_handler, self, 50); // 50 events
//    GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, (1<<self->gpio->pin)); // Enable output, is this needed?
//    gpio_pin_intr_state_set(GPIO_ID_PIN(self->gpio->pin), GPIO_PIN_INTR_ANYEDGE);
    return (mp_obj_t)self;
}


STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_1wire_reset(mp_obj_t self_in) {
    esp_1wire_obj_t *self = self_in;

    gpio_pin_intr_state_set(GPIO_ID_PIN(self->gpio->pin), GPIO_PIN_INTR_DISABLE);
    self->ints = 0;
    uint8_t vv =  ds_reset(self->gpio);
    if (vv < 0) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, vv == -1 ? "gpio wire is held down" : "no slaves held down pin"));
    }
    return mp_obj_new_bool(vv);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_1wire_reset_obj, mod_esp_1wire_reset);


STATIC ICACHE_FLASH_ATTR void write_mp_array(esp_1wire_obj_t *self, mp_obj_list_t *o, bool power) {
	for (int loop = 0; loop < o->len; loop++) {
        mp_obj_t item = o->items[loop];
        if (MP_OBJ_IS_SMALL_INT(item)) {
            ds_write(self->gpio, MP_OBJ_SMALL_INT_VALUE(item), power);
        } else {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "not an int in list"));
        }
	}
}

STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_1wire_write(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    STATIC const mp_arg_t write_args[] = {
        { MP_QSTR_data, MP_ARG_REQUIRED | MP_ARG_OBJ },                             // 0
        { MP_QSTR_address, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = mp_const_none}},  // 1          (4 bytes)
        { MP_QSTR_suppress_skip,  MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false}},  // 2
        { MP_QSTR_power, MP_ARG_BOOL | MP_ARG_KW_ONLY,  {.u_bool = false}}         // 3
    };
    esp_1wire_obj_t *self = args[0];

    mp_arg_val_t arg_vals[MP_ARRAY_SIZE(write_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kwargs, MP_ARRAY_SIZE(write_args), write_args, arg_vals);

    if (!MP_OBJ_IS_TYPE(arg_vals[0].u_obj, &mp_type_list)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "write argument must be a list"));
    } 

    gpio_pin_intr_state_set(GPIO_ID_PIN(self->gpio->pin), GPIO_PIN_INTR_POSEDGE);
    if (arg_vals[1].u_obj != mp_const_none) {
        printf("address ..");
        mp_obj_list_t *o = arg_vals[1].u_obj;
        if (o->len != 4) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "expecting 4 bytes for address"));
        }
        write_mp_array(self, o, arg_vals[3].u_bool);    // 3, power hold
    } else if (arg_vals[2].u_bool != true) {
        ds_write(self->gpio, SKIP_ROM, arg_vals[3].u_bool);
    }
    write_mp_array(self, arg_vals[0].u_obj, arg_vals[3].u_bool);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mod_esp_1wire_write_obj, 2, mod_esp_1wire_write);


STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_1wire_read(mp_obj_t self_in,  mp_obj_t len_in) {
    esp_1wire_obj_t *self = self_in;

    mp_uint_t mxl = mp_obj_get_int(len_in);
    mp_obj_tuple_t *tuple = mp_obj_new_tuple(mxl, NULL);

    gpio_pin_intr_state_set(GPIO_ID_PIN(self->gpio->pin), GPIO_PIN_INTR_DISABLE);
#if 0
    if (self->edge != GPIO_PIN_INTR_DISABLE) {
        gpio_pin_intr_state_set(GPIO_ID_PIN(self->gpio->pin), GPIO_PIN_INTR_DISABLE);   // disable before reads
    }
#endif
    for (int ii = 0; ii < mxl; ii++) {
        uint8_t val = ds_read(self->gpio);
        tuple->items[ii] = MP_OBJ_NEW_SMALL_INT(val);
    }
    return tuple;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_esp_1wire_read_obj, mod_esp_1wire_read);


STATIC const mp_map_elem_t mod_esp_1wire_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_reset), (mp_obj_t)&mod_esp_1wire_reset_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_write), (mp_obj_t)&mod_esp_1wire_write_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read), (mp_obj_t)&mod_esp_1wire_read_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CONVERT_T), MP_OBJ_NEW_SMALL_INT(CONVERT_T)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_READ_SCRATCH_PAD), MP_OBJ_NEW_SMALL_INT(READ_SCRATCH_PAD)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_WRITE_SCRATCH_PAD), MP_OBJ_NEW_SMALL_INT(WRITE_SCRATCH_PAD)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_COPY_SCRATCH_PAD), MP_OBJ_NEW_SMALL_INT(COPY_SCRATCH_PAD)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_RECALL_E), MP_OBJ_NEW_SMALL_INT(RECALL_E)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_READ_POWER_SUPPLY), MP_OBJ_NEW_SMALL_INT(READ_POWER_SUPPLY)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_READ_ROM), MP_OBJ_NEW_SMALL_INT(READ_ROM)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_MATCH_ROM), MP_OBJ_NEW_SMALL_INT(MATCH_ROM)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_SEARCH_ROM), MP_OBJ_NEW_SMALL_INT(SEARCH_ROM)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_SKIP_ROM), MP_OBJ_NEW_SMALL_INT(SKIP_ROM)}
};
STATIC MP_DEFINE_CONST_DICT(mod_esp_1wire_locals_dict, mod_esp_1wire_locals_dict_table);

const mp_obj_type_t esp_1wire_type = {
    { &mp_type_type },
    .name = MP_QSTR_one_wire,
    .print = mod_esp_1wire_print,
    .make_new = mod_esp_1wire_make_new,
    .locals_dict = (mp_obj_t)&mod_esp_1wire_locals_dict,
};

