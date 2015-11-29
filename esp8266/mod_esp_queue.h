#ifndef _INCLUDED_MOD_ESP_QUEUE_H_
#define _INCLUDED_MOD_ESP_QUEUE_H_

typedef struct _esp_queue_obj_t {
    mp_obj_base_t base;
    mp_obj_t mutex;
    queue_t *q;
} esp_queue_obj_t;

extern const mp_obj_type_t esp_queue_type;

#endif // _INCLUDED_MOD_ESP_QUEUE_H_
