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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <freertos/task.h>
#include <freertos/queue.h>

#include "playback.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define PB_TASK_STACK_SIZE  (configMINIMAL_STACK_SIZE + 5000)
#define PB_COMMAND_MSG_MAX  10
#define IDLE_QUEUE_SIZE     10

#define PB_DEBUG 0

#define _D1(...)
#define _D2(...)

#if (defined(PB_DEBUG) && (0 < PB_DEBUG))
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif

#if (defined(PB_DEBUG) && (1 < PB_DEBUG))
#undef  _D2
#define _D2(...) printf( __VA_ARGS__ )
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef enum {
    PB_CMD_INT__PLAY,
    PB_CMD_INT__RESUME,
    PB_CMD_INT__PAUSE,
    PB_CMD_INT__STOP
} pb_command_int_t;

typedef struct {
    pb_command_int_t cmd;
    int32_t tx_id;
    playback_callback_fn_t cb_fn;

    char *filename;
    media_play_fn_t play_fn;
    uint32_t gain;
    uint32_t peak;
} pb_command_msg_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static xQueueHandle __idle;

static xQueueHandle __cmd_idle;
static xQueueHandle __cmd_active;
static pb_command_msg_t __commands[PB_COMMAND_MSG_MAX];

static uint16_t __tx_id = 0;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __pb_task( void *params );
static void __notify_and_return( const pb_status_t status,
                                 pb_command_msg_t *cmd );
static bool __continue_decoding( void );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/* See playback.h for details. */
int32_t playback_init( const uint32_t priority )
{
    portBASE_TYPE status;
    int i;

    __idle = NULL;
    __cmd_idle = NULL;
    __cmd_active = NULL;

    __idle = xQueueCreate( IDLE_QUEUE_SIZE, sizeof(void*) );

    __cmd_idle = xQueueCreate( PB_COMMAND_MSG_MAX, sizeof(void*) );
    __cmd_active = xQueueCreate( PB_COMMAND_MSG_MAX, sizeof(void*) );

    if( (NULL == __idle) || (NULL == __cmd_idle) || (NULL == __cmd_active) ) {
        goto failure;
    }

    for( i = 0; i < PB_COMMAND_MSG_MAX; i++ ) {
        pb_command_msg_t *cmd = &__commands[i];
        xQueueSendToBack( __cmd_idle, &cmd, portMAX_DELAY );
    }

    status = xTaskCreate( __pb_task, ( signed portCHAR *) "Playbck",
                          PB_TASK_STACK_SIZE, NULL, priority, NULL );

    if( pdPASS == status ) {
        return 0;
    }

failure:
    if( NULL != __idle ) {
        vQueueDelete( __idle );
        __idle = NULL;
    }
    if( NULL != __cmd_idle ) {
        vQueueDelete( __cmd_idle );
        __cmd_idle = NULL;
    }
    if( NULL != __cmd_active ) {
        vQueueDelete( __cmd_active );
        __cmd_active = NULL;
    }

    return -1;
}

/* See playback.h for details. */
int32_t playback_play( const char *filename,
                       const uint32_t gain,
                       const uint32_t peak,
                       media_play_fn_t play_fn,
                       playback_callback_fn_t cb_fn )
{
    _D2( "%s( '%s', %lu, %lu, %p, %p )\n",
         __func__, filename, gain, peak, play_fn, cb_fn );

    if( NULL != filename ) {
        pb_command_msg_t *cmd;
        int32_t tx_temp;

        xQueueReceive( __cmd_idle, &cmd, portMAX_DELAY );
        cmd->cmd = PB_CMD_INT__PLAY;
        cmd->tx_id = __tx_id++;
        cmd->gain = gain;
        cmd->peak = peak;
        cmd->play_fn = play_fn;
        cmd->filename = (char*) malloc( strlen(filename) + 1 );
        cmd->cb_fn = cb_fn;
        if( NULL == cmd->filename ) {
            xQueueSendToBack( __cmd_idle, &cmd, portMAX_DELAY );
            return -1;
        }
        strcpy( cmd->filename, filename );

        tx_temp = cmd->tx_id;
        xQueueSendToBack( __cmd_active, &cmd, portMAX_DELAY );

        return tx_temp;
    }
    return -1;
}

/* See playback.h for details. */
int32_t playback_command( const pb_command_t command,
                          playback_callback_fn_t cb_fn )
{
    pb_command_msg_t *cmd;
    pb_command_int_t cmd_int;
    int32_t tx_temp;

    xQueueReceive( __cmd_idle, &cmd, portMAX_DELAY );

    switch( command ) {
        case PB_CMD__RESUME:    cmd_int = PB_CMD_INT__RESUME;   break;
        case PB_CMD__PAUSE:     cmd_int = PB_CMD_INT__PAUSE;    break;
        case PB_CMD__STOP:      cmd_int = PB_CMD_INT__STOP;     break;
        default:
            return -1;
    }

    cmd->cmd = cmd_int;
    cmd->tx_id = __tx_id++;
    cmd->filename = NULL;

    tx_temp = cmd->tx_id;
    xQueueSendToBack( __cmd_active, &cmd, portMAX_DELAY );

    return tx_temp;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __pb_task( void *params )
{
    while( 1 ) {
        pb_command_msg_t *cmd;

        _D2( "Waiting on file to play\n" );
        xQueueReceive( __cmd_active, &cmd, portMAX_DELAY );

        if( PB_CMD_INT__PLAY == cmd->cmd ) {
            pb_status_t cb_status;
            media_status_t media_status;

            if( NULL != cmd->cb_fn ) {
                (*cmd->cb_fn)( PB_STATUS__PLAYING, cmd->tx_id );
            }

            _D2( "Playing song: '%s'\n", cmd->filename );
            media_status = (*cmd->play_fn)( cmd->filename,
                                            cmd->gain, cmd->peak,
                                            __idle, IDLE_QUEUE_SIZE,
                                            &malloc, &free,
                                            &__continue_decoding );
            _D2( "Done playing song: '%s' 0x%08x\n", cmd->filename, media_status );

            free( cmd->filename );
            cmd->filename = NULL;

            if( MI_END_OF_SONG == media_status ) {
                cb_status = PB_STATUS__END_OF_SONG;
            } else if( MI_STOPPED_BY_REQUEST == media_status ) {
                cb_status = PB_STATUS__STOPPED;
            } else {
                cb_status = PB_STATUS__ERROR;
            }
            __notify_and_return( cb_status, cmd );
            cmd = NULL;

        } else {
            /* We are idle. */
            __notify_and_return( PB_STATUS__ERROR, cmd );
            cmd = NULL;
        }
    }
}

static void __notify_and_return( const pb_status_t status,
                                 pb_command_msg_t *cmd )
{
    if( NULL != cmd->cb_fn ) {
        (*cmd->cb_fn)( status, cmd->tx_id );
    }

    cmd->filename = NULL;
    cmd->gain = 0;
    cmd->peak = 0;
    cmd->play_fn = NULL;

    xQueueSendToBack( __cmd_idle, &cmd, portMAX_DELAY );
}

static bool __continue_decoding( void )
{
    pb_command_msg_t *cmd;

    if( pdTRUE == xQueuePeek(__cmd_active, &cmd, 0) ) {

        /* Pause? */
        if( PB_CMD_INT__PAUSE == cmd->cmd ) {
            /* Yes */
            xQueueReceive( __cmd_active, &cmd, portMAX_DELAY );
            __notify_and_return( PB_STATUS__PAUSED, cmd );
            cmd = NULL;

            /* Wait for a resume, or error out & stop */
            xQueuePeek( __cmd_active, &cmd, portMAX_DELAY );
            if( PB_CMD_INT__RESUME == cmd->cmd ) {
                xQueueReceive( __cmd_active, &cmd, portMAX_DELAY );
                __notify_and_return( PB_STATUS__PLAYING, cmd );
                cmd = NULL;

                return true;
            }
        }

        return false;
    }

    return true;
}
