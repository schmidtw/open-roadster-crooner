#include <mock/mock.h>
#include "queue.h"

xQueueHandle xQueueCreate( unsigned portBASE_TYPE uxQueueLength, unsigned portBASE_TYPE uxItemSize )
{
    return 0;
}


void vQueueDelete( xQueueHandle xQueue )
{
}

signed portBASE_TYPE xQueueGenericSend(
        xQueueHandle xQueue, const void * const pvItemToQueue,
        portTickType xTicksToWait, portBASE_TYPE xCopyPosition )
{
    return 0;
}


signed portBASE_TYPE xQueueGenericReceive(
        xQueueHandle xQueue, void * const pvBuffer,
        portTickType xTicksToWait, portBASE_TYPE xJustPeek )
{
    return 0;
}


unsigned portBASE_TYPE uxQueueMessagesWaiting( const xQueueHandle xQueue )
{
    return 0;
}



signed portBASE_TYPE xQueueGenericSendFromISR(
        xQueueHandle pxQueue, const void * const pvItemToQueue,
        signed portBASE_TYPE *pxHigherPriorityTaskWoken, portBASE_TYPE xCopyPosition )
{
    return 0;
}

signed portBASE_TYPE xQueueReceiveFromISR(
        xQueueHandle pxQueue, void * const pvBuffer,
        signed portBASE_TYPE *pxTaskWoken )
{
    return 0;
}


signed portBASE_TYPE xQueueIsQueueEmptyFromISR( const xQueueHandle pxQueue )
{
    return 0;
}


signed portBASE_TYPE xQueueIsQueueFullFromISR( const xQueueHandle pxQueue )
{
    return 0;
}


unsigned portBASE_TYPE uxQueueMessagesWaitingFromISR( const xQueueHandle pxQueue )
{
    return 0;
}

signed portBASE_TYPE xQueueAltGenericReceive(
        xQueueHandle pxQueue, void * const pvBuffer,
        portTickType xTicksToWait, portBASE_TYPE xJustPeeking )
{
    return 0;
}


xQueueHandle xQueueCreateMutex( void )
{
    return 0;
}

xQueueHandle xQueueCreateCountingSemaphore(
        unsigned portBASE_TYPE uxCountValue,
        unsigned portBASE_TYPE uxInitialCount )
{
    return 0;
}


portBASE_TYPE xQueueTakeMutexRecursive( xQueueHandle xMutex, portTickType xBlockTime )
{
    return 0;
}


portBASE_TYPE xQueueGiveMutexRecursive( xQueueHandle xMutex )
{
    return 0;
}

#if (configQUEUE_REGISTRY_SIZE > 0)
void vQueueAddToRegistry( xQueueHandle xQueue, signed portCHAR *pcName )
{
    return;
}
#endif
