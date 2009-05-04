#include <stdlib.h>

#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"


xQueueHandle fake_queue_create = 0;
xQueueHandle fake_queue_receive = 0;
xQueueHandle fake_semaphore_create = 0;
signed long fake_x_task_create = NULL;

xQueueHandle fakeQueueCreate( void )
{
    return fake_queue_create;
}

xQueueHandle fakeQueueReceive( void * msg )
{
    return fake_queue_receive;
}

xQueueHandle fakeSemaphoreCreate( void )
{
    return fake_semaphore_create;
}

signed long fake_xTaskCreate( void )
{
    return fake_x_task_create;
}