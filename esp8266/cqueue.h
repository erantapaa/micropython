#ifndef _INCLUDED_CQUEUE_H_
#define _INCLUDED_CQUEUE_H_

typedef struct  {
	int first;
	int last;
	int items;
    int max_items;
    size_t item_size;
	void *data;
} queue_t;

queue_t ICACHE_FLASH_ATTR *qInit(int max_items, size_t item_size);
bool ICACHE_FLASH_ATTR qEmpty(queue_t *queue);
bool qPutItem(queue_t *queue, void *item);
bool ICACHE_FLASH_ATTR getItem(queue_t *queue, void *item);

#endif // _INCLUDED_CQUEUE_H_
