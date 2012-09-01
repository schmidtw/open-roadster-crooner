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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reent.h>
#include <time.h>
#include <sys/time.h>

#include <freertos/os.h>
#include <fillable-buffer/fillable-buffer.h>
#include <memcard/memcard.h>

#include "system-log.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define LOG_DUMP_RATE_S         5
#define LOG_MSG_MAX             80
#define LOG_MAX_MESSAGE_SIZE    50
#define LOG_STACK_DEPTH         100

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    size_t size;
    uint8_t buffer[LOG_MAX_MESSAGE_SIZE];
} log_msg_t;

typedef struct {
    char *filename;
    uint32_t last;
} log_task_data_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static log_msg_t __log_messages[LOG_MSG_MAX];
static queue_handle_t __log_idle;
static queue_handle_t __log_pending;
static int __enable;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __log_task( void *data );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
syslog_status_t system_log_init( const uint32_t priority, const char *filename )
{
    int i;

    if( NULL == filename ) {
        return SYSLOG_RETURN_OK;
    }

    __log_pending = os_queue_create( LOG_MSG_MAX, sizeof(log_msg_t*) );
    __log_idle = os_queue_create( LOG_MSG_MAX, sizeof(log_msg_t*) );

    for( i = 0; i < LOG_MSG_MAX; i++ ) {
        log_msg_t *msg = &__log_messages[i];
        os_queue_send_to_back( __log_idle, &msg, NO_WAIT );
    }

    os_task_create( __log_task, "SysLog", LOG_STACK_DEPTH,
                    (void*) filename, priority, NULL );

    return SYSLOG_RETURN_OK;
}

int _error_write_r( struct _reent *reent, int fd, void *buf, size_t len )
{
    uint8_t *b;
    int sent;

    b = (uint8_t *) buf;

    if( 0 == __enable ) {
        return -1;
    }

    if( MC_CARD__MOUNTED != mc_get_status() ) {
        return -1;
    }

    sent = 0;
    while( 0 < len ) {
        size_t send;
        log_msg_t *msg;

        send = LOG_MAX_MESSAGE_SIZE;
        if( len < LOG_MAX_MESSAGE_SIZE ) {
            send = len;
        }

        os_queue_receive( __log_idle, &msg, WAIT_FOREVER );
        memcpy( msg->buffer, &b[sent], send );
        sent += send;
        len -= send;
        msg->size = send;
        os_queue_send_to_back( __log_pending, &msg, WAIT_FOREVER );
    }

    return sent;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  Returns the time since boot in seconds.
 */
static uint32_t __get_now( void )
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday( &tv, &tz );

    return tv.tv_sec;
}

/**
 *  Flushes the data out to the desired file and the stdout
 *
 *  @param data our log_task_data_t structure
 *  @param buf the buffer to send out
 *  @param size the size of the buffer to dump
 */
static void __flush( void *data, const uint8_t *buf, const size_t size )
{
    log_task_data_t *task_data;

    task_data = (log_task_data_t *) data;

    if( MC_CARD__MOUNTED == mc_get_status() ) {
        FILE *fp;
        fp = fopen( task_data->filename, "a" );
        if( NULL != fp ) {
            fwrite( buf, sizeof(uint8_t), size, fp );
            fclose( fp );
        }
    }

    task_data->last = __get_now();
    fwrite( buf, sizeof(uint8_t), size, stdout );
}

static void __log_task( void *data )
{
    log_task_data_t task_data;
    fillable_buffer_t fb;
    static uint8_t buf[512];
    uint32_t last;

    __enable = 1;

    task_data.filename = (char *) data;
    task_data.last = __get_now();

    fb.buf = buf;
    fb.size = sizeof(buf);
    fb.offset = 0;
    fb.data = &task_data;
    fb.flush = __flush;

    last = 0;
    memset( buf, 0, sizeof(buf) );

    while( 1 ) {
        bool rv;
        log_msg_t *msg;

        rv = os_queue_receive( __log_pending, &msg, 1000 );
        if( true == rv ) {
            fillbuf_append( &fb, msg->buffer, msg->size );
            os_queue_send_to_back( __log_idle, &msg, WAIT_FOREVER );
        }

        /* Force dump at LOG_DUMP_RATE_S seconds */
        if( (task_data.last + LOG_DUMP_RATE_S) < __get_now() ) {
            fillbuf_flush( &fb );
            task_data.last = __get_now();
        }
    }
}
