#ifndef _INCLUDED_MOD_ESP_QUEUE_H_
#define _INCLUDED_MOD_ESP_QUEUE_H_

typedef struct _esp_queue_obj_t {
    mp_obj_base_t base;
    mp_obj_t mutex;
	int first;
	int last;
	int items;
    int max_items;
    mp_obj_t *obj_instances;
} esp_queue_obj_t;

extern const mp_obj_type_t esp_queue_type;

extern int8_t esp_queue_daint_8(esp_queue_obj_t *queue_in, uint8_t value);
extern int8_t esp_queue_dalist_8(esp_queue_obj_t *queue_in, uint32_t len, uint8_t *vals);
extern bool esp_queue_check_for_dalist_8(esp_queue_obj_t *queue_in, uint32_t len);
#endif // _INCLUDED_MOD_ESP_QUEUE_H_
