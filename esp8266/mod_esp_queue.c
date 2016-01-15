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

#include "os_task.h"
#include "mod_esp_mutex.h"
#include "mod_esp_queue.h"


STATIC ICACHE_FLASH_ATTR void mod_esp_queue_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    esp_queue_obj_t *self = self_in;

    mp_printf(print, "queue.size=%d queue.items=%d", self->max_items, self->items);
}


STATIC const mp_arg_t mod_esp_queue_init_args[] = {
    {MP_QSTR_storage, MP_ARG_REQUIRED | MP_ARG_OBJ},
    {MP_QSTR_mutex, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none}},
    {MP_QSTR_os_task, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none}}
};
#define ESP_MUTEX_INIT_NUM_ARGS MP_ARRAY_SIZE(mod_esp_queue_init_args)

STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_queue_make_new(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    mp_arg_val_t vals[ESP_MUTEX_INIT_NUM_ARGS];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, ESP_MUTEX_INIT_NUM_ARGS, mod_esp_queue_init_args, vals);

    if (!MP_OBJ_IS_TYPE(vals[0].u_obj,  &mp_type_list)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "storage needs to be a list"));
    }
    esp_queue_obj_t *self = m_new_obj(esp_queue_obj_t);

    self->base.type = &esp_queue_type;
    mp_obj_list_t *list = (mp_obj_list_t *)vals[0].u_obj;
	self->items = 0;
	self->first = 0;
	self->last = 0;
    self->max_items = list->len;
    self->obj_instances = list->items;
    if (vals[1].u_obj != mp_const_none) {
        if (MP_OBJ_IS_TYPE(vals[1].u_obj, &esp_mutex_type)) {
            self->mutex = vals[1].u_obj;
        } else {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "mutex needs to be an esp.mutex type"));
        }
    } else {
        self->mutex = mp_const_none;
    }
    if (vals[2].u_obj != mp_const_none) {
        if (MP_OBJ_IS_TYPE(vals[2].u_obj, &esp_os_task_type)) {
            self->os_task = vals[2].u_obj;
        } else {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "os_task needs to be an esp.os_task type"));
        }
    } else {
        self->os_task = mp_const_none;
    }
    return (mp_obj_t)self;
}


STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_queue_put(mp_obj_t self_in, mp_obj_t add_obj) {
    esp_queue_obj_t *self = self_in;

	if (self->items >= self->max_items) {
         nlr_raise(mp_obj_new_exception(&mp_type_Full));
	}
    self->items++;
    self->obj_instances[self->last] = add_obj;
    self->last = (self->last + 1) % self->max_items;
    if (self->os_task != mp_const_none) {
        system_os_post(SENSOR_TASK_ID, 1, (os_param_t)self->os_task);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_esp_queue_put_obj, mod_esp_queue_put);


STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_queue_get(mp_obj_t self_in) {
    esp_queue_obj_t *self = self_in;

	if (self->items == 0) {
         nlr_raise(mp_obj_new_exception(&mp_type_Empty));
	}
    mp_obj_t ret = self->obj_instances[self->first];
    self->first = (self->first + 1) % self->max_items;
    self->items--;
    return ret;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_queue_get_obj, mod_esp_queue_get);

bool ICACHE_FLASH_ATTR esp_queue_check_for_dalist_8(esp_queue_obj_t *queue_in, uint32_t len) {
    mp_obj_t *inst = queue_in->obj_instances[queue_in->last];
    if (!MP_OBJ_IS_TYPE(inst,  &mp_type_list)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "storage needs to be a list"));
    } else {
        mp_obj_list_t *al = (mp_obj_list_t *)inst;
        if (al->len < len) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "queue data is too small"));
        }
        if (!MP_OBJ_IS_SMALL_INT(al->items[0])) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "not smallint"));
        } 
    }
    return true;
}

bool ICACHE_FLASH_ATTR esp_queue_check_for_daint_8(esp_queue_obj_t *queue_in) {
    mp_obj_t *inst = queue_in->obj_instances[queue_in->last];
    if (!MP_OBJ_IS_SMALL_INT(inst)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "not smallint"));
    } 
    return true;
}

// for use in interrupts no ICACHE_FLASH_ATTR!
int8_t esp_queue_dalist_8(esp_queue_obj_t *queue_in, uint32_t len, uint8_t *vals) {
	if (queue_in->items >= queue_in->max_items) {
        // printf("full\n");
        return -1;
    } 

    mp_obj_t *inst = queue_in->obj_instances[queue_in->last];
    if (!MP_OBJ_IS_TYPE(inst,  &mp_type_list)) {
        // printf("not a list\n");
        return -2;
    }
    mp_obj_list_t *al = (mp_obj_list_t *)inst;
    if (len > al->len) {
        // printf("too many values (%d) for list %d\n", len, al->len);
        return -3;
    }
    for (int ii = 0; ii < len; ii++, vals++) {
        al->items[ii] = MP_OBJ_NEW_SMALL_INT(*vals); // WARNING: no checking, can explode, but unlikely as 8 bits in 
    }
    queue_in->items++;
    queue_in->last = (queue_in->last + 1) % queue_in->max_items;
    if (queue_in->os_task != mp_const_none) {
        system_os_post(SENSOR_TASK_ID, 1, (os_param_t)queue_in->os_task);
    }
    return 0;
}

// for use in interrupts
int8_t esp_queue_daint_8(esp_queue_obj_t *queue_in, uint8_t value) {
	if (queue_in->items >= queue_in->max_items) {
        // printf("full\n");
        return -1;
    } 
    queue_in->obj_instances[queue_in->last] = MP_OBJ_NEW_SMALL_INT(value);
    queue_in->items++;
    queue_in->last = (queue_in->last + 1) % queue_in->max_items;
    if (queue_in->os_task != mp_const_none) {
        system_os_post(SENSOR_TASK_ID, 1, (os_param_t)queue_in->os_task);
    }
    return 0;
}


STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_queue_test(mp_obj_t self_in, mp_obj_t add_obj) {
#if 1
    esp_queue_obj_t *self = self_in;
	if (self->items >= self->max_items) {
         nlr_raise(mp_obj_new_exception(&mp_type_Full));
    } 

    if (MP_OBJ_IS_TYPE(self->obj_instances[self->last],  &mp_type_list)) {
        printf("list type of list\n");
        if (!MP_OBJ_IS_TYPE(add_obj,  &mp_type_list)) {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError, "pass a list only"));
        }
        mp_obj_list_t *in = (mp_obj_list_t *)add_obj;

        uint8_t vars[in->len];
        for (int ii = 0; ii < in->len; ii++) {
            vars[ii] = MP_OBJ_SMALL_INT_VALUE(in->items[ii]);
        }
        esp_queue_dalist_8(self, in->len, vars);
    } else if (MP_OBJ_IS_SMALL_INT(self->obj_instances[self->last])) {
        printf("smallint type of list\n");
        if (!MP_OBJ_IS_SMALL_INT(add_obj)) {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError, "pass an int only"));
        }
        printf("is small int\n");
        esp_queue_daint_8(self, MP_OBJ_SMALL_INT_VALUE(add_obj));
    }
#endif
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_esp_queue_test_obj, mod_esp_queue_test);

STATIC const mp_map_elem_t mod_esp_queue_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_put), (mp_obj_t)&mod_esp_queue_put_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get), (mp_obj_t)&mod_esp_queue_get_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_test), (mp_obj_t)&mod_esp_queue_test_obj }
};
STATIC MP_DEFINE_CONST_DICT(mod_esp_queue_locals_dict, mod_esp_queue_locals_dict_table);

const mp_obj_type_t esp_queue_type = {
    { &mp_type_type },
    .name = MP_QSTR_queue,
    .print = mod_esp_queue_print,
    .make_new = mod_esp_queue_make_new,
    .locals_dict = (mp_obj_t)&mod_esp_queue_locals_dict,
};

