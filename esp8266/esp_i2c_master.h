#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

typedef struct struct_esp_i2c {
    pmap_t *sda_pmp;
    pmap_t *scl_pmp;
    uint8 m_nLastSDA;
    uint8 m_nLastSCL;
} esp_i2c_t;

//#define I2C_MASTER_SDA_MUX PERIPHS_IO_MUX_GPIO2_U
// #define I2C_MASTER_SCL_MUX PERIPHS_IO_MUX_MTMS_U
// #define I2C_MASTER_SDA_GPIO 2
// #define I2C_MASTER_SCL_GPIO 14
// #define I2C_MASTER_SDA_FUNC FUNC_GPIO2
// #define I2C_MASTER_SCL_FUNC FUNC_GPIO14

//#define I2C_MASTER_SDA_MUX PERIPHS_IO_MUX_GPIO2_U
//#define I2C_MASTER_SCL_MUX PERIPHS_IO_MUX_GPIO0_U
//#define I2C_MASTER_SDA_GPIO 2
//#define I2C_MASTER_SCL_GPIO 0
//#define I2C_MASTER_SDA_FUNC FUNC_GPIO2
//#define I2C_MASTER_SCL_FUNC FUNC_GPIO0


#define I2C_MASTER_SDA_HIGH_SCL_HIGH(i2c)  \
    gpio_output_set(1<<i2c->sda_pmp->pin | 1<<i2c->scl_pmp->pin, 0, 1<<i2c->sda_pmp->pin | 1<<i2c->scl_pmp->pin, 0)
//   gpio_output_set(1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

#define I2C_MASTER_SDA_HIGH_SCL_LOW(i2c)  \
    gpio_output_set(1<<i2c->sda_pmp->pin, 1<<i2c->scl_pmp->pin, 1<<i2c->sda_pmp->pin | 1<<i2c->scl_pmp->pin, 0)
//  gpio_output_set(1<<I2C_MASTER_SDA_GPIO, 1<<I2C_MASTER_SCL_GPIO, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

#define I2C_MASTER_SDA_LOW_SCL_HIGH(i2c)  \
    gpio_output_set(1<<i2c->scl_pmp->pin, 1<<i2c->sda_pmp->pin, 1<<i2c->sda_pmp->pin | 1<<i2c->scl_pmp->pin, 0)
//  gpio_output_set(1<<I2C_MASTER_SCL_GPIO, 1<<I2C_MASTER_SDA_GPIO, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

#define I2C_MASTER_SDA_LOW_SCL_LOW(i2c)  \
    gpio_output_set(0, 1<<i2c->sda_pmp->pin | 1<<i2c->scl_pmp->pin, 1<<i2c->sda_pmp->pin | 1<<i2c->scl_pmp->pin, 0)
//  gpio_output_set(0, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)


esp_i2c_t ICACHE_FLASH_ATTR *i2c_master_gpio_init(uint8_t sda_pin, uint8_t scl_pin);
void ICACHE_FLASH_ATTR i2c_master_init(esp_i2c_t *esp_i2c);

#define i2c_master_wait    os_delay_us
void i2c_master_stop(esp_i2c_t *esp_i2c);
void i2c_master_start(esp_i2c_t *esp_i2c);
void i2c_master_setAck(esp_i2c_t *esp_i2, uint8 level);
uint8 i2c_master_getAck(esp_i2c_t *esp_i2c);
uint8 i2c_master_readByte(esp_i2c_t *esp_i2c);
void i2c_master_writeByte(esp_i2c_t *esp_i2, uint8 wrdata);

bool i2c_master_checkAck(esp_i2c_t *esp_i2c);
void i2c_master_send_ack(esp_i2c_t *esp_i2c);
void i2c_master_send_nack(esp_i2c_t *esp_i2c);

#endif
