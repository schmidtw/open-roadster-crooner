#ifndef __PORTMACRO_H__
#define __PORTMACRO_H__

#define portTickType    int
#define portMAX_DELAY 0xFFFFFFFF
#define pdPASS      ( 1 )
#define pdFAIL      ( 0 )
#define pdTRUE      ( 1 )
#define pdFALSE     ( 0 )
#define tskIDLE_PRIORITY 10

#define portCHAR        char
#define portFLOAT       float
#define portDOUBLE      double
#define portLONG        long
#define portSHORT       short
#define portSTACK_TYPE  unsigned long
#define portBASE_TYPE   long

typedef void (*pdTASK_CODE)( void * );

#endif /* __PORTMACRO_H__ */
