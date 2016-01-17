#include <stdio.h>
#include <stdlib.h>

#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"

#include "mod_esp_ws.h"

#define IBLEN 200

extern void *pvPortMalloc(size_t xWantedSize, const char *file, int line);
extern void vPortFree(void *ptr, const char *file, int line);
extern void *pvPortRealloc(size_t xWantedSize, const char *file, int line);
extern int skip_atoi(char **nptr);

void ICACHE_FLASH_ATTR ctx_reset(ctx_t *ctx) {
    ctx->ptr = ctx->buffer;
    ctx->state = method;
    ctx->content_length = 0;
    ctx->method = none;
}

ctx_t ICACHE_FLASH_ATTR *ctx_new(int len) {
    ctx_t *ctx = (ctx_t *)os_malloc(sizeof (ctx_t));
    ctx->buffer = (char *)os_malloc(len);
    ctx->len = len;
    ctx_reset(ctx);
    return ctx;
}

void ICACHE_FLASH_ATTR ctx_free(ctx_t *ctx) {
    os_free(ctx->buffer);
    ctx->buffer = NULL;
    os_free(ctx);
}

void  ICACHE_FLASH_ATTR ctx_add(ctx_t *ctx, char cc) {
    if (ctx->ptr - ctx->buffer > ctx->len) {
        printf("overflow\n");
        return;
    }
    *ctx->ptr++ = cc;
}

char ICACHE_FLASH_ATTR *ctx_get_reset(ctx_t *ctx) {
    ctx_add(ctx, '\0');
    ctx->ptr = ctx->buffer;
    return ctx->buffer;
}



static  ICACHE_FLASH_ATTR void webserver_recv(void *arg, char *pusrdata, unsigned short length) {
    struct espconn *pesp_conn = arg;
    ctx_t *ctx = (ctx_t *)pesp_conn->reverse;

    for (int pos = 0; pos < length; pos++) {
        uint8_t cc = pusrdata[pos];

        if (cc == '\r') {
            continue;
        }
        switch (ctx->state) {
        case method:
            if (cc == ' ') {
                char *method = ctx_get_reset(ctx);
                printf("method '%s'\n", method);
                ctx->method = strcmp(method, "GET") == 0 ? get : other;
                ctx->state = uri;
            } else {
                ctx_add(ctx, cc);
            }
            break;
        case uri:
            if (cc == ' ') {
                printf("uri '%s'\n", ctx_get_reset(ctx));
                ctx->state = http_version;
            } else {
                ctx_add(ctx, cc);
            }
            break;
        case http_version:
            if (cc == '\n') {
                printf("version '%s'\n", ctx_get_reset(ctx));
                ctx->state = header_key;
            } else {
                ctx_add(ctx, cc);
            }
            break;
        case header_key:
            if (cc == ':') {
                char *hname = ctx_get_reset(ctx);
                printf("header key '%s'\n", hname);
                if (strcmp("Content-Length", hname) == 0) {
                    ctx->state = content_length_sep;
                } else {
                    ctx->state = header_sep;
               }
            } else {
                ctx_add(ctx, cc);
            }
            break;
        case content_length_sep:
            if (cc != ' ') {
                ctx_add(ctx, cc);
                ctx->state = content_length;
            }
            break;
        case content_length:
            if (cc == '\n') {
                char *clength = ctx_get_reset(ctx);
                ctx->content_length = skip_atoi(&clength);
                ctx->state = possible_body;
            } else {
                ctx_add(ctx, cc);
            }
            break;

        case header_sep:
            if (cc != ' ') {
                ctx_add(ctx, cc);
                ctx->state = header_val;
            }
            break;
        case header_val:
            if (cc == '\n') {
                printf("header val '%s'\n", ctx_get_reset(ctx));
                ctx->state = possible_body;
            } else {
                ctx_add(ctx, cc);
            }
            break;
        case possible_body:
            if (cc != '\n') {
                ctx_add(ctx, cc);
                ctx->state = header_key;
            } else {
                ctx->state = body_sep;
            }
            break;
        case body_sep:
            if (cc != '\n') {
                ctx_add(ctx, cc);
                ctx->state = body;
            } // else bad state
            break;
        case body:
            ctx_add(ctx, cc);
            break;
        }
    }
    if (ctx->method == get && ctx->state == body_sep) {
            printf("length body '%s'\n", ctx_get_reset(ctx));
            const char *body = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 18\r\n\r\n"
                               "{\"result\": \"OK\"}\r\n";
            printf("GET sending '%s'", body);
            espconn_sent(pesp_conn, (uint8 *)body, (uint16)strlen((const char *)body));
    } else if (ctx->state == body) {
        int size = ctx->ptr - ctx->buffer;
        if (size) {
            printf("expect %d got %d\n", ctx->content_length, size);
            if (ctx->content_length == ctx->content_length) {
                printf("length body '%s'\n", ctx_get_reset(ctx));
                const char *body = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 18\r\n\r\n"
                                   "{\"result\": \"OK\"}\r\n";
                printf("sending '%s'", body);
                espconn_sent(pesp_conn, (uint8 *)body, (uint16)strlen((const char *)body));
            } else {
                printf("body '%s'\n", ctx_get_reset(ctx));
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

    ctx_t *ctx = (ctx_t *)pesp_conn->reverse;
    if (!ctx) {
        printf("no context");
    } else {
        if (ctx->state == body && (ctx->ptr - ctx->buffer)) {
            printf("body '%s'\n", ctx_get_reset(ctx));
        }
        ctx_free(ctx);
    }
    printf("disconnect | %d.%d.%d.%d:%d \n",
		   pesp_conn->proto.tcp->remote_ip[0],
		   pesp_conn->proto.tcp->remote_ip[1],
		   pesp_conn->proto.tcp->remote_ip[2],
		   pesp_conn->proto.tcp->remote_ip[3],
		   pesp_conn->proto.tcp->remote_port);
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

    pesp_conn->reverse = ctx_new(IBLEN);
    espconn_regist_recvcb(pesp_conn, webserver_recv);
    espconn_regist_reconcb(pesp_conn, webserver_recon);
    espconn_regist_disconcb(pesp_conn, webserver_discon);

}

static struct espconn esp_conn;
static esp_tcp esptcp;

int ICACHE_FLASH_ATTR  do_listen(int port) {
	printf("Listening (TCP) on port %d\n", port);
	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = port;
	espconn_regist_connectcb(&esp_conn, webserver_listen);
	espconn_accept(&esp_conn);
	return 0;
}
