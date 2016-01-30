#include <stdio.h>
#include <stdlib.h>

#include "../py/runtime.h"
#include "../py/obj.h"
#include "../py/objstr.h"
#include "../py/misc.h"
#include "netutils.h"

#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"

#include "gccollect.h"

#include "mod_esp_ws.h"


extern int skip_atoi(char **nptr);
int ets_sprintf(char *str, const char *format, ...);


void ICACHE_FLASH_ATTR http_context_reset(esp_ws_obj_t *ctx) {
    ctx->ptr = ctx->buffer;
    ctx->state = method_or_reply_http;
    ctx->content_length = 0;
    ctx->method = none;
    ctx->header_key = mp_const_none;
    ctx->headers = mp_obj_new_list(0, NULL);
    ctx->uri = mp_const_none;
    ctx->body = mp_const_none;
    ctx->str_method = mp_const_none;
    ctx->to_send = NULL;
    ctx->to_send_len = 0;
    ctx->str_status = mp_const_none;
}


void  ICACHE_FLASH_ATTR http_context_add_char(esp_ws_obj_t *ctx, char cc) {
    if (ctx->ptr - ctx->buffer > ctx->len) {
        printf("buffer_size overflow\n");
        return;
    }
    *ctx->ptr++ = cc;
}

char ICACHE_FLASH_ATTR *http_context_get_and_reset(esp_ws_obj_t *ctx) {
    http_context_add_char(ctx, '\0');
    ctx->ptr = ctx->buffer;
    return ctx->buffer;
}

char ICACHE_FLASH_ATTR *http_context_mpstr_and_reset(esp_ws_obj_t *ctx) {
    mp_obj_t bp = mp_obj_new_str(ctx->buffer, ctx->ptr - ctx->buffer, true);
    ctx->ptr = ctx->buffer;
    return bp;
}

void ICACHE_FLASH_ATTR esp_make_http_to_send(esp_ws_obj_t *pesp, mp_obj_str_t *mp_str, const char *body_base, uint16_t body_base_size) {
    int  body_size = mp_str == mp_const_none ? 0 : mp_str->len;

    if (body_size) {
        const char *bls = "Content-Length: %d\r\n";
        char body_size_str[strlen(bls) + 10];
        ets_sprintf(body_size_str, bls, mp_str->len);
        int body_size_str_len = strlen(body_size_str);

        pesp->to_send_len = body_base_size + pesp->outgoing_headers_len + body_size_str_len + 2 + mp_str->len;
        pesp->to_send = (char *)m_malloc(pesp->to_send_len);

        memcpy(pesp->to_send, body_base, body_base_size);
        if (pesp->outgoing_headers_len) {
            memcpy(pesp->to_send + body_base_size, pesp->outgoing_headers_str, pesp->outgoing_headers_len);
        }
        memcpy(pesp->to_send + body_base_size + pesp->outgoing_headers_len, body_size_str, body_size_str_len);
        memcpy(pesp->to_send + body_base_size + pesp->outgoing_headers_len + body_size_str_len, "\r\n", 2);
        memcpy(pesp->to_send + body_base_size + pesp->outgoing_headers_len + body_size_str_len + 2, (char *)mp_str->data, mp_str->len);
    } else {
        pesp->to_send_len = body_base_size + pesp->outgoing_headers_len + 2;  // + extra \r\n
        pesp->to_send = (char *)m_malloc(pesp->to_send_len);
        memcpy(pesp->to_send, body_base, body_base_size);
        if (pesp->outgoing_headers_len) {
            memcpy(pesp->to_send + body_base_size, pesp->outgoing_headers_str, pesp->outgoing_headers_len);
        }
        memcpy(pesp->to_send + body_base_size + pesp->outgoing_headers_len, "\r\n", 2);
    }
    //printf("send %.*s", pesp->to_send_len, pesp->to_send);

}

void ICACHE_FLASH_ATTR ws_reply(esp_ws_obj_t *pesp, struct espconn *pesp_conn) {
    mp_obj_t client_reply = mp_const_none;

    if (pesp->callback) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            client_reply = mp_call_function_1(pesp->callback, pesp);
        } else {
           mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
    }
    if (pesp->method != reply) {
        if (client_reply != mp_const_none) {
            if (!MP_OBJ_IS_TYPE(client_reply,  &mp_type_str)) {
                nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "ws handler can only return str (or None)"));
            }
        }
        const char *body_base = "HTTP/1.1 200 OK\r\nConnection: close\r\n";
        uint16_t body_base_size = strlen(body_base);
        esp_make_http_to_send(pesp, (mp_obj_str_t *)client_reply, body_base, body_base_size);
        espconn_sent(&pesp->esp_conn, (uint8 *)pesp->to_send, (uint16)pesp->to_send_len);
    }
}

static  ICACHE_FLASH_ATTR void webserver_recv(void *arg, char *pusrdata, unsigned short length) {
    struct espconn *pesp_conn = arg;
    esp_ws_obj_t *pesp = (esp_ws_obj_t *)((struct espconn *)arg)->reverse;

    for (int pos = 0; pos < length; pos++) {
        uint8_t cc = pusrdata[pos];
        // printf("state %d '%c'\n", pesp->state, cc);
        if (cc == '\r') {
            continue;
        }
        switch (pesp->state) {
        case method_or_reply_http:
            if (cc == ' ') {
                char *method = http_context_get_and_reset(pesp);
                if (strncmp(method, "HTTP/", 5) == 0) {
                    pesp->state = status_code;
                    pesp->method = reply;
                } else {
                    pesp->str_method = mp_obj_new_str(method, strlen(method), true);
                    pesp->method = strcmp(method, "GET") == 0 ? get : other;
                    pesp->state = uri;
                }
            } else {
                http_context_add_char(pesp, cc);
            }
            break;
        case status_code:
            if (cc == '\n') {
                pesp->str_status = http_context_mpstr_and_reset(pesp);
                pesp->state = header_key;
            } else {
                http_context_add_char(pesp, cc);
            }
            break;
        case uri:
            if (cc == ' ') {
                pesp->uri = http_context_mpstr_and_reset(pesp);
                pesp->state = http_version;
            } else {
                http_context_add_char(pesp, cc);
            }
            break;
        case http_version:
            if (cc == '\n') {
                (void)http_context_get_and_reset(pesp);
                pesp->state = header_key;
            } else {
                http_context_add_char(pesp, cc);
            }
            break;
        case header_key:
            if (cc == ':') {
                char *hname = http_context_get_and_reset(pesp);
                if (strcmp("Content-Length", hname) == 0) {
                    pesp->state = content_length_sep;
                } else {
                    pesp->header_key = mp_obj_new_str(hname, strlen(hname), true);
                    pesp->state = header_sep;
               }
            } else {
                http_context_add_char(pesp, cc);
            }
            break;
        case content_length_sep:
            if (cc != ' ') {
                http_context_add_char(pesp, cc);
                pesp->state = content_length;
            }
            break;
        case content_length:
            if (cc == '\n') {
                char *clength = http_context_get_and_reset(pesp);
                pesp->content_length = skip_atoi(&clength);
                pesp->state = possible_body;
            } else {
                http_context_add_char(pesp, cc);
            }
            break;

        case header_sep:
            if (cc != ' ') {
                http_context_add_char(pesp, cc);
                pesp->state = header_val;
            }
            break;
        case header_val:
            if (cc == '\n') {
                mp_obj_t atuple[2] = {
                    pesp->header_key,
                    http_context_mpstr_and_reset(pesp)
                };
                mp_obj_list_append(pesp->headers, mp_obj_new_tuple(2, atuple));
                pesp->state = possible_body;
            } else {
                http_context_add_char(pesp, cc);
            }
            break;
        case possible_body:
            if (cc != '\n') {
                http_context_add_char(pesp, cc);
                pesp->state = header_key;
            } else {
                pesp->state = body_sep;
            }
            break;
        case body_sep:
            if (cc != '\n') {
                http_context_add_char(pesp, cc);
                pesp->state = body;
            } // else bad state
            break;
        case body:
            http_context_add_char(pesp, cc);
            break;
        }
    }


    if (pesp->method == get && pesp->state == body_sep) {       // if a GET then no body
        ws_reply(pesp, pesp_conn);
    } else if (pesp->state == body) {
        int size = pesp->ptr - pesp->buffer;
        if (size) {
            if (pesp->content_length == pesp->content_length) {
                pesp->body = http_context_mpstr_and_reset(pesp);
                ws_reply(pesp, pesp_conn);
            } else {
                http_context_add_char(pesp, '\0');    // go through this for out by one overflow check
                pesp->ptr--;
                printf("different body '%s'\n", pesp->buffer);
            }
        } else {
            printf("have not got body and no size (all consume) check content length\n");
                printf("got body and ength matched");
        }
    }
    return;
}

static void  ICACHE_FLASH_ATTR webserver_recon(void *arg, sint8 err)
{
    struct espconn *pesp_conn = arg;
    printf("reconnect  | %d.%d.%d.%d:%d, error %d \n",
		   pesp_conn->proto.tcp->remote_ip[0],
		   pesp_conn->proto.tcp->remote_ip[1],
		   pesp_conn->proto.tcp->remote_ip[2],
		   pesp_conn->proto.tcp->remote_ip[3],
		   pesp_conn->proto.tcp->remote_port, err);
}

static void ICACHE_FLASH_ATTR webserver_discon(void *arg)
{
    esp_ws_obj_t *pesp = (esp_ws_obj_t *)((struct espconn *)arg)->reverse;
    http_context_reset(pesp);
    gc_collect();
}

static void ICACHE_FLASH_ATTR socket_sent_callback(void *arg) {
    esp_ws_obj_t *pesp = (esp_ws_obj_t *)((struct espconn *)arg)->reverse;
    if (pesp->to_send != NULL) {
        m_free(pesp->to_send);
    }
    pesp->to_send = NULL;
    pesp->to_send_len = 0;
}

static void ICACHE_FLASH_ATTR connected(void *arg)
{
    esp_ws_obj_t *pesp = (esp_ws_obj_t *)((struct espconn *)arg)->reverse;
    if (pesp->to_send) {
        int rval;
        if ((rval = espconn_sent(&pesp->esp_conn, (uint8 *)pesp->to_send, (uint16)pesp->to_send_len)) != 0) {
            printf("error in sent rval %d\n", rval);
        }
    }
}

STATIC ICACHE_FLASH_ATTR uint16_t hdr_len(mp_obj_t obj_in, char *target) {
    uint16_t len = 0;

    if (!MP_OBJ_IS_TYPE(obj_in,  &mp_type_list)) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError, "headers are a list"));
    }
    mp_obj_list_t *al = (mp_obj_list_t *)obj_in;
    for (int ii = 0; ii < al->len; ii++) {
        if (!MP_OBJ_IS_TYPE(al->items[ii], &mp_type_tuple)) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "header items not tuples"));
        } 
        mp_obj_tuple_t *tp = al->items[ii];
        if (tp->len != 2) {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError, "header inner tuple must be len 2"));
        }
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(tp->items[0], &bufinfo, MP_BUFFER_READ);
        if (target != NULL) {
            memcpy(target + len, bufinfo.buf, bufinfo.len);
            memcpy(target + len + bufinfo.len, ": ", 2);
        }
        len += bufinfo.len + 2;  // blah + ':  '
        mp_get_buffer_raise(tp->items[1], &bufinfo, MP_BUFFER_READ);
        if (target != NULL) {
            memcpy(target + len, bufinfo.buf, bufinfo.len);
            memcpy(target + len + bufinfo.len, "\r\n", 2);
        }
        len += bufinfo.len + 2;  // blah + '\r\n'
    }
    return len;
}

STATIC const mp_arg_t esp_ws_init_args[] = {
    { MP_QSTR_callback, MP_ARG_OBJ, {.u_obj = mp_const_none}},
    { MP_QSTR_local_port, MP_ARG_INT, {.u_int = 0}},
    { MP_QSTR_remote, MP_ARG_OBJ, {.u_obj = mp_const_none}},
    { MP_QSTR_buffer_size, MP_ARG_INT, {.u_int = 200}},
    { MP_QSTR_headers, MP_ARG_OBJ, {.u_obj = mp_const_none}}

};
#define ESP_WS_INIT_NUM_ARGS MP_ARRAY_SIZE(esp_ws_init_args)

STATIC ICACHE_FLASH_ATTR mp_obj_t esp_ws_make_new(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    mp_arg_val_t vals[ESP_WS_INIT_NUM_ARGS];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, ESP_WS_INIT_NUM_ARGS, esp_ws_init_args, vals);
    
    esp_ws_obj_t *self = m_new_obj(esp_ws_obj_t);
    self->base.type = &esp_ws_type;

    self->callback = vals[0].u_obj;
	self->esp_conn.type = ESPCONN_TCP;
	self->esp_conn.state = ESPCONN_NONE;

	self->esp_conn.proto.tcp = &self->esptcp;

    if (vals[2].u_obj == mp_const_none && vals[1].u_int == 0) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError, "local_port or remote"));
    }

    if (vals[2].u_obj != mp_const_none) { // remote, put address in second argument and return port
        self->esp_conn.proto.tcp->remote_port =
            netutils_parse_inet_addr(vals[2].u_obj, self->esp_conn.proto.tcp->remote_ip,
            NETUTILS_BIG);
    }
    if (vals[1].u_int != 0) {
        self->esp_conn.proto.tcp->local_port = vals[1].u_int;
    }

    if (vals[4].u_obj != mp_const_none) {
        self->outgoing_headers_len = hdr_len(vals[4].u_obj, NULL);
        self->outgoing_headers_str = (char *)m_malloc(self->outgoing_headers_len);
        hdr_len(vals[4].u_obj, self->outgoing_headers_str);
        self->outgoing_headers = vals[4].u_obj;
    } else {
        self->outgoing_headers_len = 0;
        self->outgoing_headers = mp_const_none;
        self->outgoing_headers_str = NULL;
    }

    self->esp_conn.reverse = self;
    self->accepting = false;
    self->len = vals[3].u_int;
    self->buffer = (char *)m_malloc(self->len);
    http_context_reset(self);
	espconn_regist_connectcb(&self->esp_conn, connected);
    espconn_regist_recvcb(&self->esp_conn, webserver_recv);
    espconn_regist_reconcb(&self->esp_conn, webserver_recon);
    espconn_regist_disconcb(&self->esp_conn, webserver_discon);
    espconn_regist_sentcb(&self->esp_conn, socket_sent_callback);
    return self;
}


STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_ws_listen(mp_obj_t self_in) {
    esp_ws_obj_t *self = self_in;

    if (self->accepting) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "can only listen once"));
    }
	espconn_accept(&self->esp_conn);
    self->accepting = true;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_ws_listen_obj, mod_esp_ws_listen);

// send method GET, POST, default 'GET', URL, default '/', body, default none

STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_ws_async_send(mp_obj_t self_in, mp_obj_t buf_in) {
    esp_ws_obj_t *self = self_in;

    const char *body_base = "GET / HTTP/1.1\r\n";
    uint16_t body_base_size = strlen(body_base);
    esp_make_http_to_send(self, mp_const_none, body_base, body_base_size);

    // TODO: manage this return value (and similarly others
    espconn_connect(&self->esp_conn);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_esp_ws_async_send_obj, mod_esp_ws_async_send);


STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_ws_headers(mp_obj_t self_in) {
    esp_ws_obj_t *self = self_in;
    return self->headers;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_ws_headers_obj, mod_esp_ws_headers);

STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_ws_body(mp_obj_t self_in) {
    esp_ws_obj_t *self = self_in;
    return self->body;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_ws_body_obj, mod_esp_ws_body);

STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_ws_uri(mp_obj_t self_in) {
    esp_ws_obj_t *self = self_in;
    return self->uri;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_ws_uri_obj, mod_esp_ws_uri);

STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_ws_method(mp_obj_t self_in) {
    esp_ws_obj_t *self = self_in;
    return self->str_method;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_ws_method_obj, mod_esp_ws_method);

STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_ws_status(mp_obj_t self_in) {
    esp_ws_obj_t *self = self_in;
    return self->str_status;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_ws_status_obj, mod_esp_ws_status);

STATIC const mp_map_elem_t esp_ws_locals_dict_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR_listen), (mp_obj_t)&mod_esp_ws_listen_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_async_send), (mp_obj_t)&mod_esp_ws_async_send_obj},
    // the following could go into a dictionary like the uwsi environ, then I can use 'body' in blah
    {MP_OBJ_NEW_QSTR(MP_QSTR_body), (mp_obj_t)&mod_esp_ws_body_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_headers), (mp_obj_t)&mod_esp_ws_headers_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_uri), (mp_obj_t)&mod_esp_ws_uri_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_method), (mp_obj_t)&mod_esp_ws_method_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_status), (mp_obj_t)&mod_esp_ws_status_obj}
};
STATIC MP_DEFINE_CONST_DICT(esp_ws_locals_dict, esp_ws_locals_dict_table);

const mp_obj_type_t esp_ws_type = {
    { &mp_type_type },
    .name = MP_QSTR_ws,
//    .print = mod_esp_ws_print,
    .make_new = esp_ws_make_new,
    .locals_dict = (mp_obj_t)&esp_ws_locals_dict,
};

