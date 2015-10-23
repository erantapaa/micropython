#include <stdio.h>
#include <string.h>
#include <c_types.h>

#include "esp_frozen.h"
#include "py/lexer.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/gc.h"


// static const char *test_code = "import esp\naa = 134\nprint(aa * 100)\n";

typedef struct pb_context {
    const esp_frozen_t *pp;
    const uint32_t *base;
    uint16_t    current_pos;
} pb_context_t;


STATIC void make_context(pb_context_t *ctx, const esp_frozen_t *pp) {
    ctx->pp = pp;
    ctx->base = mp_frozen_qwords + pp->offset;
    ctx->current_pos = 0;
}

STATIC void close(void *stream_data)  {
    printf("closed\n");
}

STATIC mp_uint_t next_char(void *stream_data) {
    pb_context_t *ctx = (pb_context_t *)stream_data;
    if (ctx->current_pos >= ctx->pp->size) {
        return MP_LEXER_EOF;
    }
    char val = (ctx->base[ctx->current_pos / 4] >> ((ctx->current_pos & 3) << 3)) & 0xff;
    ctx->current_pos++;
    return val;
}

static void ccr(const esp_frozen_t *pp) {
    pb_context_t ctx;
    make_context(&ctx, pp);
#if 0
    mp_uint_t cc;

    while ((cc = next_char(&ctx)) != MP_LEXER_EOF) {
        printf("%c", cc);
    }
    close(&ctx);
#else
    mp_lexer_t *lex;
    gc_dump_info();
    lex = mp_lexer_new(MP_QSTR_, (void *)&ctx, next_char, close);
    if (lex == NULL) {
        printf("out of ram?\n");
    }
    gc_dump_info();
    mp_parse_compile_execute(lex, MP_PARSE_FILE_INPUT, mp_globals_get(), mp_locals_get());
#endif
}

int rofl(const char *mod) {
    const esp_frozen_t *pp = mp_frozen_table;

    printf("mod is '%s'\n", mod);
    for (int ii = 0; ii < mp_frozen_table_size; ii++, pp++) {
        if (!strcmp(mod, pp->name)) {
            ccr(pp);
            return 1;
        }
    }
    return 0;
}
