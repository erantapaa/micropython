#include <stdio.h>
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "esp_mphal.h"
#include "etshal.h"


#include "py/nlr.h"
#include "py/runtime.h"
#include "mod_esp_gpio.h"

/* 
* Adaptation of Paul Stoffregen's One wire library to the ESP8266 and 
* Necromant's Frankenstein firmware by Erland Lewin <erland@lewin.nu> 
* Paul's original library site: 
*   http://www.pjrc.com/teensy/td_libs_OneWire.html 
*/


pmap_t ICACHE_FLASH_ATTR *ds_init(int gpio) { 
    pmap_t *pmp = pmap(gpio);
    PIN_FUNC_SELECT(pmp->periph, pmp->func);
    PIN_PULLUP_EN(pmp->periph);
    GPIO_DIS_OUTPUT(pmp->pin);
    return pmp;
}

// Perform the onewire reset function.  We will wait up to 250uS for 
// the bus to come high, if it doesn't then it is broken or shorted 
// and we return;

uint8_t ICACHE_FLASH_ATTR ds_reset(pmap_t *pmp) { 
    //    IO_REG_TYPE mask = bitmask; 
    //    volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg; 
    uint8_t retries = 125; 
    // noInterrupts(); 
    // DIRECT_MODE_INPUT(reg, mask); 
    GPIO_DIS_OUTPUT(pmp->pin); 
    // interrupts(); 
    // wait until the wire is high... just in case 
    do { 
        if (--retries == 0) {
            return -1;
        }
        os_delay_us(2); 
    } while (!GPIO_INPUT_GET(pmp->pin)); 
    // noInterrupts(); 
    GPIO_OUTPUT_SET( pmp->pin, 0 ); 
    // interrupts(); 
    os_delay_us(480); 
    // noInterrupts(); 
    GPIO_DIS_OUTPUT(pmp->pin); 
    os_delay_us(70); 
    retries = 125;
    uint8_t r;

    do { 
        if (--retries == 0) {
            return -2;
        }
        os_delay_us(10); 
    } while (!(r = GPIO_INPUT_GET(pmp->pin)));
    // interrupts(); 
    // os_delay_us(410); 
    return r;
}

// 
// Write a bit. Port and bit is used to cut lookup time and provide 
// more certain timing. 
// 
static inline void write_bit(pmap_t *pmp, int v) { 
    GPIO_OUTPUT_SET(pmp->pin, 0); 
    if (v) { 
        // noInterrupts(); 
        os_delay_us(10); 
        GPIO_OUTPUT_SET(pmp->pin, 1); 
        // interrupts(); 
        os_delay_us(55); 
    } else { 
        // noInterrupts(); 
        os_delay_us(65); 
        GPIO_OUTPUT_SET(pmp->pin, 1); 
        //        interrupts(); 
        os_delay_us(5); 
    } 
}

// 
// Read a bit. Port and bit is used to cut lookup time and provide 
// more certain timing. 
// 
static inline int read_bit(pmap_t *pmp) { 
    int r; 
    // noInterrupts(); 
    GPIO_OUTPUT_SET(pmp->pin, 0); 
    os_delay_us(3); 
    GPIO_DIS_OUTPUT(pmp->pin); 
    os_delay_us(10); 
    r = GPIO_INPUT_GET(pmp->pin); 
    // interrupts(); 
    os_delay_us(53); 
    return r; 
}

// 
// Write a byte. The writing code uses the active drivers to raise the 
// pin high, if you need power after the write (e.g. DS18S20 in 
// parasite power mode) then set 'power' to 1, otherwise the pin will 
// go tri-state at the end of the write to avoid heating in a short or 
// other mishap. 
// 
void ICACHE_FLASH_ATTR  ds_write(pmap_t *pmp, uint8_t v, int power) { 
    uint8_t bitMask; 

    for (bitMask = 0x01; bitMask; bitMask <<= 1) { 
        write_bit(pmp, (bitMask & v) ? 1 : 0); 
    } 
    if (!power) { 
        // noInterrupts(); 
        GPIO_DIS_OUTPUT(pmp->pin); 
        GPIO_OUTPUT_SET(pmp->pin, 0); 
        // interrupts(); 
    } 
}

// 
// Read a byte 
// 
uint8_t ICACHE_FLASH_ATTR ds_read(pmap_t *pmp) { 
    uint8_t bitMask; 
    uint8_t r = 0; 
    for (bitMask = 0x01; bitMask; bitMask <<= 1) { 
        if (read_bit(pmp))
            r |= bitMask; 
    } 
    return r; 
}
