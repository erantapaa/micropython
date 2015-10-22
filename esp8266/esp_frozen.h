typedef struct esp_frozen {
    const char    *name;
    uint16_t    offset;
    uint16_t    size;
} esp_frozen_t;

extern const esp_frozen_t mp_frozen_table[];
extern const uint32_t mp_frozen_qwords[];
extern const uint16_t mp_frozen_table_size;
