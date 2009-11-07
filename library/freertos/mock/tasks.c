#include <mock/mock.h>

#define FOSC0 12000000

#include "task.h"

signed portBASE_TYPE xTaskCreate(
        pdTASK_CODE pvTaskCode,
        const signed portCHAR * const pcName,
        unsigned portSHORT usStackDepth,
        void *pvParameters,
        unsigned portBASE_TYPE uxPriority,
        xTaskHandle *pvCreatedTask )
{
    return 0;
}


void vTaskDelete( xTaskHandle pxTask )
{
    return;
}


void vTaskDelay( portTickType xTicksToDelay )
{
    return;
}


void vTaskDelayUntil( portTickType * const pxPreviousWakeTime,
        portTickType xTimeIncrement )
{
    return;
}


unsigned portBASE_TYPE uxTaskPriorityGet( xTaskHandle pxTask )
{
    return 0;
}


void vTaskPrioritySet( xTaskHandle pxTask, unsigned portBASE_TYPE uxNewPriority )
{
    return;
}


void vTaskSuspend( xTaskHandle pxTaskToSuspend )
{
    return;
}


void vTaskResume( xTaskHandle pxTaskToResume )
{
    return;
}


portBASE_TYPE xTaskResumeFromISR( xTaskHandle pxTaskToResume )
{
    return 0;
}


void vTaskStartScheduler( void )
{
    return;
}


void vTaskEndScheduler( void )
{
    return;
}


void vTaskSuspendAll( void )
{
    return;
}


signed portBASE_TYPE xTaskResumeAll( void )
{
    return 0;
}

signed portBASE_TYPE xTaskIsTaskSuspended( xTaskHandle xTask )
{
    return 0;
}


xTaskHandle xTaskGetCurrentTaskHandle( void )
{
    return 0;
}

portTickType xTaskGetTickCount( void )
{
    return 0;
}


unsigned portBASE_TYPE uxTaskGetNumberOfTasks( void )
{
    return 0;
}


void vTaskList( signed portCHAR *pcWriteBuffer )
{
    return;
}


void vTaskStartTrace( signed portCHAR * pcBuffer, unsigned portLONG ulBufferSize )
{
    return;
}


unsigned portLONG ulTaskEndTrace( void )
{
    return 0;
}


unsigned portBASE_TYPE uxTaskGetStackHighWaterMark( xTaskHandle xTask )
{
    return 0;
}


void vTaskSetApplicationTaskTag( xTaskHandle xTask, pdTASK_HOOK_CODE pxHookFunction )
{
    return;
}


portBASE_TYPE xTaskCallApplicationTaskHook( xTaskHandle xTask, void *pvParameter )
{
    return 0;
}
