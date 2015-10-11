
#ifndef _INCLUDED_OS_TASK_H_
#define _INCLUDED_OS_TASK_H_

#define SENSOR_TASK_ID 1

typedef struct _esp_os_task_obj_t {
    mp_obj_base_t base;
    mp_obj_t callback;
} esp_os_task_obj_t;

extern const mp_obj_type_t esp_os_task_type;
extern void esp_os_task_init();

#endif // _INCLUDED_OS_TASK_H_
