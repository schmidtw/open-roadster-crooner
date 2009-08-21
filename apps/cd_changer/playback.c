/*
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <fatfs/ff.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <memcard/memcard.h>
#include <database/database.h>

#include "radio-interface.h"
#include "playback.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define PB_TASK_STACK_SIZE  (configMINIMAL_STACK_SIZE + 5000)
#define PB_TASK_PRIORITY    (tskIDLE_PRIORITY+1)
#define DIR_MAP_SIZE        6
#define IDLE_QUEUE_SIZE     10
#define PB_COMMAND_MSG_MAX  20

#define PB_DEBUG 2

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
typedef struct {
    pb_command_t cmd;
    uint8_t disc;
} pb_command_msg_t;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __pb_task( void *params );
static void __mount_status( const mc_card_status_t status );
static uint8_t __determine_map( void );
static bool __continue_decoding( void );
static void __handle_messages( song_node_t **current );

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static xQueueHandle __idle;
static xSemaphoreHandle __card_mounted;
static volatile mc_card_status_t __card_status;
static const char *dir_map[DIR_MAP_SIZE] = { "1", "2", "3", "4", "5", "6" };

static xQueueHandle __commands_idle;
static xQueueHandle __commands_queued;
static pb_command_msg_t __commands[PB_COMMAND_MSG_MAX];
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int32_t playback_init( void )
{
    portBASE_TYPE status;
    int32_t i;
    
    _D1( "playback_init()\n" );

    __idle = NULL;
    __card_mounted = NULL;
    __commands_idle = NULL;
    __commands_queued = NULL;

    __idle = xQueueCreate( IDLE_QUEUE_SIZE, sizeof(void*) );
    __commands_idle = xQueueCreate( PB_COMMAND_MSG_MAX, sizeof(pb_command_msg_t*) );
    __commands_queued = xQueueCreate( PB_COMMAND_MSG_MAX, sizeof(pb_command_msg_t*) );

    vSemaphoreCreateBinary( __card_mounted );

    if( (NULL == __idle) || (NULL == __card_mounted) ||
        (NULL == __commands_idle) || (NULL == __commands_queued) )
    {
        goto failure;
    }

    for( i = 0; i < PB_COMMAND_MSG_MAX; i++ ) {
        pb_command_msg_t *cmd = &__commands[i];
        xQueueSendToBack( __commands_idle, &cmd, 0 );
    }

    xSemaphoreTake( __card_mounted, 0 );

    status = xTaskCreate( __pb_task, ( signed portCHAR *) "Playbck",
                          PB_TASK_STACK_SIZE, NULL, PB_TASK_PRIORITY, NULL );

    if( pdPASS != status ) {
        goto failure;
    }

    return 0;

failure:
    if( NULL == __idle ) {
        vQueueDelete( __idle );
    }
    if( NULL == __card_mounted ) {
    }
    if( NULL == __commands_idle ) {
        vQueueDelete( __commands_idle );
    }
    if( NULL == __commands_queued ) {
        vQueueDelete( __commands_queued );
    }

    return -1;
}

void playback_command( const pb_command_t command, const uint8_t disc )
{
    pb_command_msg_t *cmd;

    xQueueReceive( __commands_idle, &cmd, portMAX_DELAY );

    cmd->cmd = command;
    cmd->disc = disc;

    xQueueSendToBack( __commands_queued, &cmd, portMAX_DELAY );
}


/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __pb_task( void *params )
{
    song_node_t *current_song;
    mc_register( &__mount_status );

    while( 1 ) {
        FATFS fs;

        xSemaphoreTake( __card_mounted, portMAX_DELAY );

        if( 0 == f_mount(0, &fs) ) {

            _D1( "mounted fs.\n" );
            ri_checking_for_discs();
            if( true == populate_database(dir_map, DIR_MAP_SIZE, "/") ) {
                media_status_t ms;

                ri_checking_complete( __determine_map() );

                current_song = NULL;
                ms = MI_STOPPED_BY_REQUEST;
                next_song( &current_song, DT_NEXT, DL_SONG );
                while( MC_CARD__MOUNTED == __card_status ) {
                    media_play_fn_t play_fn;

                    __handle_messages( &current_song );

                    play_fn = current_song->play_fn;
                    _D2( "Playing song '%s'\n", current_song->title );
                    ms = (*play_fn)( (const char*) current_song->file_location,
                                     current_song->track_gain,
                                     current_song->track_peak,
                                     __idle, IDLE_QUEUE_SIZE, &malloc, &free,
                                     &__continue_decoding );
                    _D2( "Played song\n" );

                    if( MI_STOPPED_BY_REQUEST != ms ) {
                        db_status_t rv;

                        ri_song_ended_playing_next();

                        rv = next_song( &current_song, DT_NEXT, DL_SONG );
                        if( DS_END_OF_LIST == rv ) {
                            rv = next_song( &current_song, DT_NEXT, DL_ALBUM );
                        }
                        if( DS_END_OF_LIST == rv ) {
                            rv = next_song( &current_song, DT_NEXT, DL_ARTIST );
                        }
                        if( DS_FAILURE == rv ) {
                            break;
                        }
                    }
                }

                current_song = NULL;
                database_purge();
            } else {
                ri_checking_complete( 0x00 );
            }
        }
    }
}

static void __mount_status( const mc_card_status_t status )
{
    portBASE_TYPE ignore;

    __card_status = status;

    if( MC_CARD__MOUNTED == status ) {
        xSemaphoreGiveFromISR( __card_mounted, &ignore );
    }
}

static uint8_t __determine_map( void )
{
    db_status_t status;
    song_node_t *song;
    uint8_t map;

    song = NULL;
    map = 0x00;

    status = DS_SUCCESS;
    while( DS_SUCCESS == status ) {
        status = next_song( &song, DT_NEXT, DL_GROUP );

        if( DS_FAILURE == status ) {
            map = 0x00;
            goto done;
        } else if( DS_END_OF_LIST == status ) {
            goto done;
        } else if( DS_SUCCESS == status ) {
            int32_t i;

            for( i = 0; i < DIR_MAP_SIZE; i++ ) {
                if( 0 == strcmp(dir_map[i], song->album->artist->group->name) ) {
                    map |= 1 << i;
                    break;
                }
            }
        }
    }

done:

    return map;
}

static bool __continue_decoding( void )
{
    pb_command_msg_t *cmd;

    if( pdTRUE == xQueueReceive(__commands_queued, &cmd, 0) ) {
        /* If we are told to pause, block until we get a new message.  If the
         * new message is play, continue, otherwise, stop decoding. */
        if( PB_CMD__pause == cmd->cmd ) {
            xQueueSendToBack( __commands_idle, &cmd, 0 );
            xQueueReceive( __commands_queued, &cmd, portMAX_DELAY );
            if( PB_CMD__play == cmd->cmd ) {
                xQueueSendToBack( __commands_idle, &cmd, 0 );
                return true;
            }
        }
        xQueueSendToFront( __commands_queued, &cmd, 0 );

        return false;
    }

    return true;
}

static void __handle_messages( song_node_t **current )
{
    db_status_t rv;
    pb_command_msg_t *cmd;

    _D2( "%s()\n", __func__ );
retry:

    if( pdTRUE == xQueueReceive(__commands_queued, &cmd, 0) ) {
        _D2( "Got a message\n" );
        switch( cmd->cmd ) {
            case PB_CMD__album_next:
                _D2( "PB_CMD__album_next\n" );
                rv = next_song( current, DT_NEXT, DL_ALBUM );
                if( DS_END_OF_LIST == rv ) {
                    next_song( current, DT_NEXT, DL_ARTIST );
                }
                break;

            case PB_CMD__album_prev:
                _D2( "PB_CMD__album_prev\n" );
                rv = next_song( current, DT_PREVIOUS, DL_ALBUM );
                if( DS_END_OF_LIST == rv ) {
                    next_song( current, DT_PREVIOUS, DL_ARTIST );
                }
                break;

            case PB_CMD__song_next:
                _D2( "PB_CMD__song_next\n" );
                rv = next_song( current, DT_NEXT, DL_SONG );
                if( DS_END_OF_LIST == rv ) {
                    rv = next_song( current, DT_NEXT, DL_ALBUM );
                }
                if( DS_END_OF_LIST == rv ) {
                    rv =next_song( current, DT_NEXT, DL_ARTIST );
                }
                break;

            case PB_CMD__song_prev:
                _D2( "PB_CMD__song_prev\n" );
                rv = next_song( current, DT_PREVIOUS, DL_SONG );
                if( DS_END_OF_LIST == rv ) {
                    rv = next_song( current, DT_PREVIOUS, DL_ALBUM );
                }
                if( DS_END_OF_LIST == rv ) {
                    rv =next_song( current, DT_PREVIOUS, DL_ARTIST );
                }
                break;

            case PB_CMD__play:
                _D2( "PB_CMD__play\n" );
                /* Do nothing, just exit so we can play. */
                break;

            case PB_CMD__pause:
                _D2( "PB_CMD__pause\n" );
            case PB_CMD__stop:
                _D2( "PB_CMD__stop\n" );
                /* Stall until a new command. */
                xQueueSendToBack( __commands_idle, &cmd, 0 );
                xQueuePeek( __commands_queued, &cmd, portMAX_DELAY );
                goto retry;

            case PB_CMD__change_disc:
                _D2( "PB_CMD__change_disc\n" );
                *current = NULL;
                rv = DS_SUCCESS;
                while( DS_SUCCESS == rv ) {
                    rv = next_song( current, DT_NEXT, DL_GROUP );
                    if( DS_FAILURE != rv ) {
                        if( 0 == strcasecmp(dir_map[cmd->disc - 1],
                                            (*current)->album->artist->group->name) )
                        {
                            rv = DS_END_OF_LIST;
                        }
                    }
                }
                break;
        }

        xQueueSendToBack( __commands_idle, &cmd, 0 );
    }
}
