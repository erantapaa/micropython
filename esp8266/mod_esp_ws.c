#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"

#include <stdio.h>
#include <stdlib.h>

#define LINEBUFFER_SIZE 1024
static char* linebuffer;
static int lineptr; 
static int disconn = 1; 

// void *pvPortMalloc( size_t xWantedSize, const char ){
extern void *pvPortMalloc(size_t xWantedSize, const char *file, const char *line);

static  ICACHE_FLASH_ATTR void webserver_recv(void *arg, char *pusrdata, unsigned short length)
{
    printf("receive    |");
    for (int ii  = 0; ii < length; ii++) {
        printf("%c", *pusrdata++);
    }
    printf("|\n");
    return;
	while (length--) {
		if ((*pusrdata == 13) || (*pusrdata == 10))
		{
			linebuffer[lineptr++]=0x0;
			printf("receive    | %s\n", linebuffer);
			lineptr = 0;
			pusrdata++;
			if (disconn) {
				espconn_disconnect(arg);
				return;
			}
			continue;
		} else  if (lineptr == LINEBUFFER_SIZE) {
			linebuffer[LINEBUFFER_SIZE - 1]=0x0;
			printf("receive   | %s\n", linebuffer);
			lineptr = 0;
			pusrdata++;
			if (disconn) {
				espconn_disconnect(arg);
				return;
			}
			continue;
		}
		linebuffer[lineptr++] = *pusrdata++;
	}

	
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
//	linebuffer = os_malloc(LINEBUFFER_SIZE);
	lineptr = 0;
	return 0;
}
