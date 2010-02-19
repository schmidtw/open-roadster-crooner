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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

#include "file-stream.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define FSTREAM_TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE+100)
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

typedef struct {
    fs_task_state_t cmd;
    char *name;
} fstream_command_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static volatile int __filesize;

static volatile fs_task_state_t __state;

static fstream_buffer_t __big_buffer;

/* Data queues */
static xQueueHandle __data_active;
static xQueueHandle __data_idle;

/* Control queues */
static xQueueHandle __command_active;
static xQueueHandle __command_idle;

static fstream_command_t __command;

static fstream_free_fct __free_fn;
static fstream_malloc_fct __malloc_fn;

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

    if( (NULL == malloc_fn) || (NULL == free_fn) ) {
        return false;
    }

    __state = FSTS__IDLE;

    __free_fn = free_fn;
    __malloc_fn = malloc_fn;

    queue_size = FSTREAM_TOTAL_BUFFER_SIZE / FSTREAM_SMALL_BUFFER_SIZE;

    __data_idle = xQueueCreate( queue_size, sizeof(fstream_buffer_t*) );
    __data_active = xQueueCreate( queue_size, sizeof(fstream_buffer_t*) );

    __command_idle = xQueueCreate( 1, sizeof(fstream_buffer_t*) );
    __command_active = xQueueCreate( 1, sizeof(fstream_buffer_t*) );

    {
        fstream_command_t *cmd;
        cmd = &__command;
        xQueueSendToBack( __command_idle, &cmd, 0 );
    }

    __big_buffer.valid_bytes = 0;

    __big_buffer.buffer = (uint8_t *)(*malloc_fn)( FSTREAM_BIG_BUFFER_SIZE + FSTREAM_SMALL_BUFFER_SIZE );

    for( i = 0; i < FSTREAM_TOTAL_BUFFER_SIZE; i += FSTREAM_SMALL_BUFFER_SIZE ) {
        fstream_buffer_t *n;

        n = (fstream_buffer_t*) (*malloc_fn)( sizeof(fstream_buffer_t) );
        n->buffer = (uint8_t *) (*malloc_fn)( FSTREAM_SMALL_BUFFER_SIZE );
        n->valid_bytes = 0;

        xQueueSendToBack( __data_idle, &n, 0 );
    }

    xTaskCreate( __task, (signed portCHAR *) "FileStrm",
                 FSTREAM_TASK_STACK_SIZE, NULL, priority,
                 NULL );

    return true;
}

/** Details in file-stream.h */
bool fstream_open( const char *filename )
{
    fstream_command_t *cmd;

    _D1( "%s( '%s' )\n", __func__, filename );
    if( NULL == filename ) {
        _D1( "%s( %p ) -> failure\n", __func__, filename );
        return false;
    }

    fstream_close();

    xQueueReceive( __command_idle, &cmd, portMAX_DELAY );

    cmd->name = (char *) (*__malloc_fn)( strlen(filename) + 1 );
    if( NULL == cmd->name ) {
        xQueueSendToBack( __command_idle, &cmd, 0 );
        _D1( "%s( '%s' ) -> failure\n", __func__, filename );
        return false;
    }
    strcpy( cmd->name, filename );
    cmd->cmd = FSTS__STREAMING;

    xQueueSendToBack( __command_active, &cmd, 0 );

    /* Wait for the command to be processed. */
    xQueuePeek( __command_idle, &cmd, portMAX_DELAY );

    if( FSTS__STREAMING == cmd->cmd ) {
        _D2( "%s( %p ) -> success\n", __func__, filename );
        return true;
    }

    _D1( "%s( %p ) -> failure\n", __func__, filename );
    return false;
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
            rv = xQueueReceive( __data_active, &node, 0 );
        } else {
            rv = xQueueReceive( __data_active, &node, portMAX_DELAY );
        }

        if( pdFALSE == rv ) {
            eof = true;
        } else {
            memcpy( &__big_buffer.buffer[__big_buffer.valid_bytes],
                        node->buffer, node->valid_bytes );

            __big_buffer.valid_bytes += node->valid_bytes;

            eof = node->last;
            xQueueSendToBack( __data_idle, &node, 0 );
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
    fstream_command_t *cmd;
    fstream_buffer_t *node;

    _D1( "%s()\n", __func__ );
    xQueueReceive( __command_idle, &cmd, portMAX_DELAY );
    cmd->cmd = FSTS__IDLE;
    cmd->name = NULL;
    xQueueSendToBack( __command_active, &cmd, 0 );

    /* Clear out any active data so we unblock the thread. */
    while( pdTRUE == xQueueReceive(__data_active, &node, 0) ) {
        xQueueSendToBack( __data_idle, &node, 0 );
    }

    /* Wait for it to shut down. */
    xQueuePeek( __command_idle, &cmd, portMAX_DELAY );

    /* Clear out any active data. */
    while( pdTRUE == xQueueReceive(__data_active, &node, 0) ) {
        xQueueSendToBack( __data_idle, &node, 0 );
    }

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
    if( FSTS__STREAMING == __state ) {
        return __filesize;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

void __task( void *params )
{
    while( 1 ) {
        fstream_command_t *cmd;

        _D2( "Waiting on command\n" );
        xQueueReceive( __command_active, &cmd, portMAX_DELAY );

        if( FSTS__STREAMING == cmd->cmd ) {
            char *filename;
            int fd;

            filename = cmd->name;
            cmd->name = NULL;

            _D2( "Told to stream: '%s'\n", filename );
            fd = open( filename, O_RDONLY );
            if( -1 == fd ) {
                _D2( "Failed to open file\n" );
                /* Send the failure ack back. */
                cmd->cmd = FSTS__IDLE;
                xQueueSendToBack( __command_idle, &cmd, 0 );
            } else {
                struct stat st;

                _D2( "Opened file\n" );
                if( 0 != fstat(fd, &st) ) {
                    _D2( "Failed to fstat file\n" );
                    /* Send the failure ack back. */
                    cmd->cmd = FSTS__IDLE;
                    xQueueSendToBack( __command_idle, &cmd, 0 );
                } else {
                    uint32_t bytes_left;

                    bytes_left = (uint32_t) st.st_size;

                    _D2( "Got filesize: %ld\n", bytes_left );

                    __big_buffer.valid_bytes = 0;
                    __state = FSTS__STREAMING;

                    /* Send the success ack back. */
                    cmd->cmd = FSTS__STREAMING;
                    xQueueSendToBack( __command_idle, &cmd, 0 );
                    cmd = NULL;

                    _D2( "Streaming\n" );
                    /* read the file */
                    while( 0 < bytes_left ) {
                        if( pdTRUE == xQueueReceive(__command_active, &cmd, 0) ) {
                            /* we're done with this file, stop reading */
                            bytes_left = 0;
                            _D2( "Told to Stop\n" );
                        } else {
                            size_t requested;
                            ssize_t bytes_read;
                            fstream_buffer_t *node;

                            xQueueReceive( __data_idle, &node, portMAX_DELAY );

                            requested = MIN( FSTREAM_SMALL_BUFFER_SIZE, bytes_left );
                            bytes_read = read( fd, node->buffer, requested );
                            if( -1 == bytes_read ) {
                                bytes_left = 0;
                            } else {
                                if( bytes_read == requested ) {
                                    node->valid_bytes = bytes_read;
                                    bytes_left -= bytes_read;
                                    node->last = (0 == bytes_left) ? true : false;
                                    xQueueSendToBack( __data_active, &node, 0 );
                                } else {
                                    bytes_left = 0;
                                }
                            }
                        }
                    }

                    __state = FSTS__IDLE;
                    close( fd );
                    fd = -1;

                    if( NULL == cmd ) {
                        _D2( "End of the song\n" );
                    } else {
                        /* We were told to stop, ack. */
                        cmd->cmd = FSTS__IDLE;
                        xQueueSendToBack( __command_idle, &cmd, 0 );
                        _D2( "Stopping on command\n" );
                    }
                }
            }

            (*__free_fn)( filename );
            filename = NULL;
        } else {
            /* We are idle. */
            _D2( "Idle Ack\n" );
            cmd->cmd = FSTS__IDLE;
            xQueueSendToBack( __command_idle, &cmd, 0 );
        }
    }
}
