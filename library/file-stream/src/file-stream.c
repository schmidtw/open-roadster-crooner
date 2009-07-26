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


#include <string.h>
#include <stdio.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <fatfs/ff.h>

#include "file-stream.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define FSTREAM_TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE + 400)
#define FSTREAM_BIG_BUFFER_SIZE         (32*1024)
#define FSTREAM_TOTAL_BUFFER_SIZE       (128*1024)
#define FSTREAM_SMALL_BUFFER_SIZE       512

#define MIN(a,b)    ((a) < (b)) ? (a) : (b)

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
    bool last;
} fstream_buffer_t;

typedef enum {
    FSTS__IDLE,
    FSTS__STREAMING
} fs_task_state_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static FIL __file;

static volatile fs_task_state_t __state;
static volatile fs_task_state_t __goal;

static fstream_buffer_t __big_buffer;

static xQueueHandle __active;
static xQueueHandle __idle;
static xSemaphoreHandle __wakeup;

fstream_free_fct __free_fn;

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
    uint32_t queue_size;

    _D1( "%s( %lu, %p, %p )\n", __func__, priority, malloc_fn, free_fn );

    /* Free is optional. */
    if( NULL == malloc_fn ) {
        return false;
    }

    __state = FSTS__IDLE;
    __goal = FSTS__IDLE;

    __free_fn = free_fn;

    vSemaphoreCreateBinary( __wakeup );
    xSemaphoreTake( __wakeup, 0 );

    queue_size = FSTREAM_TOTAL_BUFFER_SIZE / FSTREAM_SMALL_BUFFER_SIZE;

    __idle = xQueueCreate( queue_size, sizeof(fstream_buffer_t*) );
    __active = xQueueCreate( queue_size, sizeof(fstream_buffer_t*) );

    __big_buffer.valid_bytes = 0;

    __big_buffer.buffer = (uint8_t *)(*malloc_fn)( FSTREAM_BIG_BUFFER_SIZE + FSTREAM_SMALL_BUFFER_SIZE );

    for( i = 0; i < FSTREAM_TOTAL_BUFFER_SIZE; i += FSTREAM_SMALL_BUFFER_SIZE ) {
        fstream_buffer_t *n;

        n = (fstream_buffer_t*) (*malloc_fn)( sizeof(fstream_buffer_t) );
        n->buffer = (uint8_t *) (*malloc_fn)( FSTREAM_SMALL_BUFFER_SIZE );
        n->valid_bytes = 0;

        xQueueSendToBack( __idle, &n, 0 );
    }

    xTaskCreate( __task, (signed portCHAR *) "FSTR",
                 FSTREAM_TASK_STACK_SIZE, NULL, priority,
                 NULL );

    return true;
}

/** Details in file-stream.h */
bool fstream_open( const char *filename )
{
    _D1( "%s( %p )\n", __func__, filename );
    if( NULL == filename ) {
        _D1( "%s( %p ) -> failure\n", __func__, filename );
        return false;
    }

    fstream_close();

    if( FR_OK != f_open(&__file, filename, FA_READ|FA_OPEN_EXISTING) ) {
        _D1( "%s( %p ) -> failure\n", __func__, filename );
        return false;
    }

    __big_buffer.valid_bytes = 0;

    __goal = FSTS__STREAMING;
    xSemaphoreGive( __wakeup );

    _D2( "%s( %p ) -> success\n", __func__, filename );
    return true;
}

/** Details in file-stream.h */
void* fstream_get_buffer( const size_t wanted, size_t *got )
{
    bool eof;

    _D1( "%s( %ld, %p ) -> ?\n", __func__, wanted, got );
    if( (0 == wanted) || (NULL == got) || (FSTREAM_BIG_BUFFER_SIZE < wanted) ) {
        _D1( "%s( %ld, %p ) -> NULL\n", __func__, wanted, got );
        return NULL;
    }

    eof = false;
    while( (__big_buffer.valid_bytes < wanted) && (false == eof) ) {
        portBASE_TYPE rv;
        fstream_buffer_t *node;

        if( FSTS__IDLE == __state ) {
            rv = xQueueReceive( __active, &node, 0 );
        } else {
            rv = xQueueReceive( __active, &node, portMAX_DELAY );
        }

        if( pdFALSE == rv ) {
            eof = true;
        } else {
            memcpy( &__big_buffer.buffer[__big_buffer.valid_bytes],
                        node->buffer, node->valid_bytes );

            __big_buffer.valid_bytes += node->valid_bytes;

            eof = node->last;
            xQueueSendToBack( __idle, &node, 0 );
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
    if( 0 < consumed ) {
        if( consumed <= __big_buffer.valid_bytes ) {

            if( consumed < __big_buffer.valid_bytes ) {
                memmove( __big_buffer.buffer, &__big_buffer.buffer[consumed],
                         (__big_buffer.valid_bytes - consumed) );
            }

            __big_buffer.valid_bytes -= consumed;
        }
    }

    _D2( "%s( %ld )\n", __func__, consumed );
}

/** Details in file-stream.h */
void fstream_skip( const size_t skip )
{
    _D1( "%s( %ld )\n", __func__, skip );

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
    _D2( "%s( %ld )\n", __func__, skip );
}

/** Details in file-stream.h */
void fstream_close( void )
{
    fstream_buffer_t *node;

    _D1( "%s()\n", __func__ );
    __goal = FSTS__IDLE;

    /* Clear out any active data so we unblock the thread. */
    while( pdTRUE == xQueueReceive(__active, &node, 0) ) {
        xQueueSendToBack( __idle, &node, 0 );
    }

    /* Wait for it to shut down. */
    while( FSTS__STREAMING == __state ) {
        vTaskDelay( TASK_DELAY_MS(10) );
    }

    /* Clear out any active data. */
    while( pdTRUE == xQueueReceive(__active, &node, 0) ) {
        xQueueSendToBack( __idle, &node, 0 );
    }

    f_close( &__file );
    __file.fsize = 0;
    __big_buffer.valid_bytes = 0;
    _D2( "%s()\n", __func__ );
}

/** Details in file-stream.h */
void fstream_destroy( void )
{
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
    while( 1 ) {
        _D2( "%s:__task: FSTS__IDLE\n", __FILE__ );
        __state = FSTS__IDLE;
        xSemaphoreTake( __wakeup, portMAX_DELAY );

        if( FSTS__STREAMING == __goal ) {
            uint32_t bytes_left;

            __state = FSTS__STREAMING;
            bytes_left = __file.fsize;

            /* read the file */
            while( 0 < bytes_left ) {
                uint32_t requested, read;
                fstream_buffer_t *node;

                xQueueReceive( __idle, &node, portMAX_DELAY );

                requested = MIN( FSTREAM_SMALL_BUFFER_SIZE, bytes_left );
                if( FR_OK != f_read(&__file, node->buffer, requested, (UINT*)&read) ) {
                    bytes_left = 0;
                } else {
                    if( read == requested ) {
                        node->valid_bytes = read;
                        bytes_left -= read;
                        node->last = (0 == bytes_left) ? true : false;
                        xQueueSendToBack( __active, &node, 0 );
                    } else {
                        bytes_left = 0;
                    }
                }

                if( FSTS__IDLE == __goal ) {
                    bytes_left = 0;
                }

                _D2( "%s:__task: bytes_left: %lu\n", __FILE__, bytes_left );
            }

            _D2( "%s:__task: done\n", __FILE__ );
        }
    }
}
