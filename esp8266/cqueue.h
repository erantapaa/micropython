#ifndef _INCLUDED_CQUEUE_H_
#define _INCLUDED_CQUEUE_H_

typedef struct  {
	int first;
	int last;
	int items;
    int max_items;
	mp_obj_t *objs;
} queue_t;


queue_t ICACHE_FLASH_ATTR *qInit(int max_items);
bool qPutItem(queue_t *queue, mp_obj_t item);
bool ICACHE_FLASH_ATTR getItem(queue_t *queue, mp_obj_t *item);

bool ICACHE_FLASH_ATTR qEmpty(queue_t *queue);
#endif // _INCLUDED_CQUEUE_H_
