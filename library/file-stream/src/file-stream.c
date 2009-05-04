/*
 * Copyright (c) 2009  Weston Schmidt
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
#undef PERFORMANCE_METRIC_OUTPUT


#include <string.h>
#include <stdio.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <linked-list/linked-list.h>
#include <fatfs/ff.h>

#ifdef PERFORMANCE_METRIC_OUTPUT
#include <bsp/abdac.h>
#endif

#include "file-stream.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define FSTREAM_TASK_STACK_SIZE         1000
#define FSTREAM_BIG_BUFFER_SIZE         (32*1024)
#define FSTREAM_TOTAL_BUFFER_SIZE       (128*1024)
#define FSTREAM_SMALL_BUFFER_SIZE       512

#define MIN(a,b)    ((a) < (b)) ? (a) : (b)
#define FSTREAM_MUTEX_LOCK()            xSemaphoreTake( __mutex, portMAX_DELAY )
#define FSTREAM_MUTEX_UNLOCK()          xSemaphoreGive( __mutex )

#define _D1(...)
#define _D2(...)

#if (defined(FSTREAM_DEBUG) && (0 < FSTREAM_DEBUG))
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif

#if (defined(FSTREAM_DEBUG) && (1 < FSTREAM_DEBUG))
#undef  _D2
#define _D2(...) printf( __VA_ARGS__ )
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    uint8_t *buffer;
    size_t valid_bytes;
    ll_node_t node;
} fstream_buffer_t;

typedef struct {
    uint8_t *buffer;
    size_t valid_bytes;
} fstream_big_buffer_t;

#ifdef PERFORMANCE_METRIC_OUTPUT
typedef struct {
    uint32_t consumed;
    uint32_t underflow;
    uint32_t time;
} frame_data_t;
#endif

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static bool __setup;
static xSemaphoreHandle __mutex;
static xSemaphoreHandle __data_added;
static xSemaphoreHandle __buffer_added;
static xSemaphoreHandle __done;
static xTaskHandle __task_handle;

static bool __eof;
static bool __file_valid;
static FIL __file;

static fstream_big_buffer_t __big_buffer;

static ll_list_t __active;
static ll_list_t __idle;

fstream_malloc_fct __malloc_fn;
fstream_free_fct __free_fn;

#ifdef PERFORMANCE_METRIC_OUTPUT
uint32_t no_data_count;
uint32_t no_buffers_count;

frame_data_t __frame[3000];
#endif

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
void __task( void *params );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/** Details in file-stream.h */
bool fstream_init( const uint32_t priority,
                   fstream_malloc_fct malloc_fn,
                   fstream_free_fct free_fn )
{
    int32_t i;

    _D1( "%s( %lu, %p, %p )\n", __func__, priority, malloc_fn, free_fn );

#ifdef PERFORMANCE_METRIC_OUTPUT
    no_data_count = 0;
    no_buffers_count = 0;
#endif

    /* Free is optional. */
    if( (true == __setup) || (NULL == malloc_fn) ) {
        return false;
    }

    __malloc_fn = malloc_fn;
    __free_fn = free_fn;

    __mutex = xSemaphoreCreateMutex();

    vSemaphoreCreateBinary( __data_added );
    vSemaphoreCreateBinary( __buffer_added );
    vSemaphoreCreateBinary( __done );
    xSemaphoreTake( __data_added, 0 );
    xSemaphoreTake( __buffer_added, 0 );
    xSemaphoreTake( __done, 0 );

    ll_init_list( &__active );
    ll_init_list( &__idle );

    __big_buffer.valid_bytes = 0;

    __big_buffer.buffer = (uint8_t *)(*__malloc_fn)( FSTREAM_BIG_BUFFER_SIZE + FSTREAM_SMALL_BUFFER_SIZE );

    for( i = 0; i < FSTREAM_TOTAL_BUFFER_SIZE; i += FSTREAM_SMALL_BUFFER_SIZE ) {
        fstream_buffer_t *n;

        n = (fstream_buffer_t*) (*__malloc_fn)( sizeof(fstream_buffer_t) );
        n->buffer = (uint8_t *) (*__malloc_fn)( FSTREAM_SMALL_BUFFER_SIZE );
        n->valid_bytes = 0;
        ll_init_node( &n->node, n );
        ll_append( &__idle, &n->node );
    }

    __file_valid = false;
    __eof = false;

    /* Make sure we can create the tread & suspend it. */
    if( pdPASS != xTaskCreate(__task, (signed portCHAR *) "FSTR",
                              FSTREAM_TASK_STACK_SIZE, NULL, priority,
                              &__task_handle) )
    {
        if( NULL != __free_fn ) {
            (*__free_fn)( __big_buffer.buffer );
        }
        __big_buffer.valid_bytes = 0;
        return false;
    }

    __setup = true;

    return true;
}

/** Details in file-stream.h */
bool fstream_open( const char *filename )
{
    _D1( "%s( %p )\n", __func__, filename );
    if( (false == __setup) || (false != __file_valid) || (NULL == filename) ) {
        return false;
    }

    /* The thread is suspended */

    if( FR_OK != f_open(&__file, filename, FA_READ|FA_OPEN_EXISTING) ) {
        return false;
    }

    __file_valid = true;
    __eof = false;
    __big_buffer.valid_bytes = 0;
#ifdef PERFORMANCE_METRIC_OUTPUT
    memset( __frame, 0, sizeof(__frame) );
#endif

    xSemaphoreTake( __done, 0 );
    xSemaphoreGive( __buffer_added );
    _D2( "%s:%d\n", __FILE__, __LINE__ );
    vTaskResume( __task_handle );

    return true;
}

/** Details in file-stream.h */
void* fstream_get_buffer( const size_t wanted, size_t *got )
{
    _D1( "%s( %ld, %p ) -> ?\n", __func__, wanted, got );
    if( (false == __setup) || (0 == wanted) || (false == __file_valid) ||
        (NULL == got) || (FSTREAM_BIG_BUFFER_SIZE < wanted) )
    {
        _D2( "%s( %ld, %p ) -> NULL\n", __func__, wanted, got );
        return NULL;
    }

    while( __big_buffer.valid_bytes < wanted ) {
        ll_node_t *node;
        bool eof;

        FSTREAM_MUTEX_LOCK();
        node = ll_remove_head( &__active );
        eof = __eof;
        FSTREAM_MUTEX_UNLOCK();

        if( NULL == node ) {
            if( false == eof ) {
#ifdef PERFORMANCE_METRIC_OUTPUT
                no_data_count++;
#endif
                xSemaphoreTake( __data_added, portMAX_DELAY );
            } else {
                /* That's it ... we have all the data there is. */
                *got = MIN( wanted, __big_buffer.valid_bytes );
                _D2( "%s( %ld, %p ) -> %ld\n", __func__, wanted, got, *got );
                return __big_buffer.buffer;
            }
        } else {
            fstream_buffer_t *fnode;

            fnode = (fstream_buffer_t *) node->data;
            memcpy( &__big_buffer.buffer[__big_buffer.valid_bytes],
                    fnode->buffer, fnode->valid_bytes );
            __big_buffer.valid_bytes += fnode->valid_bytes;
            fnode->valid_bytes = 0;

            FSTREAM_MUTEX_LOCK();
            ll_append( &__idle, node );
            FSTREAM_MUTEX_UNLOCK();
            xSemaphoreGive( __buffer_added );
        }
    }

    *got = MIN( wanted, __big_buffer.valid_bytes );
    _D2( "%s( %ld, %p ) -> %ld (max: %ld)\n", __func__, wanted, got, *got, __big_buffer.valid_bytes );
    return __big_buffer.buffer;
}

/** Details in file-stream.h */
void fstream_release_buffer( const size_t consumed )
{
    _D1( "%s( %ld )\n", __func__, consumed );
    if( (true == __setup) && (consumed <= __big_buffer.valid_bytes) ) {

        if( consumed < __big_buffer.valid_bytes ) {
            memmove( __big_buffer.buffer, &__big_buffer.buffer[consumed],
                     (__big_buffer.valid_bytes - consumed) );
        }

        __big_buffer.valid_bytes -= consumed;
    }

#ifdef PERFORMANCE_METRIC_OUTPUT
    {
        static uint32_t frame = 0;

        __frame[frame].underflow = abdac_get_underflow();
        __frame[frame].consumed = consumed;
        __frame[frame].time = xTaskGetTickCount();
        frame++;
    }
#endif

    _D2( "%s( %ld )\n", __func__, consumed );
}

/** Details in file-stream.h */
void fstream_skip( const size_t skip )
{
    _D1( "%s( %ld )\n", __func__, skip );
    if( (true == __setup) && (true == __file_valid) ) {
        if( skip < __big_buffer.valid_bytes ) {
            fstream_release_buffer( skip );
        } else {
            size_t left = skip;
            left -= __big_buffer.valid_bytes;
            __big_buffer.valid_bytes = 0;
            while( 0 < left ) {
                size_t got, frame;

                frame = MIN( left, FSTREAM_BIG_BUFFER_SIZE );
                fstream_get_buffer( frame, &got );
                fstream_release_buffer( frame );
                left -= frame;
            }
        }
    }
    _D2( "%s( %ld )\n", __func__, skip );
}

/** Details in file-stream.h */
void fstream_close( void )
{
    _D1( "%s()\n", __func__ );
    if( (true == __setup) && (true == __file_valid) ) {
        __file_valid = false;
        vTaskResume( __task_handle );

        xSemaphoreTake( __done, portMAX_DELAY );
        f_close( &__file );
    }
    _D2( "%s()\n", __func__ );
}

/** Details in file-stream.h */
void fstream_destroy( void )
{
    ll_node_t *node;
    fstream_buffer_t *n;

    _D1( "%s()\n", __func__ );

    FSTREAM_MUTEX_LOCK();
    (*__free_fn)( __big_buffer.buffer );
    __big_buffer.buffer = NULL;

    node = ll_remove_head( &__idle );
    while( NULL != node ) {
        n = (fstream_buffer_t*) node->data;

        (*__free_fn)( n->buffer );
        (*__free_fn)( n );
    }
    FSTREAM_MUTEX_UNLOCK();

}

/** Details in file-stream.h */
uint32_t fstream_get_filesize( void )
{
    return (uint32_t) __file.fsize;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

void __task( void *params )
{
    vTaskSuspend( NULL );

    while( 1 ) {
        uint32_t read, last_requested;
        bool first;

        first = true;
        read = 0;
        last_requested = 0;

        while( (true == __file_valid) && (last_requested == read) ) {
            ll_node_t *node;

            FSTREAM_MUTEX_LOCK();
            node = ll_remove_head( &__idle );
            FSTREAM_MUTEX_UNLOCK();

            if( NULL == node ) {
                /* Sleep until someone makes some space by using the
                 * buffers */
#ifdef PERFORMANCE_METRIC_OUTPUT
                no_buffers_count++;
#endif
                if( true == first ) {
                    xSemaphoreGive( __data_added );
                    first = false;
                }
                xSemaphoreTake( __buffer_added, portMAX_DELAY );

                read = 0;
                last_requested = 0;
            } else {
                fstream_buffer_t *fnode;
                fnode = (fstream_buffer_t *) node->data;
                last_requested = FSTREAM_SMALL_BUFFER_SIZE;
                if( FR_OK != f_read(&__file, fnode->buffer, last_requested, (UINT*)&read) ) {
                    read = 0;
                }
                fnode->valid_bytes = read;

                FSTREAM_MUTEX_LOCK();
                ll_append( &__active, node );
                __eof = (last_requested != read);
                FSTREAM_MUTEX_UNLOCK();
                if( false == first ) {
                    xSemaphoreGive( __data_added );
                }
            }
        }

        while( true == __file_valid ) {
            /* File is completely read in. */
            vTaskSuspend( NULL );
        }

        /* Done with the file completely */
        FSTREAM_MUTEX_LOCK();
        while( NULL != __active.head ) {
            ll_node_t *n;
            n = ll_remove_head( &__active );
            ((fstream_buffer_t*) n->data)->valid_bytes = 0;
            ll_append( &__idle, n );
        }
        FSTREAM_MUTEX_UNLOCK();

#ifdef PERFORMANCE_METRIC_OUTPUT
        printf( "no_buffers_count: %lu no_data_count: %lu\n", no_buffers_count, no_data_count );
        printf( "total underflow count: %lu\n", abdac_get_underflow() );

        no_buffers_count = 0;
        no_data_count = 0;
#endif

        xSemaphoreGive( __done );
        vTaskSuspend( NULL );
    }
}
