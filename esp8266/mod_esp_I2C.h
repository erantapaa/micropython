#ifndef _INCLUDED_MOD_ESP_I2C_H_
#define _INCLUDED_MOD_ESP_I2C_H_

typedef struct _esp_I2C_obj_t {
    mp_obj_base_t base;
    uint8_t sda_pin;
    uint8_t scl_pin;
} esp_I2C_obj_t;

extern const mp_obj_type_t esp_I2C_type;
#endif // _INCLUDED_MOD_ESP_I2C_H_
