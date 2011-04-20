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
#ifndef __OS_H__
#define __OS_H__
#include <stdbool.h>
#include <stdint.h>

#define WAIT_FOREVER    0xffffffff
#define NO_WAIT         0

typedef void* task_handle_t;
typedef void* queue_handle_t;
typedef void* semaphore_handle_t;

typedef void (*task_fn_t)( void *);

/*----------------------------------------------------------------------------*/
/*                           Task Related Functions                           */
/*----------------------------------------------------------------------------*/
void os_task_suspend_all( void );
bool os_task_resume_all( void );
void os_task_delay_ticks( uint32_t ticks );
void os_task_delay_ms( uint32_t ms );
void os_task_get_run_time_stats( char *buffer );
bool os_task_create( task_fn_t task_fn,
                     const char *name,
                     uint16_t stack_depth,
                     void *params,
                     uint32_t priority,
                     task_handle_t *handle );
void os_task_delete( task_handle_t *handle );
void os_task_start_scheduler( void );

/*----------------------------------------------------------------------------*/
/*                          Queue Related Functions                           */
/*----------------------------------------------------------------------------*/
queue_handle_t os_queue_create( uint32_t length, uint32_t size );
void os_queue_delete( queue_handle_t queue );
uint32_t os_queue_get_queued_messages_waiting( queue_handle_t queue );
bool os_queue_is_empty_ISR( queue_handle_t queue );
bool os_queue_is_full_ISR( queue_handle_t queue );
bool os_queue_peek( queue_handle_t queue, void *buffer, uint32_t ms );
bool os_queue_receive( queue_handle_t queue, void *buffer, uint32_t ms );
bool os_queue_receive_ISR( queue_handle_t queue, void *buffer, bool *hp_task_woke );
bool os_queue_send_to_back( queue_handle_t queue, const void *buffer, uint32_t ms );
bool os_queue_send_to_back_ISR( queue_handle_t queue, const void *buffer, bool *hp_task_woke );
bool os_queue_send_to_front( queue_handle_t queue, const void *buffer, uint32_t ms );
bool os_queue_send_to_front_ISR( queue_handle_t queue, const void *buffer, bool *hp_task_woke );

/*----------------------------------------------------------------------------*/
/*                        Semaphore Related Functions                         */
/*----------------------------------------------------------------------------*/
semaphore_handle_t os_semaphore_create_binary( void );
bool os_semaphore_take( semaphore_handle_t semaphore, uint32_t ms );
bool os_semaphore_give( semaphore_handle_t semaphore );
bool os_semaphore_give_ISR( semaphore_handle_t semaphore, bool *hp_task_woke );
#endif
