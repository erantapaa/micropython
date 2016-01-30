#ifndef _INCLUDED_MOD_ES_WS_H_
#define _INCLUDED_MOD_ES_WS_H_

typedef struct _esp_ws_obj_t {
    mp_obj_base_t base;
    // incoming data buffer, current pointer and buffer size
    char *buffer;
    char *ptr;
    int len;
    // web server states
    enum {
        method_or_reply_http = 0, uri = 1, http_version = 2, header_key = 3,
        header_sep = 4, header_val = 5, content_length_sep = 6, content_length = 7, possible_body = 8, body_sep = 9, body = 10, status_code = 11
    } state;
    // info saved to be sent to the application
    int content_length;
    enum { get, other, none, reply}  method;
    mp_obj_t header_key;
   // mp_obj_t header_val;
    mp_obj_t header_kp;
    mp_obj_t headers;
    mp_obj_t uri;
    mp_obj_t str_method;
    mp_obj_t str_status;
    mp_obj_t body;
    // function called on reception
    mp_obj_t callback;

    // esp internal state
    struct espconn esp_conn;
    esp_tcp esptcp;
    //
    bool accepting;
    // this holds an allocated block that is passed to 'sent' for the esp runtime to send
    char *to_send;
    int to_send_len;
    // extra outgoing headers
    char *outgoing_headers_str;
    mp_obj_t outgoing_headers;
    int outgoing_headers_len;
} esp_ws_obj_t;

extern const mp_obj_type_t esp_ws_type;
#endif // _INCLUDED_MOD_ES_WS_H_
