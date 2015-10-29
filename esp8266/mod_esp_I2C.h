#ifndef _INCLUDED_MOD_ESP_I2C_H_
#define _INCLUDED_MOD_ESP_I2C_H_

typedef struct _esp_I2C_obj_t {
    mp_obj_base_t base;
    esp_i2c_t esp_i2c;
} esp_I2C_obj_t;

extern const mp_obj_type_t esp_I2C_type;
#endif // _INCLUDED_MOD_ESP_I2C_H_
