#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "freertos/portmacro.h"

struct QueueDefinition;
typedef struct QueueDefinition *xQueueHandle;

#define xQueueSendToBack(X, Y, Z) 1
#define xQueueCreateMutex() fakeQueueCreate()
#define xQueueCreate(X, Y) fakeQueueCreate()
#define vQueueDelete(X) 0

#define xQueueReceive(X,Y,Z) fakeQueueReceive( Y )

extern xQueueHandle fake_queue_create;
extern xQueueHandle fake_queue_receive;

xQueueHandle fakeQueueCreate( void );
xQueueHandle fakeQueueReceive( void * msg );


#endif /* __QUEUE_H__ */
