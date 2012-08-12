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

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "os-mock-impl.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define OS_MAX_TASK_COUNT   10
#define OS_MAX_MUTEX_COUNT  20
#define OS_MAX_QUEUE_COUNT  20
#define OS_MAX_SEM_COUNT    20

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    int index;
    bool active;
    task_fn_t task_fn;
    char *name;
    uint16_t stack_depth;
    void *params;
    uint32_t priority;
    pthread_t id;
} mock_task_t;

typedef struct mock_queue_node {
    void *data;

    struct mock_queue_node *next;
} mock_queue_node_t;

typedef struct mock_queue {
    bool active;
    uint32_t max_depth;
    uint32_t data_size;
    pthread_mutex_t mutex;
    pthread_cond_t cond_queued;
    pthread_cond_t cond_idle;
    mock_queue_node_t *queued;
    mock_queue_node_t *idle;
} mock_queue_t;

typedef struct {
    bool active;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
} mock_sem_t;

typedef struct {
    bool active;
    pthread_mutex_t mutex;
} mock_mutex_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static pthread_mutex_t __internal_mutex;

static mock_task_t __tasks[OS_MAX_TASK_COUNT];
static mock_mutex_t __mutexes[OS_MAX_MUTEX_COUNT];
static mock_queue_t __queues[OS_MAX_QUEUE_COUNT];
static mock_sem_t __sems[OS_MAX_SEM_COUNT];

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void* __task_wrapper( void *params );
static void __task_delay( struct timespec *until );
static void __get_expire_time( struct timespec *t, uint64_t ns );
static bool __queue_peek( queue_handle_t queue, void *buffer, uint32_t ms,
                          bool peek );
static bool __queue_push( queue_handle_t queue, const void *buffer, uint32_t ms,
                          bool head );
static void __lock( void );
static void __unlock( void );

void MOCK_os_init( void )
{
    int i, rv;

    rv = pthread_mutex_init( &__internal_mutex, NULL );
    assert( 0 == rv );

    for( i = 0; i < OS_MAX_TASK_COUNT; i++ ) {
        __tasks[i].index = i;
        __tasks[i].active = false;
    }

    for( i = 0; i < OS_MAX_MUTEX_COUNT; i++ ) {
        __mutexes[i].active = false;
    }

    for( i = 0; i < OS_MAX_QUEUE_COUNT; i++ ) {
        __queues[i].active = false;
    }

    for( i = 0; i < OS_MAX_SEM_COUNT; i++ ) {
        __sems[i].active = false;
    }
}

void MOCK_os_destroy( void )
{
    int rv;

    rv = pthread_mutex_destroy( &__internal_mutex );
    assert( 0 == rv );
}

/*----------------------------------------------------------------------------*/
/*                      External Task Related Functions                       */
/*----------------------------------------------------------------------------*/
/* See os.h for details. */
void os_task_suspend_all_std( void )
{
}


/* See os.h for details. */
bool os_task_resume_all_std( void )
{
}


/* See os.h for details. */
void os_task_delay_ticks_std( uint32_t ticks )
{
    struct timespec e;
    __get_expire_time( &e, ticks );
    __task_delay( &e );
}


/* See os.h for details. */
void os_task_delay_ms_std( uint32_t ms )
{
    struct timespec e;
    __get_expire_time( &e, ms * 1000000 );
    __task_delay( &e );
}


/* See os.h for details. */
void os_task_get_run_time_stats_std( char *buffer )
{
    if( NULL == buffer ) {
        *buffer = '\0';
    }
}


/* See os.h for details. */
bool os_task_create_std( task_fn_t task_fn,
                         const char *name,
                         uint16_t stack_depth,
                         void *params,
                         uint32_t priority,
                         task_handle_t *handle )
{
    mock_task_t *task;
    int rv;
    int i;

    assert( NULL != task_fn );
    assert( NULL != name );
    assert( 0 < stack_depth );

    task = NULL;
    __lock();
    for( i = 0; i < OS_MAX_TASK_COUNT; i++ ) {
        if( false == __tasks[i].active ) {
            __tasks[i].active = true;
            task = &__tasks[i];
            break;
        }
    }
    __unlock();
    assert( NULL != task );

    task->task_fn = task_fn;
    task->name = strdup( name );
    task->stack_depth = stack_depth;
    task->params = params;
    task->priority = priority;

    rv = pthread_create( &task->id, NULL, __task_wrapper, task );
    assert( 0 == rv );

    if( NULL != handle ) {
        *handle = &task->index;
    }

    return true;
}


/* See os.h for details. */
void os_task_delete_std( task_handle_t *handle )
{
    int index;

    assert( NULL != handle );

    index = *((int*) handle);

    assert( 0 <= index );
    assert( index < OS_MAX_TASK_COUNT );

    if( pthread_self() != __tasks[index].id ) {
        pthread_join( __tasks[index].id, NULL );
    }

    __lock();
    free( __tasks[index].name );
    __tasks[index].active = false;
    __unlock();
}


/* See os.h for details. */
void os_task_start_scheduler_std( void )
{
}


/*----------------------------------------------------------------------------*/
/*                      External Queue Related Functions                      */
/*----------------------------------------------------------------------------*/
/* See os.h for details. */
queue_handle_t os_queue_create_std( uint32_t length, uint32_t size )
{
    mock_queue_t *queue;
    int rv;
    int i;

    queue = NULL;
    __lock();
    for( i = 0; i < OS_MAX_TASK_COUNT; i++ ) {
        if( false == __queues[i].active ) {
            __queues[i].active = true;
            queue = &__queues[i];
            break;
        }
    }
    __unlock();
    assert( NULL != queue );

    rv = pthread_mutex_init( &queue->mutex, NULL );
    assert( 0 == rv );

    rv = pthread_cond_init( &queue->cond_queued, NULL );
    assert( 0 == rv );

    rv = pthread_cond_init( &queue->cond_idle, NULL );
    assert( 0 == rv );

    queue->max_depth = length;
    queue->data_size = size;
    queue->queued = NULL;
    queue->idle = NULL;

    for( i = 0; i < length; i++ ) {
        mock_queue_node_t *n;

        n = (mock_queue_node_t*) malloc( sizeof(mock_queue_node_t) );
        assert( NULL != n );
        n->data = malloc( size );
        assert( NULL != n->data );

        n->next = queue->idle;
        queue->idle = n;
    }

    return (queue_handle_t) queue;
}


/* See os.h for details. */
void os_queue_delete_std( queue_handle_t queue )
{
    mock_queue_t *q;
    mock_queue_node_t *n;
    int rv, i;

    q = (mock_queue_t*) queue;
    assert( NULL != q );

    rv = pthread_mutex_destroy( &q->mutex );
    assert( rv == 0 );
    rv = pthread_cond_destroy( &q->cond_queued );
    assert( rv == 0 );
    rv = pthread_cond_destroy( &q->cond_idle );
    assert( rv == 0 );

    n = q->queued;
    while( NULL != n ) {
        free( n->data );
        q->queued = n->next;
        free( n );
        n = q->queued;
    }

    n = q->idle;
    while( NULL != n ) {
        free( n->data );
        q->idle = n->next;
        free( n );
        n = q->idle;
    }
}


/* See os.h for details. */
uint32_t os_queue_get_queued_messages_waiting_std( queue_handle_t queue )
{
    uint32_t count;
    mock_queue_t *q;
    mock_queue_node_t *n;
    int rv, i;

    q = (mock_queue_t*) queue;
    assert( NULL != q );

    rv = pthread_mutex_lock( &q->mutex );
    assert( rv == 0 );

    count = 0;
    n = q->queued;
    while( NULL != n ) {
        count++;
        n = n->next;
    }

    assert( rv == 0 );
    rv = pthread_mutex_unlock( &q->mutex );
    assert( rv == 0 );

    return count;
}


/* See os.h for details. */
bool os_queue_is_empty_ISR_std( queue_handle_t queue )
{
    uint32_t count;

    count = os_queue_get_queued_messages_waiting( queue );

    return (0 == count) ? true : false;
}


/* See os.h for details. */
bool os_queue_is_full_ISR_std( queue_handle_t queue )
{
    uint32_t count;

    count = ((mock_queue_t*) queue)->max_depth;
    count -= os_queue_get_queued_messages_waiting( queue );

    return (0 == count) ? true : false;
}


/* See os.h for details. */
bool os_queue_peek_std( queue_handle_t queue, void *buffer, uint32_t ms )
{
    return __queue_peek( queue, buffer, ms, true );
}


/* See os.h for details. */
bool os_queue_receive_std( queue_handle_t queue, void *buffer, uint32_t ms )
{
    return __queue_peek( queue, buffer, ms, false );
}


/* See os.h for details. */
bool os_queue_receive_ISR_std( queue_handle_t queue,
                               void *buffer,
                               bool *hp_task_woke )
{
    *hp_task_woke = false;
    return os_queue_receive( queue, buffer, WAIT_FOREVER );
}


/* See os.h for details. */
bool os_queue_send_to_back_std( queue_handle_t queue,
                                const void *buffer,
                                uint32_t ms )
{
    return __queue_push( queue, buffer, ms, false );
}


/* See os.h for details. */
bool os_queue_send_to_back_ISR_std( queue_handle_t queue,
                                    const void *buffer,
                                    bool *hp_task_woke )
{
    *hp_task_woke = false;
    return os_queue_send_to_back( queue, buffer, WAIT_FOREVER );
}


/* See os.h for details. */
bool os_queue_send_to_front_std( queue_handle_t queue,
                                 const void *buffer,
                                 uint32_t ms )
{
    return __queue_push( queue, buffer, ms, true );
}


/* See os.h for details. */
bool os_queue_send_to_front_ISR_std( queue_handle_t queue,
                                     const void *buffer,
                                     bool *hp_task_woke )
{
    *hp_task_woke = false;
    return os_queue_send_to_front( queue, buffer, WAIT_FOREVER );
}


/*----------------------------------------------------------------------------*/
/*                    External Semaphore Related Functions                    */
/*----------------------------------------------------------------------------*/
/* See os.h for details. */
semaphore_handle_t os_semaphore_create_binary_std( void )
{
    mock_sem_t *sem;
    int i, rv;

    sem = NULL;
    __lock();
    for( i = 0; i < OS_MAX_SEM_COUNT; i++ ) {
        if( false == __sems[i].active ) {
            __sems[i].active = true;
            sem = &__sems[i];
            break;
        }
    }
    __unlock();
    assert( NULL != sem );

    rv = pthread_mutex_init( &sem->mutex, NULL );
    assert( 0 == rv );

    rv = pthread_cond_init( &sem->cond, NULL );
    assert( 0 == rv );

    sem->count = 1;

    return sem;
}


/* See os.h for details. */
void os_semaphore_delete_std( semaphore_handle_t semaphore )
{
    mock_sem_t *sem;
    int rv;

    __lock();

    sem = (mock_sem_t*) semaphore;
    assert( NULL != sem );

    rv = pthread_mutex_destroy( &sem->mutex );
    assert( 0 == rv );

    rv = pthread_cond_destroy( &sem->cond );
    assert( 0 == rv );

    sem->active = false;

    __unlock();
}


/* See os.h for details. */
bool os_semaphore_take_std( semaphore_handle_t semaphore, uint32_t ms )
{
    mock_sem_t *sem;
    int rv;
    struct timespec e;
    __get_expire_time( &e, ms * 1000000 );

    sem = (mock_sem_t*) semaphore;
    assert( NULL != sem );

    rv = pthread_mutex_lock( &sem->mutex );
    assert( 0 == rv );

    rv = 0;
    while( (0 == sem->count) && (0 == rv) ) {
        rv = pthread_cond_timedwait( &sem->cond, &sem->mutex, &e );
    }
    if( ETIMEDOUT == rv ) {
        rv = pthread_mutex_unlock( &sem->mutex );
        assert( 0 == rv );

        return false;
    }

    assert( 0 == rv );
    sem->count = 0;

    rv = pthread_mutex_unlock( &sem->mutex );
    assert( 0 == rv );

    return true;
}


/* See os.h for details. */
bool os_semaphore_give_std( semaphore_handle_t semaphore )
{
    mock_sem_t *sem;
    int rv;

    sem = (mock_sem_t*) semaphore;
    assert( NULL != sem );

    rv = pthread_mutex_lock( &sem->mutex );
    assert( 0 == rv );

    pthread_cond_signal( &sem->cond );
    sem->count = 1;

    rv = pthread_mutex_unlock( &sem->mutex );
    assert( 0 == rv );

    return true;
}


/* See os.h for details. */
bool os_semaphore_give_ISR_std( semaphore_handle_t semaphore,
                                bool *hp_task_woke )
{
    *hp_task_woke = false;

    return os_semaphore_give( semaphore );
}


/*----------------------------------------------------------------------------*/
/*                      External Mutex Related Functions                      */
/*----------------------------------------------------------------------------*/
/* See os.h for details. */
mutex_handle_t os_mutex_create_std( void )
{
    mock_mutex_t *mutex;
    int i, rv;

    mutex = NULL;
    __lock();
    for( i = 0; i < OS_MAX_MUTEX_COUNT; i++ ) {
        if( false == __mutexes[i].active ) {
            mutex = &__mutexes[i];
            break;
        }
    }
    __unlock();
    assert( NULL != mutex );

    rv = pthread_mutex_init( &mutex->mutex, NULL );
    assert( 0 == rv );

    return (mutex_handle_t) mutex;
}


/* See os.h for details. */
void os_mutex_delete_std( mutex_handle_t mutex )
{
    int rv;
    mock_mutex_t *m;

    __lock();

    assert( NULL != mutex );
    m = (mock_mutex_t*) mutex;

    rv = pthread_mutex_destroy( &m->mutex );
    assert( 0 == rv );

    m->active = false;

    __unlock();
}


/* See os.h for details. */
bool os_mutex_take_std( mutex_handle_t mutex, uint32_t ms )
{
    mock_mutex_t *m;
    struct timespec e;
    int rv;

    assert( NULL != mutex );
    m = (mock_mutex_t*) mutex;

    __get_expire_time( &e, ms * 1000000 );

    rv = pthread_mutex_timedlock( &m->mutex, &e );
    if( ETIMEDOUT == rv ) {
        return false;
    }

    return true;
}


/* See os.h for details. */
bool os_mutex_give_std( mutex_handle_t mutex )
{
    mock_mutex_t *m;
    int rv;

    assert( NULL != mutex );
    m = (mock_mutex_t*) mutex;

    rv = pthread_mutex_unlock( &m->mutex );
    assert( 0 == rv );

    return true;
}


/*----------------------------------------------------------------------------*/
/*                      Internal Task Related Functions                       */
/*----------------------------------------------------------------------------*/
static void* __task_wrapper( void *params )
{
    mock_task_t *task;

    task = (mock_task_t*) params;

    (*task->task_fn)( task->params );

    __lock();
    task->active = false;
    free( task->name );
    task->name = NULL;
    __unlock();

    pthread_detach(pthread_self());

    return NULL;
}

static void __task_delay( struct timespec *until )
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int rv;

    rv = pthread_mutex_init( &mutex, NULL );
    assert( 0 == rv );

    rv = pthread_cond_init( &cond, NULL );
    assert( 0 == rv );

    rv = pthread_mutex_lock( &mutex );
    assert( 0 == rv );

    rv = pthread_cond_timedwait( &cond, &mutex, until );

    rv = pthread_mutex_unlock( &mutex );
    assert( 0 == rv );

    rv = pthread_cond_destroy( &cond );
    assert( 0 == rv );

    rv = pthread_mutex_destroy( &mutex );
    assert( 0 == rv );
}

static void __get_expire_time( struct timespec *t, uint64_t ns )
{
    struct timeval now;

    gettimeofday( &now, NULL );

    t->tv_sec = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;

    while( 1000000000ULL < ns ) {
        t->tv_sec++;
        ns -= 1000000000ULL;
    }

    t->tv_nsec += ns;

    if( 1000000000ULL <= t->tv_nsec ) {
        t->tv_sec++;
        t->tv_nsec -= 1000000000ULL;
    }
}

/*----------------------------------------------------------------------------*/
/*                      Internal Queue Related Functions                      */
/*----------------------------------------------------------------------------*/
static bool __queue_peek( queue_handle_t queue, void *buffer, uint32_t ms,
                          bool peek )
{
    struct timespec e;
    mock_queue_node_t *n;
    mock_queue_t *q;
    int rv, i;
    bool out;

    __get_expire_time( &e, ms * 1000000 );

    q = (mock_queue_t*) queue;
    assert( NULL != q );

    rv = pthread_mutex_lock( &q->mutex );
    if( EINVAL == rv ) {
        return false;
    }

    assert( rv == 0 );

    n = q->queued;
    while( NULL == n ) {
        if( NO_WAIT == ms ) {
            break;
        } else if( WAIT_FOREVER == ms ) {
            rv = pthread_cond_wait( &q->cond_queued, &q->mutex );
        } else {
            rv = pthread_cond_timedwait( &q->cond_queued, &q->mutex, &e );
        }

        if( 0 != rv ) {
            break;
        }
        n = q->queued;
    }

    out = false;
    if( NULL != n ) {
        memcpy( buffer, n->data, q->data_size );
        if( false == peek ) {
            q->queued = n->next;
            n->next = q->idle;
            q->idle = n;
            pthread_cond_signal( &q->cond_idle );
        }
        out = true;
    }

    rv = pthread_mutex_unlock( &q->mutex );
    assert( rv == 0 );
    return out;
}


static bool __queue_push( queue_handle_t queue, const void *buffer, uint32_t ms,
                          bool head )
{
    struct timespec e;
    mock_queue_node_t *n;
    mock_queue_t *q;
    int rv, i;
    bool out;

    __get_expire_time( &e, ms * 1000000 );

    q = (mock_queue_t*) queue;
    assert( NULL != q );

    rv = pthread_mutex_lock( &q->mutex );
    assert( rv == 0 );

    n = q->idle;
    while( NULL == n ) {
        if( NO_WAIT == ms ) {
            break;
        } else if( WAIT_FOREVER == ms ) {
            rv = pthread_cond_wait( &q->cond_idle, &q->mutex );
        } else {
            rv = pthread_cond_timedwait( &q->cond_idle, &q->mutex, &e );
        }

        if( 0 != rv ) {
            break;
        }
        n = q->idle;
    }

    out = false;
    if( NULL != n ) {
        q->idle = q->idle->next;
        memcpy( n->data, buffer, q->data_size );
        if( true == head ) {
            n->next = q->queued;
            q->queued = n;
        } else {
            n->next = NULL;

            if( NULL == q->queued ) {
                q->queued = n;
            } else {
                mock_queue_node_t *tmp;

                for( tmp = q->queued; NULL != tmp->next; tmp = tmp->next ) {
                    ;
                }
                tmp->next = n;
            }
        }
        out = true;
        pthread_cond_signal( &q->cond_queued );
    }

    rv = pthread_mutex_unlock( &q->mutex );
    assert( rv == 0 );
    return out;
}

/*----------------------------------------------------------------------------*/
/*                    Internal Semaphore Related Functions                    */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                      Internal Mutex Related Functions                      */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/
/**
 *  Locks the creation/desctruction mutex
 */
static void __lock( void )
{
    int rv;

    rv = pthread_mutex_lock( &__internal_mutex );
    assert( rv == 0 );
}

/**
 *  Unlocks the creation/desctruction mutex
 */
static void __unlock( void )
{
    int rv;

    rv = pthread_mutex_unlock( &__internal_mutex );
    assert( rv == 0 );
}
