#ifndef __SEMPHR_H__
#define __SEMPHR_H__

#include "freertos/portmacro.h"
#include "freertos/queue.h"

typedef xQueueHandle xSemaphoreHandle;

#define xSemaphoreTake(X, Y) 0
#define xSemaphoreGive(X)    0
#define xSemaphoreCreateMutex() fakeSemaphoreCreate()

extern xQueueHandle fake_semaphore_create;

xQueueHandle fakeSemaphoreCreate( void );


#endif /* __SEMPHR_H__ */
