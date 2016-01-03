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
#include "netutils.h"
#include "user_interface.h"
#include "ip_addr.h"


typedef struct _esp_wifi_events_obj_t {
    mp_obj_t stamode_connected_cb;
    mp_obj_t stamode_disconnected_cb;
    mp_obj_t stamode_authmode_change_cb;
    mp_obj_t stamode_got_ip_cb;
} esp_wifi_events_obj_t;

static  esp_wifi_events_obj_t cb;


#if 0
mp_obj_t STATIC ICACHE_FLASH_ATTR esp_ip_addr_to_str(ip_addr_t *ipaddr) {
    return netutils_format_ipv4_addr((uint8_t *)&ipaddr->addr, NETUTILS_BIG);
}
#endif

void STATIC ICACHE_FLASH_ATTR wifi_handle_event_cb(System_Event_t *evt) {

    switch (evt->event) {
    case EVENT_STAMODE_CONNECTED:
        if (cb.stamode_connected_cb != mp_const_none) {
            call_function_2_protected(cb.stamode_connected_cb,
                                      mp_obj_new_str((char *)evt->event_info.connected.ssid,
                                          strlen((char *)evt->event_info.connected.ssid), true),
                                      MP_OBJ_NEW_SMALL_INT(evt->event_info.connected.channel));
        }
        break;
    case EVENT_STAMODE_DISCONNECTED:
        if (cb.stamode_disconnected_cb != mp_const_none) {
            call_function_2_protected(cb.stamode_disconnected_cb,
                                      mp_obj_new_str((char *)evt->event_info.disconnected.ssid,
                                          strlen((char *)evt->event_info.disconnected.ssid), true),
                                      MP_OBJ_NEW_SMALL_INT(evt->event_info.disconnected.reason));

        }
        break;
    case EVENT_STAMODE_AUTHMODE_CHANGE:
        if (cb.stamode_authmode_change_cb != mp_const_none) {
            call_function_2_protected(cb.stamode_authmode_change_cb,
                                      MP_OBJ_NEW_SMALL_INT(evt->event_info.auth_change.old_mode),
                                      MP_OBJ_NEW_SMALL_INT(evt->event_info.auth_change.new_mode));
        }
        break;
    case EVENT_STAMODE_GOT_IP:
        if (cb.stamode_got_ip_cb != mp_const_none) {
            mp_obj_t args[3];
            args[0] = netutils_format_ipv4_addr((uint8_t *)&evt->event_info.got_ip.ip.addr, NETUTILS_BIG);
            args[1] = netutils_format_ipv4_addr((uint8_t *)&evt->event_info.got_ip.mask, NETUTILS_BIG);
            args[2] = netutils_format_ipv4_addr((uint8_t *)&evt->event_info.got_ip.gw, NETUTILS_BIG);
            nlr_buf_t nlr;
            if (nlr_push(&nlr) == 0) {
                mp_call_function_n_kw(cb.stamode_got_ip_cb, 3, 0, args);
            } else {
                mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
                return;
            }
        }
    default:
        break;
   }
}

STATIC const mp_arg_t wifi_events_args[] = {
    {MP_QSTR_stamode_connected, MP_ARG_OBJ|MP_ARG_KW_ONLY, {.u_obj = mp_const_none}},
    {MP_QSTR_stamode_disconnected, MP_ARG_OBJ|MP_ARG_KW_ONLY, {.u_obj = mp_const_none}},
    {MP_QSTR_stamode_authmode_change, MP_ARG_OBJ|MP_ARG_KW_ONLY, {.u_obj = mp_const_none}},
    {MP_QSTR_stamode_got_ip, MP_ARG_OBJ|MP_ARG_KW_ONLY, {.u_obj = mp_const_none}}
};
#define WIFI_EVENTS_RUN_NUM_ARGS MP_ARRAY_SIZE(wifi_events_args)

STATIC ICACHE_FLASH_ATTR mp_obj_t wifi_events_init(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp_arg_val_t vals[WIFI_EVENTS_RUN_NUM_ARGS];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(wifi_events_args), wifi_events_args, vals);

    cb.stamode_connected_cb = vals[0].u_obj;
    cb.stamode_disconnected_cb = vals[1].u_obj;
    cb.stamode_authmode_change_cb = vals[2].u_obj;
    cb.stamode_got_ip_cb = vals[3].u_obj;

    wifi_set_event_handler_cb(wifi_handle_event_cb);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(wifi_events_init_obj, 0, wifi_events_init);


// TODO: make events dns pass a tuple to the lambda and make thus common
STATIC mp_obj_t ICACHE_FLASH_ATTR wifi_events_status(void) {
    struct ip_info info;

    wifi_get_ip_info(0, &info);
    mp_obj_t tuple[3] = {
        netutils_format_ipv4_addr((uint8_t *)&info.ip.addr, NETUTILS_BIG),
        netutils_format_ipv4_addr((uint8_t *)&info.netmask.addr, NETUTILS_BIG),
        netutils_format_ipv4_addr((uint8_t *)&info.gw.addr, NETUTILS_BIG)
    };
    return mp_obj_new_tuple(3, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(wifi_events_status_obj, wifi_events_status);


STATIC const mp_map_elem_t mo_module_esp_wifi_events_globals_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_wifi_events)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_init), (mp_obj_t)&wifi_events_init_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_status), (mp_obj_t)&wifi_events_status_obj},
};

STATIC MP_DEFINE_CONST_DICT(mo_module_esp_wifi_events_globals, mo_module_esp_wifi_events_globals_table);

const mp_obj_module_t mp_module_esp_wifi_events = {
    .base = { &mp_type_module },
    .name = MP_QSTR_gpio,
    .globals = (mp_obj_dict_t*)&mo_module_esp_wifi_events_globals,
};
