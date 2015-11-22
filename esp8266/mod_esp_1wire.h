#ifndef _INCLUDED_MOD_ESP_1WIRE_H_
#define _INCLUDED_MOD_ESP_1WIRE_H_

typedef struct _esp_1wire_obj_t {
    mp_obj_base_t base;
    pmap_t  *gpio;
} esp_1wire_obj_t;

extern const mp_obj_type_t esp_1wire_type;
#endif // _INCLUDED_MOD_ESP_1WIRE_H_
