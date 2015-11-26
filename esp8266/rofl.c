#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <gpio.h>
#include <user_interface.h>
#include <espconn.h>
#include <mem.h>

#include <spi_flash.h>

#include "py/obj.h"

#define K *(1024)
#define M *(1024 * 1024)

uint32_t flash_size(void) {
    uint32_t id = spi_flash_get_id();
    uint8_t mfgr_id = id & 0xff;

    if (mfgr_id == 0xE0 || mfgr_id == 0xEF) { // winbond ids
        uint8_t size_id = (id >> 16) & 0xff; 
        return 1 << size_id;
    }
    // from arduino code, no idea of these work, I have boards with Winbonds
    switch(id) {
        // GigaDevice
        case 0x1740C8: // GD25Q64B
            return 8 M;
        case 0x1640C8: // GD25Q32B
            return 4 M;
        case 0x1540C8: // GD25Q16B
            return 2 M;
        case 0x1440C8: // GD25Q80
            return 1 M;
        case 0x1340C8: // GD25Q40
            return 512 K;
        case 0x1240C8: // GD25Q20
            return 256 K;
        case 0x1140C8: // GD25Q10
            return 128 K;
        case 0x1040C8: // GD25Q12
            return 64 K;

        default:
            printf("Uknown %x\n", id);
            return 0;
    }
}


#include "mod_esp_gpio.h"
#include "esp_1wire.h"

static pmap_t *gpio;
#define MAXI    10

void rofl(const char *arg, int arg2) {
    uint8_t arr[MAXI];

    switch (arg[0]) {
    case 'i':
        gpio = ds_init(5);
        break;
    case 'e':
        printf("reset %d\n", ds_reset(gpio));
        break;
    case 'w':
        printf("writing %x (%u)\n", arg2, arg2);
        ds_write(gpio, arg2, 1);
        break;
    case 'r':
        if (arg2 > MAXI) {
            printf("%d too big\n", arg2);
            return;
        }
        for (int ii = 0; ii < arg2; ii++) {
            arr[ii] = ds_read(gpio); 
        }
        for (int ii = 0; ii < arg2; ii++) {
            printf("%u (%x) ", arr[ii], arr[ii]);
        }
        break;
    }
}
#if 0
static uint32_t blah[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int32_t fsize = flash_size() / 8;
    extern int32_t _irom0_text_end;
    extern  int32_t _irom0_text_start;
    uint32 buffer[sizeof(blah)];

    printf("flash size %u\n", fsize);
    uint32_t used = (uint32_t)(&_irom0_text_end - &_irom0_text_start);
    printf("used sectors %u\n", used / SPI_FLASH_SEC_SIZE);
    printf("max sectors %u\n", fsize / SPI_FLASH_SEC_SIZE);
    uint32_t start_sector = (used / SPI_FLASH_SEC_SIZE) + 1;
    printf("start sector %u\n", start_sector);
    printf("start %u %x\n", (uint32_t)&_irom0_text_start, (uint32_t)&_irom0_text_start);
    printf("end %u %x\n", (uint32_t)&_irom0_text_end, (uint32_t)&_irom0_text_end);
    switch (arg[0]) {
    case 'e':
        printf("erase %d\n", arg2);
        printf("result %d\n", spi_flash_erase_sector(arg2));
        break;
    case 'w':
        printf("write %d address %x\n", arg2, arg2 * SPI_FLASH_SEC_SIZE);
        printf("result %d\n", spi_flash_write(arg2 * SPI_FLASH_SEC_SIZE, blah, sizeof (blah)));
        break;
    case 'r':
        printf("read %d\n", arg2);
        printf("result %u\n", spi_flash_read( arg2 * SPI_FLASH_SEC_SIZE, buffer, sizeof (blah)));
        for (int ii = 0; ii < sizeof (blah) / sizeof (uint32_t); ii++) {
            printf("%d is %d\n", ii, buffer[ii]);
        }
        break;
    }
#endif

