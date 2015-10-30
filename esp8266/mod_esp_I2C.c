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

#include "mod_esp_gpio.h"
#include "esp_i2c_master.h"
#include "mod_esp_I2C.h"

STATIC ICACHE_FLASH_ATTR void mod_esp_I2C_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    esp_I2C_obj_t *self = self_in;

    mp_printf(print, "I2C.sda_pin=%d, I2C.scl_pin=%d", self->esp_i2c.sda_pmp->pin, self->esp_i2c.scl_pmp->pin);
}



STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_I2C_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    STATIC const mp_arg_t mod_esp_I2C_init_args[] = {
        { MP_QSTR_sda_pin, MP_ARG_INT, {.u_int = 2}},
        { MP_QSTR_scl_pin, MP_ARG_INT, {.u_int = 14}}
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(mod_esp_I2C_init_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(mod_esp_I2C_init_args), mod_esp_I2C_init_args, vals);

    esp_I2C_obj_t *self = m_new_obj(esp_I2C_obj_t);
    self->base.type = &esp_I2C_type;
    i2c_master_gpio_init(&self->esp_i2c, vals[0].u_int, vals[1].u_int);
    return (mp_obj_t)self;
}


STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_I2C_mem_write(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    STATIC const mp_arg_t mem_write_args[] = {
        { MP_QSTR_data, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_address, MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_register, MP_ARG_INT },
    };
    esp_I2C_obj_t *self = args[0];

    mp_arg_val_t arg_vals[MP_ARRAY_SIZE(mem_write_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kwargs, MP_ARRAY_SIZE(mem_write_args), mem_write_args, arg_vals);

    if (!MP_OBJ_IS_TYPE(arg_vals[0].u_obj, &mp_type_list)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "send argument must be a list"));
    } 
    mp_obj_list_t *o = MP_OBJ_CAST(arg_vals[0].u_obj);
    i2c_master_start(&self->esp_i2c);
	i2c_master_writeByte(&self->esp_i2c, (uint8)(arg_vals[1].u_int << 1));  // write address
	if (!i2c_master_checkAck(&self->esp_i2c)) {
		i2c_master_stop(&self->esp_i2c);
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "i2c no ack for address"));
    }
	for (int loop = 0; loop < o->len; loop++) {
        mp_obj_t item = o->items[loop];
        if (MP_OBJ_IS_SMALL_INT(item)) {
            i2c_master_writeByte(&self->esp_i2c, MP_OBJ_SMALL_INT_VALUE(item));
        } else {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "not an int in list"));
        }
		if (!i2c_master_checkAck(&self->esp_i2c)) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "i2c erro no ack for data"));
		}
	}
    i2c_master_stop(&self->esp_i2c);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mod_esp_I2C_mem_write_obj, 2, mod_esp_I2C_mem_write);


STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_I2C_mem_read(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    STATIC const mp_arg_t mem_read_args[] = {
        { MP_QSTR_len, MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_address, MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_register, MP_ARG_INT },
    };
    esp_I2C_obj_t *self = args[0];

    mp_arg_val_t arg_vals[MP_ARRAY_SIZE(mem_read_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kwargs, MP_ARRAY_SIZE(mem_read_args), mem_read_args, arg_vals);

    uint8_t len = arg_vals[0].u_int;
    uint8_t *data = alloca(len * sizeof(uint8_t));

    i2c_master_start(&self->esp_i2c);
	i2c_master_writeByte(&self->esp_i2c, (uint8)(arg_vals[1].u_int << 1) | 1); 
	if (!i2c_master_checkAck(&self->esp_i2c)) {
		i2c_master_stop(&self->esp_i2c);
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "i2c no ack for address"));
    }
	for (int loop = 0; loop < len; loop++) {
		data[loop] = i2c_master_readByte(&self->esp_i2c);
		// send ack (except after last byte, then we send nack)
		if (loop < (len - 1)) {
            i2c_master_send_ack(&self->esp_i2c);
        } else {
            i2c_master_send_nack(&self->esp_i2c);
        }
	}
    i2c_master_stop(&self->esp_i2c);
    mp_obj_list_t *nl = mp_obj_new_list(len, NULL);
    for (int loop = 0; loop < len; loop++) {
        nl->items[loop] = MP_OBJ_NEW_SMALL_INT(data[loop]);
    }
    return nl;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mod_esp_I2C_mem_read_obj, 2, mod_esp_I2C_mem_read);


STATIC const mp_map_elem_t mod_esp_I2C_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_mem_write), (mp_obj_t)&mod_esp_I2C_mem_write_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_mem_read), (mp_obj_t)&mod_esp_I2C_mem_read_obj },
};
STATIC MP_DEFINE_CONST_DICT(mod_esp_I2C_locals_dict, mod_esp_I2C_locals_dict_table);

const mp_obj_type_t esp_I2C_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = mod_esp_I2C_print,
    .make_new = mod_esp_I2C_make_new,
    .locals_dict = (mp_obj_t)&mod_esp_I2C_locals_dict,
};

