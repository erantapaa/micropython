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

extern void *pvPortMalloc(size_t xWantedSize, const char *file, const char *line);

ctx_t *new_ctx(int len) {
    ctx_t *ctx = (ctx_t *)pvPortMalloc(sizeof (ctx_t), "", "");
    ctx->buffer = (char *)pvPortMalloc(len, "", "");
    ctx->len = len;
    ctx->ptr = ctx->buffer;
    ctx->state = method;
    return ctx;
}

void    add(ctx_t *ctx, char cc) {
    if (ctx->ptr - ctx->buffer > ctx->len) {
        printf("overflow\n");
        return;
    }
    *ctx->ptr++ = cc;
}

char *get(ctx_t *ctx) {
    add(ctx, '\0');
    ctx->ptr = ctx->buffer;
    return ctx->buffer;
}



static  ICACHE_FLASH_ATTR void webserver_recv(void *arg, char *pusrdata, unsigned short length) {
#if 0
    printf("receive    |");
    for (int ii  = 0; ii < length; ii++) {
        printf("%c", *pusrdata++);
    }
    printf("|\n");
#endif

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
                printf("method '%s'\n", get(ctx));
                ctx->state = uri;
            } else {
                add(ctx, cc);
            }
            break;
        case uri:
            if (cc == ' ') {
                printf("uri '%s'\n", get(ctx));
                ctx->state = http_version;
            } else {
                add(ctx, cc);
            }
            break;
        case http_version:
            if (cc == '\n') {
                printf("version '%s'\n", get(ctx));
                ctx->state = header_key;
            } else {
                add(ctx, cc);
            }
            break;
        case header_key:
            if (cc == ':') {
                printf("header key '%s'\n", get(ctx));
                ctx->state = header_sep;
            } else {
                add(ctx, cc);
            }
            break;
        case header_sep:
            if (cc != ' ') {
                add(ctx, cc);
                ctx->state = header_val;
            }
            break;
        case header_val:
            if (cc == '\n') {
                printf("header val '%s'\n", get(ctx));
                ctx->state = possible_body;
            } else {
                add(ctx, cc);
            }
            break;
        case possible_body:
            if (cc != '\n') {
                add(ctx, cc);
                ctx->state = header_key;
            } else {
                ctx->state = body_sep;
            }
            break;
        case body_sep:
            if (cc != '\n') {
                add(ctx, cc);
                ctx->state = body;
            } // else bad state
            break;
        case body:
            if (cc == -1) {
                printf("body '%s'\n", get(ctx));
                exit(0);
            } else {
                add(ctx, cc);
            }
            break;
        }
        
    }
    if (ctx->state == body) {
        printf("body '%s'\n", get(ctx));
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

    pesp_conn->reverse = new_ctx(IBLEN);
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
