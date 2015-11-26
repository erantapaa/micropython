#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>

#include "mem.h"
#include "osapi.h"
#include "os_type.h"

#include "py/nlr.h"
#include "py/runtime.h"

#include "cqueue.h"

extern void *pvPortMalloc(size_t xWantedSize, const char *file, const char *line);

queue_t ICACHE_FLASH_ATTR *qInit(int max_items, size_t item_size)
{
    queue_t *queue;
    if ((queue = (queue_t *)os_malloc(sizeof (queue_t))) == NULL) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "nomem for q"));
    }
	queue->items = 0;
	queue->first = 0;
	queue->last = 0;
    queue->max_items = max_items;
    queue->item_size = item_size;
    if ((queue->data = os_malloc(max_items * item_size)) == NULL) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "nomem for q data"));
    }
	return queue;
}


bool ICACHE_FLASH_ATTR qEmpty(queue_t *queue) {
	if (queue->items == 0)
		return true;
	else
		return false;
}


bool qPutItem(queue_t *queue, void *item) {
	if (queue->items >= queue->max_items) {
		return false;
	} else {
		queue->items++;
		memcpy((char *)queue->data + queue->last * queue->item_size, item, queue->item_size);
		queue->last = (queue->last + 1) % queue->max_items;
	}
    return true;
}

bool ICACHE_FLASH_ATTR getItem(queue_t *queue, void *item) {
	if (qEmpty(queue)) {
		return false;
	} else {
        memcpy(item, (char *)queue->data + queue->first * queue->item_size, queue->item_size);
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
        printer((char *)queue->data + aux * queue->item_size);
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
