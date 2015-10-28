#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

typedef struct struct_esp_i2c {
    pmap_t *sda_pmp;
    pmap_t *scl_pmp;
    uint8 m_nLastSDA;
    uint8 m_nLastSCL;
} esp_i2c_t;


//#define I2C_MASTER_SDA_MUX PERIPHS_IO_MUX_GPIO2_U
//#define I2C_MASTER_SCL_MUX PERIPHS_IO_MUX_MTMS_U
//#define I2C_MASTER_SDA_GPIO 2
//#define I2C_MASTER_SCL_GPIO 14
//#define I2C_MASTER_SDA_FUNC FUNC_GPIO2
//#define I2C_MASTER_SCL_FUNC FUNC_GPIO14


esp_i2c_t ICACHE_FLASH_ATTR *i2c_master_gpio_init(esp_i2c_t *esp_i2c, uint8_t sda_pin, uint8_t scl_pin);
void i2c_master_init(esp_i2c_t *esp_i2c);

void i2c_master_stop(esp_i2c_t *esp_i2c);
void i2c_master_start(esp_i2c_t *esp_i2c);
void i2c_master_setAck(esp_i2c_t *esp_i2c, uint8 level);
uint8 i2c_master_getAck(esp_i2c_t *esp_i2c);
uint8 i2c_master_readByte(esp_i2c_t *esp_i2c);
void i2c_master_writeByte(esp_i2c_t *esp_i2c, uint8 wrdata);

bool i2c_master_checkAck(esp_i2c_t *esp_i2c);
void i2c_master_send_ack(esp_i2c_t *esp_i2c);
void i2c_master_send_nack(esp_i2c_t *esp_i2c);

#endif
