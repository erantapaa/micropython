#include <stdio.h>
#include <c_types.h>

#include "esp_frozen.h"

static void ccr(const esp_frozen_t *pp) {
    const uint32_t *kk = mp_frozen_qwords + pp->offset;

    for (int ii = 0; ii < pp->size; ii++) {
        int nibble = ii % 4;
        printf("%c", ((kk[ii / 4] >> (nibble * 8)) & 0xff));
    }
}

void rofl() {
    const esp_frozen_t *pp = mp_frozen_table;

    for (int ii = 0; ii < mp_frozen_table_size; ii++, pp++) {
        printf("ROFL is %s\n", pp->name);
        ccr(pp);
    }
}
