#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>

#include "mem.h"
#include "osapi.h"
#include "os_type.h"

#include "py/obj.h"
#include "py/nlr.h"
#include "py/runtime.h"

#include "cqueue.h"


queue_t ICACHE_FLASH_ATTR *qInit(int max_items)
{
    queue_t *queue;
    if ((queue = (queue_t *)m_new_obj(queue_t)) == NULL) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "nomem for q"));
    }
	queue->items = 0;
	queue->first = 0;
	queue->last = 0;
    queue->max_items = max_items;
    if ((queue->objs = m_new(mp_obj_t, max_items)) == NULL) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "no mem for q data"));
    }
	return queue;
}


bool ICACHE_FLASH_ATTR qEmpty(queue_t *queue) {
	if (queue->items == 0)
		return true;
	else
		return false;
}


bool qPutItem(queue_t *queue, mp_obj_t item) {
	if (queue->items >= queue->max_items) {
		return false;
	} else {
		queue->items++;
		queue->objs[queue->last] = item;
		queue->last = (queue->last + 1) % queue->max_items;
	}
    return true;
}

bool ICACHE_FLASH_ATTR getItem(queue_t *queue, mp_obj_t *item) {
	if (qEmpty(queue)) {
		return false;
	} else {
        *item = queue->objs[queue->first];
		queue->first = (queue->first + 1) % queue->max_items;
		queue->items--;
		return true;
	}
}

#if 0
void printQueue(queue_t *queue, void (printer)(void *)) {
	int aux, aux1;

	aux = queue->first;
	aux1 = queue->items;
	while (aux1 > 0) {
		printf("Element #%d = ", aux);
        printer((char *)queue->objs + aux * queue->item_size);
        printf("\n");
		aux = (aux + 1) % queue->max_items;
		aux1--;
	}
	return;
}

void    printer(void *value) {
    printf("%d", *(int*)value);
}

int main(void) {
	int i;
	int readValue;

	queue_t *qq = initializeQueue(10, sizeof (int));
	for (i = 0; i < qq->max_items + 1; i++) {
		putItem(qq, &i);
	}
	printQueue(qq, printer);

	for (i = 0; i < qq->max_items / 2; i++) {
		getItem(qq, &readValue);
		printf("readValue = %d\n", readValue);
	}
	printQueue(qq, printer);
	exit(0);
}
#endif
