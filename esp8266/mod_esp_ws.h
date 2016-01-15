#ifndef _INCLUDED_MOD_ES_WS_H_
#define _INCLUDED_MOD_ES_WS_H_

typedef struct ctx_s {
    char *buffer;
    char *ptr;
    int bp;
    int len;
    enum {
        method, uri, http_version, header_key,
        header_sep, header_val, possible_body, body_sep, body
    } state;
} ctx_t;

#endif // _INCLUDED_MOD_ES_WS_H_
