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


STATIC ICACHE_FLASH_ATTR void esp_os_task_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    esp_os_task_obj_t *self = self_in;

    mp_printf(print, "task(callback=%p arg=%p)", (unsigned int)&self->callback,  (unsigned int)self->arg);
}

STATIC void task_common_callback(os_event_t *evt) {
    esp_os_task_obj_t *self = (esp_os_task_obj_t *)evt->par;
    if (self->callback) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            (void)mp_call_function_2(self->callback, self, self->arg);
        } else {
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
        gc_collect();
    }
}

STATIC os_event_t sensor_evt_queue[16];

void  ICACHE_FLASH_ATTR  esp_os_task_init() {
    system_os_task(task_common_callback, SENSOR_TASK_ID, sensor_evt_queue, sizeof(sensor_evt_queue) / sizeof(*sensor_evt_queue));
    printf("init os_task\n");
}

STATIC const mp_arg_t esp_os_task_init_args[] = {
    { MP_QSTR_callback, MP_ARG_REQUIRED | MP_ARG_OBJ },
};
#define PYB_TASK_INIT_NUM_ARGS MP_ARRAY_SIZE(esp_os_task_init_args)

STATIC ICACHE_FLASH_ATTR mp_obj_t esp_os_task_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    mp_arg_val_t vals[PYB_TASK_INIT_NUM_ARGS];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, PYB_TASK_INIT_NUM_ARGS, esp_os_task_init_args, vals);
    
    esp_os_task_obj_t *self = m_new_obj(esp_os_task_obj_t);
    self->base.type = &esp_os_task_type;

    self->callback = vals[0].u_obj;
    return (mp_obj_t)self;
}

ICACHE_FLASH_ATTR void os_task_post(mp_obj_t *callback, mp_obj_t *arg) {
    esp_os_task_obj_t *tasko = m_new_obj(esp_os_task_obj_t);
    tasko->base.type = &esp_os_task_type;
    tasko->callback = callback;
    tasko->arg = arg;
    printf("callback is %u arg is %d", (unsigned)callback, (unsigned)arg);
    system_os_post(SENSOR_TASK_ID, 1, (os_param_t)tasko);
}

STATIC const mp_arg_t esp_os_task_post_args[] = {
    { MP_QSTR_arg, MP_ARG_KW_ONLY | MP_ARG_OBJ },
};
#define ESP_OS_TASK_POST_NUM_ARGS MP_ARRAY_SIZE(esp_os_task_post_args)

STATIC ICACHE_FLASH_ATTR mp_obj_t esp_os_task_post(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    esp_os_task_obj_t *self = args[0];

    mp_arg_val_t vals[ESP_OS_TASK_POST_NUM_ARGS];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, ESP_OS_TASK_POST_NUM_ARGS, esp_os_task_post_args, vals);

    os_task_post(self->callback, vals[0].u_obj);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(esp_os_task_post_obj, 1, esp_os_task_post);

STATIC const mp_map_elem_t esp_os_task_locals_dict_table[] = {
//    { MP_OBJ_NEW_QSTR(MP_QSTR_cancel), (mp_obj_t)&esp_os_timer_cancel_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_post), (mp_obj_t)&esp_os_task_post_obj },
};
STATIC MP_DEFINE_CONST_DICT(esp_os_task_locals_dict, esp_os_task_locals_dict_table);

const mp_obj_type_t esp_os_task_type = {
    { &mp_type_type },
    .name = MP_QSTR_os_task,
    .print = esp_os_task_print,
    .make_new = esp_os_task_make_new,
    .locals_dict = (mp_obj_t)&esp_os_task_locals_dict,
};

