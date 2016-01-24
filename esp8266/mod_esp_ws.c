#include <stdio.h>
#include <stdlib.h>

#include "../py/runtime.h"
#include "../py/obj.h"
#include "../py/objstr.h"
#include "../py/misc.h"

#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"

#include "gccollect.h"

#include "mod_esp_ws.h"

#define IBLEN 200

extern int skip_atoi(char **nptr);
int ets_sprintf(char *str, const char *format, ...);


void ICACHE_FLASH_ATTR ctx_reset(esp_ws_obj_t *ctx) {
    ctx->ptr = ctx->buffer;
    ctx->state = method;
    ctx->content_length = 0;
    ctx->method = none;
    ctx->header_key = mp_const_none;
    ctx->headers = mp_obj_new_list(0, NULL);
    ctx->uri = mp_const_none;
    ctx->body = mp_const_none;
    ctx->str_method = mp_const_none;
}


void  ICACHE_FLASH_ATTR ctx_add(esp_ws_obj_t *ctx, char cc) {
    if (ctx->ptr - ctx->buffer > ctx->len) {
        printf("overflow\n");
        return;
    }
    *ctx->ptr++ = cc;
}

char ICACHE_FLASH_ATTR *ctx_get_reset(esp_ws_obj_t *ctx) {
    ctx_add(ctx, '\0');
    ctx->ptr = ctx->buffer;
    return ctx->buffer;
}

char ICACHE_FLASH_ATTR *ctx_str_reset(esp_ws_obj_t *ctx) {
    mp_obj_t bp = mp_obj_new_str(ctx->buffer, ctx->ptr - ctx->buffer, true);
    ctx->ptr = ctx->buffer;
    return bp;
}

void ICACHE_FLASH_ATTR ws_reply(esp_ws_obj_t *pesp, struct espconn *pesp_conn) {
    mp_obj_t reply = mp_const_none;

    if (pesp->callback) {
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            reply = mp_call_function_1(pesp->callback, pesp);
        } else {
           mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
           return;
        }
    }
    mp_obj_str_t *mp_str = (mp_obj_str_t *)reply;
    if (reply != mp_const_none) {
        if (!MP_OBJ_IS_TYPE(reply,  &mp_type_str)) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "ws handler can only return str (or None)"));
            return;
        }
        mp_str = (mp_obj_str_t *)reply;
    }
    const char *body_base = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: ";
    int body_base_size = strlen(body_base);

    int  body_size = reply == mp_const_none ? 0 : mp_str->len;

    char body_size_str[20];
    ets_sprintf(body_size_str, "%d\r\n\r\n", body_size);
    int body_size_str_len = strlen(body_size_str);

    int total_len = body_base_size + body_size_str_len + body_size;

    pesp->to_send = (char *)m_malloc(total_len);

    strcpy(pesp->to_send, body_base);
    strcpy(pesp->to_send + body_base_size, body_size_str);
    if (body_size) {
        strcpy(pesp->to_send + body_base_size + body_size_str_len, (char *)mp_str->data); // todo, mp_str data may not be null terminated
    }
//    printf("sending '%s'\n", total_reply);
    espconn_sent(pesp_conn, (uint8 *)pesp->to_send, (uint16)total_len);

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
        case method:
            if (cc == ' ') {
                char *method = ctx_get_reset(pesp);
                pesp->str_method =  mp_obj_new_str(method, strlen(method), true);

                pesp->method = strcmp(method, "GET") == 0 ? get : other;
                pesp->state = uri;
            } else {
                ctx_add(pesp, cc);
            }
            break;
        case uri:
            if (cc == ' ') {
                pesp->uri = ctx_str_reset(pesp);
                pesp->state = http_version;
            } else {
                ctx_add(pesp, cc);
            }
            break;
        case http_version:
            if (cc == '\n') {
                (void)ctx_get_reset(pesp);
                pesp->state = header_key;
            } else {
                ctx_add(pesp, cc);
            }
            break;
        case header_key:
            if (cc == ':') {
                char *hname = ctx_get_reset(pesp);
                if (strcmp("Content-Length", hname) == 0) {
                    pesp->state = content_length_sep;
                } else {
                    pesp->header_key = mp_obj_new_str(hname, strlen(hname), true);
                    pesp->state = header_sep;
               }
            } else {
                ctx_add(pesp, cc);
            }
            break;
        case content_length_sep:
            if (cc != ' ') {
                ctx_add(pesp, cc);
                pesp->state = content_length;
            }
            break;
        case content_length:
            if (cc == '\n') {
                char *clength = ctx_get_reset(pesp);
                pesp->content_length = skip_atoi(&clength);
                pesp->state = possible_body;
            } else {
                ctx_add(pesp, cc);
            }
            break;

        case header_sep:
            if (cc != ' ') {
                ctx_add(pesp, cc);
                pesp->state = header_val;
            }
            break;
        case header_val:
            if (cc == '\n') {
                mp_obj_t atuple[2] = {
                    pesp->header_key,
                    ctx_str_reset(pesp)
                };
                mp_obj_list_append(pesp->headers, mp_obj_new_tuple(2, atuple));
                pesp->state = possible_body;
            } else {
                ctx_add(pesp, cc);
            }
            break;
        case possible_body:
            if (cc != '\n') {
                ctx_add(pesp, cc);
                pesp->state = header_key;
            } else {
                pesp->state = body_sep;
            }
            break;
        case body_sep:
            if (cc != '\n') {
                ctx_add(pesp, cc);
                pesp->state = body;
            } // else bad state
            break;
        case body:
            ctx_add(pesp, cc);
            break;
        }
    }


    if (pesp->method == get && pesp->state == body_sep) {
        ws_reply(pesp, pesp_conn);
    } else if (pesp->state == body) {
        int size = pesp->ptr - pesp->buffer;
        if (size) {
            printf("expect %d got %d\n", pesp->content_length, size);
            if (pesp->content_length == pesp->content_length) {
                printf("length body '%s'\n", ctx_get_reset(pesp));
                const char *body = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 18\r\n\r\n"
                                   "{\"result\": \"OK\"}\r\n";
                printf("sending '%s'", body);
                espconn_sent(pesp_conn, (uint8 *)body, (uint16)strlen((const char *)body));
            } else {
                printf("body '%s'\n", ctx_get_reset(pesp));
            }
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
//    struct espconn *pesp_conn = arg;
    esp_ws_obj_t *pesp = (esp_ws_obj_t *)((struct espconn *)arg)->reverse;

#if 0
    printf("disconnect | %d.%d.%d.%d:%d \n",
		   pesp_conn->proto.tcp->remote_ip[0],
		   pesp_conn->proto.tcp->remote_ip[1],
		   pesp_conn->proto.tcp->remote_ip[2],
		   pesp_conn->proto.tcp->remote_ip[3],
		   pesp_conn->proto.tcp->remote_port);
#endif
    if (pesp->state == body && (pesp->ptr - pesp->buffer)) {
        printf("body '%s'\n", ctx_get_reset(pesp));
    }
    ctx_reset(pesp);
    gc_collect();
}
static void ICACHE_FLASH_ATTR socket_sent_callback(void *arg) {
    esp_ws_obj_t *pesp = (esp_ws_obj_t *)((struct espconn *)arg)->reverse;
    m_free(pesp->to_send);
}

static void ICACHE_FLASH_ATTR webserver_listen(void *arg)
{
    struct espconn *pesp_conn = arg;
#if 0
    printf("connect    | %d.%d.%d.%d:%d \n",
		   pesp_conn->proto.tcp->remote_ip[0],
		   pesp_conn->proto.tcp->remote_ip[1],
		   pesp_conn->proto.tcp->remote_ip[2],
		   pesp_conn->proto.tcp->remote_ip[3],
		   pesp_conn->proto.tcp->remote_port);
#endif
//    esp_ws_obj_t *pesp = (esp_ws_obj_t *)((struct espconn *)arg)->reverse;
    espconn_regist_recvcb(pesp_conn, webserver_recv);
    espconn_regist_reconcb(pesp_conn, webserver_recon);
    espconn_regist_disconcb(pesp_conn, webserver_discon);
    espconn_regist_sentcb(pesp_conn, socket_sent_callback);

}

STATIC const mp_arg_t esp_ws_init_args[] = {
    { MP_QSTR_callback, MP_ARG_OBJ, {.u_obj = mp_const_none}},
    { MP_QSTR_port, MP_ARG_INT, {.u_int = 80}},
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
	self->esp_conn.proto.tcp->local_port = vals[1].u_int;
    self->esp_conn.reverse = self;
    self->accepting = false;
    self->buffer = (char *)m_malloc(IBLEN);
    self->len = IBLEN;
    ctx_reset(self);
	espconn_regist_connectcb(&self->esp_conn, webserver_listen);
    return self;
}


STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_ws_listen(mp_obj_t self_in) {
    esp_ws_obj_t *self = self_in;

	espconn_accept(&self->esp_conn);
    self->accepting = true;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_ws_listen_obj, mod_esp_ws_listen);

STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_ws_test(mp_obj_t self_in) {
//    esp_ws_obj_t *self = self_in;

    mp_obj_t alist[2] = {
        mp_obj_new_str("help", 4, false),
        mp_obj_new_str("crap", 4, false)
    };
    mp_obj_t it = mp_obj_new_list(0, NULL);
    for (int ii = 0; ii < 10; ii++) {
        mp_obj_list_append(it, mp_obj_new_list(2, alist));
    }
    return it;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_ws_test_obj, mod_esp_ws_test);

STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_ws_headers(mp_obj_t self_in) {
    esp_ws_obj_t *self = self_in;
    return self->headers;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_esp_ws_headers_obj, mod_esp_ws_headers);

STATIC ICACHE_FLASH_ATTR mp_obj_t mod_esp_ws_body(mp_obj_t self_in) {
//    esp_ws_obj_t *self = self_in;
    return mp_const_none;
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

STATIC const mp_map_elem_t esp_ws_locals_dict_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR_listen), (mp_obj_t)&mod_esp_ws_listen_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_test), (mp_obj_t)&mod_esp_ws_test_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_body), (mp_obj_t)&mod_esp_ws_body_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_headers), (mp_obj_t)&mod_esp_ws_headers_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_uri), (mp_obj_t)&mod_esp_ws_uri_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_method), (mp_obj_t)&mod_esp_ws_method_obj}
};
STATIC MP_DEFINE_CONST_DICT(esp_ws_locals_dict, esp_ws_locals_dict_table);

const mp_obj_type_t esp_ws_type = {
    { &mp_type_type },
    .name = MP_QSTR_ws,
//    .print = mod_esp_ws_print,
    .make_new = esp_ws_make_new,
    .locals_dict = (mp_obj_t)&esp_ws_locals_dict,
};

