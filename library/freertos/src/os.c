/*
 * Copyright (c) 2011  Weston Schmidt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static inline unsigned long __ms_to_ticks( uint32_t ms );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void os_task_suspend_all( void )
{
    vTaskSuspendAll();
}

bool os_task_resume_all( void )
{
    return (pdFALSE == xTaskResumeAll()) ? false : true;
}

void os_task_delay_ticks( uint32_t ticks )
{
    vTaskDelay( ticks );
}

void os_task_delay_ms( uint32_t ms )
{
    vTaskDelay( __ms_to_ticks(ms) );
}

void os_task_get_run_time_stats( char *buffer )
{
    vTaskGetRunTimeStats( (signed char*) buffer );
}

bool os_task_create( task_fn_t task_fn,
                     const char *name,
                     uint16_t stack_depth,
                     void *params,
                     uint32_t priority,
                     task_handle_t *handle )
{
    long rv;

    rv = xTaskCreate( task_fn, (const signed char * const) name,
                      (unsigned short) stack_depth + configMINIMAL_STACK_SIZE,
                      params, (unsigned long) priority + tskIDLE_PRIORITY,
                      (xTaskHandle) handle );

    if( pdPASS == rv ) {
        return true;
    }

    /* may want to better handle rv when it fails. */
    return false;
}

void os_task_delete( task_handle_t *handle )
{
    vTaskDelete( (xTaskHandle) handle );
}

void os_task_start_scheduler( void )
{
    vTaskStartScheduler();
}

uint32_t os_queue_get_queued_messages_waiting( queue_handle_t queue )
{
    return (uint32_t) uxQueueMessagesWaiting( (xQueueHandle) queue );
}

void os_queue_delete( queue_handle_t queue )
{
    vQueueDelete( (xQueueHandle) queue );
}

queue_handle_t os_queue_create( uint32_t length, uint32_t size )
{
    return (queue_handle_t) xQueueCreate( (unsigned long) length, (unsigned long) size );
}

bool os_queue_is_empty_ISR( queue_handle_t queue )
{
    return (pdFALSE == xQueueIsQueueEmptyFromISR((xQueueHandle) queue)) ? false : true;
}

bool os_queue_is_full_ISR( queue_handle_t queue )
{
    return (pdFALSE == xQueueIsQueueFullFromISR((xQueueHandle) queue)) ? false : true;
}

bool os_queue_peek( queue_handle_t queue, void *buffer, uint32_t ms )
{
    return (pdFALSE == xQueuePeek((xQueueHandle) queue, buffer, __ms_to_ticks(ms))) ? false : true;
}

bool os_queue_receive( queue_handle_t queue, void *buffer, uint32_t ms )
{
    return (pdFALSE == xQueueReceive((xQueueHandle) queue, buffer, __ms_to_ticks(ms))) ? false : true;
}

bool os_queue_receive_ISR( queue_handle_t queue, void *buffer, bool *hp_task_woke )
{
    long was_taken;
    unsigned long rv;

    was_taken = 0;
    rv = xQueueReceiveFromISR( (xQueueHandle) queue, buffer, &was_taken );

    if( NULL != hp_task_woke ) {
        *hp_task_woke = (0 == was_taken) ? false : true;
    }

    return (pdFALSE == rv) ? false : true;
}

bool os_queue_send_to_back( queue_handle_t queue, const void *buffer, uint32_t ms )
{
    return (pdFALSE == xQueueSendToBack((xQueueHandle) queue, buffer, __ms_to_ticks(ms))) ? false : true;
}

bool os_queue_send_to_back_ISR( queue_handle_t queue, const void *buffer, bool *hp_task_woke )
{
    long was_taken;
    unsigned long rv;

    was_taken = 0;
    rv = xQueueSendToBackFromISR( (xQueueHandle) queue, buffer, &was_taken );

    if( NULL != hp_task_woke ) {
        *hp_task_woke = (0 == was_taken) ? false : true;
    }

    return (pdFALSE == rv) ? false : true;
}

bool os_queue_send_to_front( queue_handle_t queue, const void *buffer, uint32_t ms )
{
    return (pdFALSE == xQueueSendToFront((xQueueHandle) queue, buffer, __ms_to_ticks(ms))) ? false : true;
}

bool os_queue_send_to_front_ISR( queue_handle_t queue, const void *buffer, bool *hp_task_woke )
{
    long was_taken;
    unsigned long rv;

    was_taken = 0;
    rv = xQueueSendToFrontFromISR( (xQueueHandle) queue, buffer, &was_taken );

    if( NULL != hp_task_woke ) {
        *hp_task_woke = (0 == was_taken) ? false : true;
    }

    return (pdFALSE == rv) ? false : true;
}

semaphore_handle_t os_semaphore_create_binary( void )
{
    xSemaphoreHandle rv;

    vSemaphoreCreateBinary( rv );

    return (semaphore_handle_t) rv;
}

bool os_semaphore_take( semaphore_handle_t semaphore, uint32_t ms )
{
    return (pdFALSE == xSemaphoreTake((xSemaphoreHandle) semaphore, __ms_to_ticks(ms))) ? false : true;
}

bool os_semaphore_give( semaphore_handle_t semaphore )
{
    return (pdFALSE == xSemaphoreGive((xSemaphoreHandle) semaphore)) ? false : true;
}

bool os_semaphore_give_ISR( semaphore_handle_t semaphore, bool *hp_task_woke )
{
    long was_taken;
    unsigned long rv;

    was_taken = 0;
    rv = xSemaphoreGiveFromISR( (xSemaphoreHandle) semaphore, &was_taken );

    if( NULL != hp_task_woke ) {
        *hp_task_woke = (0 == was_taken) ? false : true;
    }

    return (pdFALSE == rv) ? false : true;
}

mutex_handle_t os_mutex_create( void )
{
    xSemaphoreHandle rv;

    rv = xSemaphoreCreateMutex();

    return (mutex_handle_t) rv;
}

bool os_mutex_take( mutex_handle_t mutex, uint32_t ms )
{
    return (pdFALSE == xSemaphoreTake((xSemaphoreHandle) mutex, ms)) ? false : true;
}

bool os_mutex_give( mutex_handle_t mutex )
{
    return (pdFALSE == xSemaphoreGive((xSemaphoreHandle) mutex)) ? false : true;
}

bool os_mutex_give_ISR( mutex_handle_t mutex, bool *hp_task_woke )
{
    long was_taken;
    unsigned long rv;

    was_taken = 0;
    rv = xSemaphoreGiveFromISR( (xSemaphoreHandle) mutex, &was_taken );

    if( NULL != hp_task_woke ) {
        *hp_task_woke = (0 == was_taken) ? false : true;
    }

    return (pdFALSE == rv) ? false : true;
}
/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static inline unsigned long __ms_to_ticks( uint32_t ms )
{
#if (1000 == configTICK_RATE_HZ)
    return ms;
#else
    if( (NO_WAIT == ms) || (WAIT_FOREVER == ms) ) {
        return ms;
    } else if( ms < portTICK_RATE_MS ) {
        return 1;
    }

    return (ms / portTICK_RATE_MS);
#endif
}
