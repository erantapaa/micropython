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

#include "mod_esp_ws.h"

#define IBLEN 200

extern int skip_atoi(char **nptr);

void ICACHE_FLASH_ATTR ctx_reset(esp_ws_obj_t *ctx) {
    ctx->ptr = ctx->buffer;
    ctx->state = method;
    ctx->content_length = 0;
    ctx->method = none;
    ctx->header_key = mp_const_none;
    ctx->header_val = mp_const_none;
    ctx->header_kp = mp_const_none;
    ctx->headers = mp_const_none; // mp_obj_new_list(0, NULL);
    printf("reset\n");
}


#if 0
void ICACHE_FLASH_ATTR ctx_free(esp_ws_obj_t *ctx) {
    m_free(ctx->buffer);
    ctx->buffer = NULL;
    m_free(ctx);
}
#endif

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



static  ICACHE_FLASH_ATTR void webserver_recv(void *arg, char *pusrdata, unsigned short length) {
    struct espconn *pesp_conn = arg;
    esp_ws_obj_t *pesp = (esp_ws_obj_t *)((struct espconn *)arg)->reverse;

    for (int pos = 0; pos < length; pos++) {
        uint8_t cc = pusrdata[pos];
        // printf("state %d '%c'\n", ctx->state, cc);
        if (cc == '\r') {
            continue;
        }
        switch (pesp->state) {
        case method:
            if (cc == ' ') {
                char *method = ctx_get_reset(pesp);
                // printf("method '%s'\n", method);
                pesp->method = strcmp(method, "GET") == 0 ? get : other;
                pesp->state = uri;
            } else {
                ctx_add(pesp, cc);
            }
            break;
        case uri:
            if (cc == ' ') {
                // printf("uri '%s'\n", ctx_get_reset(pesp));
                pesp->state = http_version;
            } else {
                ctx_add(pesp, cc);
            }
            break;
        case http_version:
            if (cc == '\n') {
                // printf("version '%s'\n", ctx_get_reset(pesp));
                pesp->state = header_key;
            } else {
                ctx_add(pesp, cc);
            }
            break;
        case header_key:
            if (cc == ':') {
                char *hname = ctx_get_reset(pesp);
                // printf("header key '%s'\n", hname);
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
                // printf("length body '%d'\n", pesp->content_length);
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
                // printf("header val '%s'\n", ctx_get_reset(pesp));
                char *header_val = ctx_get_reset(pesp);
                printf("header '%s'\n", header_val);
//                mp_obj_t alist[2] = {
//                    pesp->header_key,
                    pesp->header_val = mp_obj_new_str(header_val, strlen(header_val), true);
                    pesp->header_kp = mp_obj_new_list(0, NULL);
                    //mp_obj_list_append(pesp->header_kp, pesp->header_key);
                    // mp_obj_list_append(pesp->header_kp, pesp->header_val);

//                };
//                mp_obj_list_append(pesp->headers, 
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
            const char *body = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 18\r\n\r\n"
                               "{\"result\": \"OK\"}\r\n";
            printf("GET sending '%s'", body);
            espconn_sent(pesp_conn, (uint8 *)body, (uint16)strlen((const char *)body));
            printf("done\n");
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
    struct espconn *pesp_conn = arg;
    esp_ws_obj_t *pesp = (esp_ws_obj_t *)((struct espconn *)arg)->reverse;

    printf("disconnect | %d.%d.%d.%d:%d \n",
		   pesp_conn->proto.tcp->remote_ip[0],
		   pesp_conn->proto.tcp->remote_ip[1],
		   pesp_conn->proto.tcp->remote_ip[2],
		   pesp_conn->proto.tcp->remote_ip[3],
		   pesp_conn->proto.tcp->remote_port);

    if (pesp->state == body && (pesp->ptr - pesp->buffer)) {
        printf("body '%s'\n", ctx_get_reset(pesp));
    }
    printf("free context\n");
    ctx_reset(pesp);
//        ctx_free(pesp->ctx);
}
static void ICACHE_FLASH_ATTR socket_sent_callback(void *arg) {
    printf("sent\n");
}

static void ICACHE_FLASH_ATTR webserver_listen(void *arg)
{
    struct espconn *pesp_conn = arg;
    printf("connect    | %d.%d.%d.%d:%d \n",
		   pesp_conn->proto.tcp->remote_ip[0],
		   pesp_conn->proto.tcp->remote_ip[1],
		   pesp_conn->proto.tcp->remote_ip[2],
		   pesp_conn->proto.tcp->remote_ip[3],
		   pesp_conn->proto.tcp->remote_port);

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
    printf("connect\n");
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

STATIC const mp_map_elem_t esp_ws_locals_dict_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR_listen), (mp_obj_t)&mod_esp_ws_listen_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_test), (mp_obj_t)&mod_esp_ws_test_obj}
};
STATIC MP_DEFINE_CONST_DICT(esp_ws_locals_dict, esp_ws_locals_dict_table);

const mp_obj_type_t esp_ws_type = {
    { &mp_type_type },
    .name = MP_QSTR_ws,
//    .print = mod_esp_ws_print,
    .make_new = esp_ws_make_new,
    .locals_dict = (mp_obj_t)&esp_ws_locals_dict,
};

