#include <stdio.h>
#include <time.h>
#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <gpio.h>
#include <user_interface.h>
#include <espconn.h>
#include <mem.h>

#include "py/obj.h"

#include "mod_esp_gpio.h"
#include "esp_i2c_master.h"

#define DS3231_ADDR 0x68
#define DS3231_ADDR_TEMP    0x11


// send a number of bytes to the rtc over i2c
// returns true to indicate success
bool ICACHE_FLASH_ATTR ds3231_send(esp_i2c_t *esp_i2c, uint8 *data, uint8 len) {
	
	int loop;

	// signal i2c start
	i2c_master_start(esp_i2c);

	// write address & direction
	i2c_master_writeByte(esp_i2c, (uint8)(DS3231_ADDR << 1));
	if (!i2c_master_checkAck(esp_i2c)) {
		printf("i2c error no ack for address\r\n");
		i2c_master_stop(esp_i2c);
		return false;
	}

	// send the data
	for (loop = 0; loop < len; loop++) {
		i2c_master_writeByte(esp_i2c, data[loop]);
		if (!i2c_master_checkAck(esp_i2c)) {
			printf("i2c erro no ack for data\r\n");
			i2c_master_stop(esp_i2c);
			return false;
		}
	}

	// signal i2c stop
	i2c_master_stop(esp_i2c);
	return true;

}

// read a number of bytes from the rtc over i2c
// returns true to indicate success
bool ICACHE_FLASH_ATTR ds3231_recv(esp_i2c_t *esp_i2c, uint8 *data, uint8 len) {
	
	int loop;

	// signal i2c start
	i2c_master_start(esp_i2c);

	// write address & direction
	i2c_master_writeByte(esp_i2c, (uint8)((DS3231_ADDR << 1) | 1));
	if (!i2c_master_checkAck(esp_i2c)) {
		printf("i2c error write for recv\r\n");
		i2c_master_stop(esp_i2c);
		return false;
	}

	// read bytes
	for (loop = 0; loop < len; loop++) {
		data[loop] = i2c_master_readByte(esp_i2c);
		// send ack (except after last byte, then we send nack)
		if (loop < (len - 1)) i2c_master_send_ack(esp_i2c); else i2c_master_send_nack(esp_i2c);
	}

	// signal i2c stop
	i2c_master_stop(esp_i2c);

	return true;

}

void rofl(const char *pArg) {
#if 1
    esp_i2c_t i2c_d; 
    esp_i2c_t *esp_i2c = i2c_master_gpio_init(&i2c_d, 2, 14);

// get the temperature as an integer (rounded down)
// returns true to indicate success
	uint8 data[1];
	data[0] = DS3231_ADDR_TEMP;
	// get just the integer part of the temp
	if (ds3231_send(esp_i2c, data, 1) && ds3231_recv(esp_i2c, data, 1)) {
        printf("temp %d", (signed)data[0]);
	}
#endif
#if 0
    extern void i2c_master_gpio_init();
    i2c_master_gpio_init();
    extern bool ds3231_getTempInteger(int8 *out);

    int8 out;
    bool res = ds3231_getTempInteger(&out);
    printf("\nDone %d %d!\n", out, res);
#endif
}
