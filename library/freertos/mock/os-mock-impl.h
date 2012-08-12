/*
 * Copyright (c) 2012  Weston Schmidt
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
#ifndef __OS_MOCK_IMPL_H__
#define __OS_MOCK_IMPL_H__

#include <stdbool.h>
#include <stdint.h>
#include "../src/os.h"

/*----------------------------------------------------------------------------*/
/*                    Mock Implementation Related Functions                   */
/*----------------------------------------------------------------------------*/
void mock_os_init_std( void );
void mock_os_destroy_std( void );

/*----------------------------------------------------------------------------*/
/*                           Task Related Functions                           */
/*----------------------------------------------------------------------------*/
void os_task_suspend_all_std( void );
bool os_task_resume_all_std( void );
void os_task_delay_ticks_std( uint32_t ticks );
void os_task_delay_ms_std( uint32_t ms );
void os_task_get_run_time_stats_std( char *buffer );
bool os_task_create_std( task_fn_t task_fn,
                         const char *name,
                         uint16_t stack_depth,
                         void *params,
                         uint32_t priority,
                         task_handle_t *handle );
void os_task_delete_std( task_handle_t *handle );
void os_task_start_scheduler_std( void );

/*----------------------------------------------------------------------------*/
/*                          Queue Related Functions                           */
/*----------------------------------------------------------------------------*/
queue_handle_t os_queue_create_std( uint32_t length, uint32_t size );
void os_queue_delete_std( queue_handle_t queue );
uint32_t os_queue_get_queued_messages_waiting_std( queue_handle_t queue );
bool os_queue_is_empty_ISR_std( queue_handle_t queue );
bool os_queue_is_full_ISR_std( queue_handle_t queue );
bool os_queue_peek_std( queue_handle_t queue, void *buffer, uint32_t ms );
bool os_queue_receive_std( queue_handle_t queue, void *buffer, uint32_t ms );
bool os_queue_receive_ISR_std( queue_handle_t queue,
                               void *buffer,
                               bool *hp_task_woke );
bool os_queue_send_to_back_std( queue_handle_t queue,
                                const void *buffer,
                                uint32_t ms );
bool os_queue_send_to_back_ISR_std( queue_handle_t queue,
                                    const void *buffer,
                                    bool *hp_task_woke );
bool os_queue_send_to_front_std( queue_handle_t queue,
                                 const void *buffer,
                                 uint32_t ms );
bool os_queue_send_to_front_ISR_std( queue_handle_t queue,
                                     const void *buffer,
                                     bool *hp_task_woke );

/*----------------------------------------------------------------------------*/
/*                        Semaphore Related Functions                         */
/*----------------------------------------------------------------------------*/
semaphore_handle_t os_semaphore_create_binary_std( void );
void os_semaphore_delete_std( semaphore_handle_t semaphore );
bool os_semaphore_take_std( semaphore_handle_t semaphore, uint32_t ms );
bool os_semaphore_give_std( semaphore_handle_t semaphore );
bool os_semaphore_give_ISR_std( semaphore_handle_t semaphore,
                                bool *hp_task_woke );

/*----------------------------------------------------------------------------*/
/*                           Mutex Related Functions                          */
/*----------------------------------------------------------------------------*/
mutex_handle_t os_mutex_create_std( void );
void os_mutex_delete_std( mutex_handle_t mutex );
bool os_mutex_take_std( mutex_handle_t mutex, uint32_t ms );
bool os_mutex_give_std( mutex_handle_t mutex );

#endif
