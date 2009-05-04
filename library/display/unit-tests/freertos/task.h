#ifndef __TASK_H__
#define __TASK_H__

#include "freertos/portmacro.h"

typedef void * xTaskHandle;

#define xTaskCreate( X, Y, Z, A, B, C ) fake_xTaskCreate( )
#define vTaskDelay( X ) 0
#define vTaskDelete( X ) 0

extern signed long fake_x_task_create;

signed long fake_xTaskCreate( void );

#endif /* __TASK_H__ */
