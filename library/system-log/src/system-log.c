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

#include <freertos/os.h>
#include <memcard/memcard.h>

#include "system-log.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define LOG_MSG_MAX             80
#define LOG_MAX_MESSAGE_SIZE    10
#define LOG_STACK_DEPTH         100

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    size_t size;
    uint8_t buffer[LOG_MAX_MESSAGE_SIZE];
} log_msg_t;

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
    size_t left;

    if( 0 == __enable ) {
        return -1;
    }

    if( MC_CARD__MOUNTED != mc_get_status() ) {
        return -1;
    }

    left = len;
    while( left ) {
        size_t send;
        log_msg_t *msg;

        send = LOG_MAX_MESSAGE_SIZE;
        if( left < LOG_MAX_MESSAGE_SIZE ) {
            send = left;
        }
        left -= send;

        os_queue_receive( __log_idle, &msg, WAIT_FOREVER );
        memcpy( msg->buffer, buf, send );
        buf += send;
        msg->size = send;
        os_queue_send_to_back( __log_pending, &msg, WAIT_FOREVER );
    }

    return len;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __log_task( void *data )
{
    char *filename;

    filename = (char *) data;   /* really a const, so don't change */

    __enable = 1;

    while( 1 ) {
        log_msg_t *msg;

        os_queue_receive( __log_pending, &msg, WAIT_FOREVER );

        if( MC_CARD__MOUNTED == mc_get_status() ) {
            int i;
            FILE *fp;

            fp = fopen( filename, "a" );
            for( i = 0; i < msg->size; i++ ) {
                fputc( msg->buffer[i], fp );
            }

            //fflush( fp );
            fclose( fp );
        }

        os_queue_send_to_back( __log_idle, &msg, WAIT_FOREVER );
    }
}
