#include <stdio.h>
#include <string.h>
#include <c_types.h>

#include "py/lexer.h"
#include "py/runtime.h"
#include "esp_frozen.h"



pb_context_t *irom_make_context(const esp_frozen_t *pp) {
    pb_context_t *ctx = m_new_obj_maybe(pb_context_t);
    if (ctx == NULL) {
        printf("ctx allocation failed\n");
        return NULL;
    }
    ctx->pp = pp;
    ctx->base = mp_frozen_qwords + pp->offset;
    ctx->current_pos = 0;
    return ctx;
}

void irom_close(void *stream_data)  {
    pb_context_t *ctx = (pb_context_t *)stream_data;
    m_del_obj(pb_context_t, ctx);
}

mp_uint_t irom_next_char(void *stream_data) {
    pb_context_t *ctx = (pb_context_t *)stream_data;
    if (ctx->current_pos >= ctx->pp->size) {
        return MP_LEXER_EOF;
    }
    char val = (ctx->base[ctx->current_pos / 4] >> ((ctx->current_pos & 3) << 3)) & 0xff;
    ctx->current_pos++;
    return val;
}

// len not used
mp_lexer_t *mp_find_frozen_module(const char *mod, int len) {
    const esp_frozen_t *pp = mp_frozen_table;

    for (int ii = 0; ii < mp_frozen_table_size; ii++, pp++) {
        if (!strcmp(mod, pp->name)) {
            pb_context_t *ctx = irom_make_context(pp);
            return mp_lexer_new(MP_QSTR_, (void *)ctx, (mp_lexer_stream_next_byte_t)irom_next_char, irom_close);
        }
    }
    return NULL;
}
