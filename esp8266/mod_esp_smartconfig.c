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
#include "smartconfig.h"


void STATIC ICACHE_FLASH_ATTR smartconfig_callback(sc_status status, void *pdata) {
    switch(status) {
        case SC_STATUS_WAIT:
            printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            printf("SC_STATUS_GETTING_SSID_PSWD\n");
			sc_type *type = pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:
            printf("SC_STATUS_LINK\n");
            struct station_config *sta_conf = pdata;

            printf("name: \"%s\"\n", sta_conf->ssid);
            printf("pass: \"%s\"\n", sta_conf->password);
            printf("bssid_set: %u\n", sta_conf->bssid_set);
            printf("bssid: \"%s\"\n", sta_conf->bssid);

	        wifi_station_set_config(sta_conf);
	        wifi_station_disconnect();
	        wifi_station_connect();
            break;
        case SC_STATUS_LINK_OVER:
            printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL) {
                uint8 phone_ip[4] = {0};

                memcpy(phone_ip, (uint8*)pdata, 4);
                printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
            }
            smartconfig_stop();
            break;
    }
	
}

STATIC const mp_arg_t smartconfig_run_args[] = {
    {MP_QSTR_mode, MP_ARG_INT|MP_ARG_KW_ONLY, {.u_int = SC_TYPE_ESPTOUCH}}
};
#define SMARTCONFIG_RUN_NUM_ARGS MP_ARRAY_SIZE(smartconfig_run_args)

STATIC ICACHE_FLASH_ATTR mp_obj_t smartconfig_run(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp_arg_val_t vals[SMARTCONFIG_RUN_NUM_ARGS];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(smartconfig_run_args), smartconfig_run_args, vals);

    smartconfig_set_type(vals[0].u_int); //SC_TYPE_ESPTOUCH,SC_TYPE_AIRKISS,SC_TYPE_ESPTOUCH_AIRKISS
    wifi_set_opmode(STATION_MODE);
    smartconfig_start(smartconfig_callback);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(smartconfig_run_obj, 0, smartconfig_run);

STATIC const mp_map_elem_t mo_module_esp_smartconfig_globals_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_smartconfig)},
    { MP_OBJ_NEW_QSTR(MP_QSTR_run), (mp_obj_t)&smartconfig_run_obj },
};

STATIC MP_DEFINE_CONST_DICT(mo_module_esp_smartconfig_globals, mo_module_esp_smartconfig_globals_table);

const mp_obj_module_t mp_module_esp_smartconfig = {
    .base = { &mp_type_module },
    .name = MP_QSTR_gpio,
    .globals = (mp_obj_dict_t*)&mo_module_esp_smartconfig_globals,
};
