#include <stdio.h>
#include <string.h>
#include <c_types.h>

#include "esp_frozen.h"
#include "py/lexer.h"

#if 0
typedef struct _mp_lexer_str_buf_t {
    mp_uint_t free_len;         // if > 0, src_beg will be freed when done by: m_free(src_beg, free_len)
    const char *src_beg;        // beginning of source
    const char *src_cur;        // current location in source
    const char *src_end;        // end (exclusive) of source
} mp_lexer_str_buf_t;

STATIC mp_uint_t str_buf_next_byte(mp_lexer_str_buf_t *sb) {
    if (sb->src_cur < sb->src_end) {
        return *sb->src_cur++;
    } else {
        return MP_LEXER_EOF;
    }
}

STATIC void str_buf_free(mp_lexer_str_buf_t *sb) {
    if (sb->free_len > 0) {
        m_del(char, (char*)sb->src_beg, sb->free_len);
    }
    m_del_obj(mp_lexer_str_buf_t, sb);
}

mp_lexer_t *mp_lexer_new_from_str_len(qstr src_name, const char *str, mp_uint_t len, mp_uint_t free_len) {
    mp_lexer_str_buf_t *sb = m_new_obj_maybe(mp_lexer_str_buf_t);
    if (sb == NULL) {
        return NULL;
    }
    sb->free_len = free_len;
    sb->src_beg = str;
    sb->src_cur = str;
    sb->src_end = str + len;
    return mp_lexer_new(src_name, sb, (mp_lexer_stream_next_byte_t)str_buf_next_byte, (mp_lexer_stream_close_t)str_buf_free);
}
#endif
static void ccr(const esp_frozen_t *pp) {
    const uint32_t *kk = mp_frozen_qwords + pp->offset;

    for (int ii = 0; ii < pp->size; ii++) {
        printf("%c", ((kk[ii / 4] >> ((ii & 3) << 3)) & 0xff));
    }
}

int rofl(const char *mod) {
    const esp_frozen_t *pp = mp_frozen_table;

    printf("mod is '%s'\n", mod);
    for (int ii = 0; ii < mp_frozen_table_size; ii++, pp++) {
        if (!strcmp(mod, pp->name)) {
            ccr(pp);
            // mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR_, s, pp->size, 0);
            return 1;
        }
    }
    return 0;
}
