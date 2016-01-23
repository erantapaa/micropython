#ifndef _INCLUDED_MOD_ES_WS_H_
#define _INCLUDED_MOD_ES_WS_H_

typedef struct ctx_s {
    char *buffer;
    char *ptr;
    int bp;
    int len;
    enum {
        method = 0, uri = 1, http_version = 2, header_key = 3,
        header_sep = 4, header_val = 5, content_length_sep = 6, content_length = 7, possible_body = 8, body_sep = 9, body = 10
    } state;
    int content_length;
    enum { get, other, none}  method;
    mp_obj_t header_key;
    mp_obj_t headers;
} ctx_t;

typedef struct _esp_ws_obj_t {
    mp_obj_base_t base;
    struct espconn esp_conn;
    esp_tcp esptcp;
    bool accepting;
    mp_obj_t callback;
} esp_ws_obj_t;

extern const mp_obj_type_t esp_ws_type;
#endif // _INCLUDED_MOD_ES_WS_H_
