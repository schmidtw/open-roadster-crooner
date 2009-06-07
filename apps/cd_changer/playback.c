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
#include <freertos/semphr.h>
#include <memcard/memcard.h>
#include <database/database.h>

#include "radio-interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define PB_TASK_STACK_SIZE  (configMINIMAL_STACK_SIZE + 8000)
#define PB_TASK_PRIORITY    (tskIDLE_PRIORITY+1)
#define DIR_MAP_SIZE        6

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __pb_task( void *params );
static void __codec_suspend( void );
static void __codec_resume( void );
static void __mount_status( const mc_card_status_t status );
static uint8_t __determine_map( void );

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static volatile xSemaphoreHandle __codec_semaphore;
static volatile xSemaphoreHandle __card_mounted;
static volatile mc_card_status_t __card_status;
static const char *dir_map[DIR_MAP_SIZE] = { "1", "2", "3", "4", "5", "6" };
static volatile song_node_t *__current_song;
static volatile media_command_t __playback_mode;

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void playback_init( void )
{
    printf( "playback_init()\n" );

    __current_song = NULL;
    __playback_mode = MI_STOP;

    vSemaphoreCreateBinary( __codec_semaphore );
    vSemaphoreCreateBinary( __card_mounted );
    xSemaphoreTake( __card_mounted, 0 );

    xTaskCreate( __pb_task, ( signed portCHAR *) "PLYB",
                 PB_TASK_STACK_SIZE, NULL, PB_TASK_PRIORITY, NULL );
}

void playback_song_next( void )
{
    db_status_t rv;

    if( NULL != __current_song ) {
        __current_song->command_fn( MI_STOP );
    }

    rv = next_song( &__current_song, DT_NEXT, DL_SONG );
    if( DS_END_OF_LIST == rv ) {
        rv = next_song( &__current_song, DT_NEXT, DL_ALBUM );
    }
    if( DS_END_OF_LIST == rv ) {
        rv = next_song( &__current_song, DT_NEXT, DL_ARTIST );
    }

    __playback_mode = MI_PLAY;
}

void playback_song_prev( void )
{
    db_status_t rv;

    if( NULL != __current_song ) {
        __current_song->command_fn( MI_STOP );
    }

    rv = next_song( &__current_song, DT_PREVIOUS, DL_SONG );
    if( DS_END_OF_LIST == rv ) {
        rv = next_song( &__current_song, DT_PREVIOUS, DL_ALBUM );
    }
    if( DS_END_OF_LIST == rv ) {
        rv = next_song( &__current_song, DT_PREVIOUS, DL_ARTIST );
    }

    __playback_mode = MI_PLAY;
}

void playback_disc( const uint8_t disc )
{
    db_status_t rv;

    if( NULL != __current_song ) {
        __current_song->command_fn( MI_STOP );
    }

    __current_song = NULL;
    rv = DS_SUCCESS;

    while( DS_SUCCESS == rv ) {
        rv = next_song( &__current_song, DT_NEXT, DL_GROUP );
        if( DS_FAILURE != rv ) {
            if( 0 == strcasecmp(dir_map[disc - 1],
                                __current_song->album->artist->group->name) )
            {
                rv = DS_END_OF_LIST;
            }
        }
    }
}

void playback_stop( void )
{
    if( NULL != __current_song ) {
        __current_song->command_fn( MI_STOP );
    }
    __playback_mode = MI_STOP;
}

void playback_play( void )
{
    if( NULL != __current_song ) {
        __current_song->command_fn( MI_PLAY );
    }

    __playback_mode = MI_PLAY;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __pb_task( void *params )
{
    mc_register( &__mount_status );

    while( 1 ) {
        FATFS fs;

        xSemaphoreTake( __card_mounted, portMAX_DELAY );

        if( 0 == f_mount(0, &fs) ) {

            printf( "mounted fs.\n" );
            ri_checking_for_discs();
            if( true == populate_database(dir_map, DIR_MAP_SIZE, "/") ) {
                media_status_t ms;

                ri_checking_complete( __determine_map() );

                __current_song = NULL;
                ms = MI_STOPPED_BY_REQUEST;
                next_song( &__current_song, DT_NEXT, DL_SONG );
                while( MC_CARD__MOUNTED == __card_status ) {
                    media_play_fn_t play_fn;

                    while( MI_PLAY != __playback_mode ) {
                        vTaskDelay( TASK_DELAY_MS(10) );
                    }

                    play_fn = __current_song->play_fn;
                    ms = (*play_fn)( __current_song->file_location,
                                  __codec_suspend, __codec_resume );

                    if( MI_STOPPED_BY_REQUEST != ms ) {
                        db_status_t rv;

                        ri_song_ended_playing_next();

                        rv = next_song( &__current_song, DT_NEXT, DL_SONG );
                        if( DS_END_OF_LIST == rv ) {
                            rv = next_song( &__current_song, DT_NEXT, DL_ALBUM );
                        }
                        if( DS_END_OF_LIST == rv ) {
                            rv = next_song( &__current_song, DT_NEXT, DL_ARTIST );
                        }
                        if( DS_FAILURE == rv ) {
                            break;
                        }
                    }
                }

                __current_song = NULL;
                database_purge();
            } else {
                ri_checking_complete( 0x00 );
            }
        }
    }
}

/**
 *  Block the codec decoding task using a semaphore instead
 *  of spin-locking.
 */
static void __codec_suspend( void )
{
    xSemaphoreTake( __codec_semaphore, portMAX_DELAY );
}

/**
 *  Resume the codec decoding task from inside an ISR.
 */
static void __codec_resume( void )
{
    portBASE_TYPE xTaskWoken;
    xSemaphoreGiveFromISR( __codec_semaphore, &xTaskWoken );
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
