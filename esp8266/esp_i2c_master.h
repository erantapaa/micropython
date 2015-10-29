#ifndef __ESP_I2C_MASTER_H__
#define __ESP_I2C_MASTER_H__

typedef struct struct_esp_i2c {
    struct pmap_s *sda_pmp;
    struct pmap_s *scl_pmp;
    uint8 m_nLastSDA;
    uint8 m_nLastSCL;
} esp_i2c_t;


extern esp_i2c_t *i2c_master_gpio_init(esp_i2c_t *esp_i2c, uint8_t sda_pin, uint8_t scl_pin);
extern void i2c_master_init(esp_i2c_t *esp_i2c);

extern void i2c_master_stop(esp_i2c_t *esp_i2c);
extern void i2c_master_start(esp_i2c_t *esp_i2c);
extern void i2c_master_setAck(esp_i2c_t *esp_i2c, uint8 level);
extern uint8 i2c_master_getAck(esp_i2c_t *esp_i2c);
extern uint8 i2c_master_readByte(esp_i2c_t *esp_i2c);
extern void i2c_master_writeByte(esp_i2c_t *esp_i2c, uint8 wrdata);

extern bool i2c_master_checkAck(esp_i2c_t *esp_i2c);
extern void i2c_master_send_ack(esp_i2c_t *esp_i2c);
extern void i2c_master_send_nack(esp_i2c_t *esp_i2c);

#endif
