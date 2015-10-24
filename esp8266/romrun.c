#include <stdio.h>
#include <string.h>
#include <c_types.h>

#include "py/lexer.h"
#include "py/compile.h"
#include "py/runtime.h"

#include "esp_frozen.h"

static void ccr(const esp_frozen_t *pp) {
    pb_context_t *ctx = irom_make_context(pp);
#if 0
    mp_uint_t cc;

    while ((cc = irom_next_char(&ctx)) != MP_LEXER_EOF) {
        printf("%c", cc);
    }
    close(&ctx);
#else
    mp_lexer_t *lex;
    lex = mp_lexer_new(MP_QSTR_, (void *)ctx, (mp_lexer_stream_next_byte_t)irom_next_char, irom_close);
    if (lex == NULL) {
        printf("out of ram?\n");
    }
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
