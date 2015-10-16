
#ifndef _INCLUDED_MOD_ESP_MUTEX_H_
#define _INCLUDED_MOD_ESP_MUTEX_H_

typedef int32 mutex_t;

typedef struct _esp_mutex_obj_t {
    mp_obj_base_t base;
    mutex_t mutex;
} esp_mutex_obj_t;

extern const mp_obj_type_t esp_mutex_type;

#endif // _INCLUDED_MOD_ESP_MUTEX_H_
