typedef struct esp_frozen {
    const char    *name;
    uint16_t    offset;
    uint16_t    size;
} esp_frozen_t;

extern const esp_frozen_t mp_frozen_table[];
extern const uint32_t mp_frozen_qwords[];
extern const uint16_t mp_frozen_table_size;


typedef struct pb_context {
    const esp_frozen_t *pp;
    const uint32_t *base;
    uint16_t    current_pos;
} pb_context_t;

extern pb_context_t *irom_make_context(const esp_frozen_t *pp);
extern mp_uint_t irom_next_char(void *stream_data);
extern void irom_close(void *stream_data);

//extern mp_lexer_t *mp_find_irom_frozen_module(const char *mod);
