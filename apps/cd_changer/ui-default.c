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
#include <stddef.h>
#include <stdint.h>

#include <freertos/task.h>

#include "user-interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define DIR_MAP_SIZE        6
#define UI_DEFAULT_DEBUG    0

#define _D1(...)
#define _D2(...)
#define _D3(...)

#if (defined(UI_DEFAULT_DEBUG) && (0 < UI_DEFAULT_DEBUG))
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif

#if (defined(UI_DEFAULT_DEBUG) && (1 < UI_DEFAULT_DEBUG))
#undef  _D2
#define _D2(...) printf( __VA_ARGS__ )
#endif

#if (defined(UI_DEFAULT_DEBUG) && (2 < UI_DEFAULT_DEBUG))
#undef  _D3
#define _D3(...) printf( __VA_ARGS__ )
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static const char** __dir_map_get( size_t *size );
static void __get_disc_info( uint8_t *map, uint8_t *disc, uint8_t *track,
                             song_node_t **song, void *user_data );
static void __process_command( irp_state_t *device_status,
                               const uint8_t disc_map,
                               uint8_t *current_disc,
                               uint8_t *current_track,
                               const ri_msg_t *msg,
                               song_node_t **song,
                               void *user_data );
static uint8_t __map_get( void );
static uint8_t __find_lowest_disc( const uint8_t map );
static void __find_song( song_node_t **song, irp_cmd_t cmd, const uint8_t disc );

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static const char *__dir_map[DIR_MAP_SIZE] = { "1", "2", "3", "4", "5", "6" };

static const ui_impl_t __impl = {
    .name = "default",
    .ui_user_data_init_fn = NULL,
    .ui_user_data_destroy_fn = NULL,
    .ui_dir_map_get_fn = &__dir_map_get,
    .ui_dir_map_release_fn = NULL,
    .ui_get_disc_info_fn = &__get_disc_info,
    .ui_process_command_fn = &__process_command,
    .impl_free = NULL
};

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See ui-default.h for details. */
bool uid_init( void )
{
    return ui_register( &__impl );
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* See user-interface.h for details. */
static const char** __dir_map_get( size_t *size )
{
    *size = DIR_MAP_SIZE;

    return __dir_map;
}

/* See user-interface.h for details. */
static void __get_disc_info( uint8_t *map, uint8_t *disc, uint8_t *track,
                             song_node_t **song, void *user_data )
{
    *map = __map_get();
    *disc = __find_lowest_disc( *map );
    *track = 1;
    *song = NULL;
}

/* See user-interface.h for details. */
static void __process_command( irp_state_t *device_status,
                               const uint8_t disc_map,
                               uint8_t *current_disc,
                               uint8_t *current_track,
                               const ri_msg_t *msg,
                               song_node_t **song,
                               void *user_data )
{
    _D2( "device_status: 0x%04x\n", *device_status );

    if( RI_MSG_TYPE__IBUS_CMD == msg->type ) {
        switch( msg->d.ibus.command ) {
            case IRP_CMD__SCAN_DISC__ENABLE:
            case IRP_CMD__SCAN_DISC__DISABLE:
            case IRP_CMD__RANDOMIZE__ENABLE:
            case IRP_CMD__RANDOMIZE__DISABLE:
                _D2( "MISC Ignored: 0x%04x\n", msg->d.ibus.command );
                break;

            case IRP_CMD__STOP:
                _D2( "IRP_CMD__STOP\n" );
                if( (IRP_STATE__PLAYING == *device_status) ||
                    (IRP_STATE__PAUSED == *device_status) )
                {
                    ri_playback_command( PB_CMD__STOP );
                }
                break;

            case IRP_CMD__PAUSE:
                _D2( "IRP_CMD__PAUSE\n" );
                if( IRP_STATE__PLAYING == *device_status ) {
                    ri_playback_command( PB_CMD__PAUSE );
                }
                break;

            case IRP_CMD__PLAY:
                _D2( "IRP_CMD__PLAY\n" );
                if( IRP_STATE__STOPPED == *device_status ) {
                    if( NULL == *song ) {
                        __find_song( song, IRP_CMD__CHANGE_DISC, *current_disc );
                    }
                    ri_playback_play( *song );
                } else if( IRP_STATE__PAUSED == *device_status ) {
                    ri_playback_command( PB_CMD__RESUME );
                }
                break;

            case IRP_CMD__FAST_PLAY__FORWARD:
                _D2( "IRP_CMD__FAST_PLAY__FORWARD\n" );

                if( IRP_STATE__FAST_PLAYING__FORWARD != *device_status ) {
                    *device_status = IRP_STATE__FAST_PLAYING__FORWARD;
                    __find_song( song, msg->d.ibus.command, 0 );
                    ri_playback_play( *song );
                }
                break;

            case IRP_CMD__FAST_PLAY__REVERSE:
                _D2( "IRP_CMD__FAST_PLAY__REVERSE\n" );
                if( IRP_STATE__FAST_PLAYING__REVERSE != *device_status ) {
                    *device_status = IRP_STATE__FAST_PLAYING__REVERSE;
                    __find_song( song, msg->d.ibus.command, 0 );
                    ri_playback_play( *song );
                }
                break;

            case IRP_CMD__SEEK__NEXT:
                _D2( "IRP_CMD__SEEK__NEXT\n" );
                *device_status = IRP_STATE__SEEKING__NEXT;
                ri_send_state( *device_status, disc_map, *current_disc, *current_track );
                __find_song( song, msg->d.ibus.command, 0 );
                ri_playback_play( *song );
                break;

            case IRP_CMD__SEEK__PREV:
                _D2( "IRP_CMD__SEEK__PREV\n" );
                *device_status = IRP_STATE__SEEKING__PREV;
                ri_send_state( *device_status, disc_map, *current_disc, *current_track );
                __find_song( song, msg->d.ibus.command, 0 );
                ri_playback_play( *song );
                break;

            case IRP_CMD__CHANGE_DISC:
                _D2( "IRP_CMD__CHANGE_DISC\n" );
                *device_status = IRP_STATE__LOADING_DISC;
                ri_send_state( *device_status, disc_map, *current_disc, *current_track );
                __find_song( song, msg->d.ibus.command, msg->d.ibus.disc );
                *current_disc = msg->d.ibus.disc;
                ri_playback_play( *song );
                break;

            case IRP_CMD__GET_STATUS:   /* Never sent. */
            case IRP_CMD__POLL:         /* Never sent. */
                break;
        }
    } else {
        _D2( "RI_MSG_TYPE__PLAYBACK_STATUS\n" );
        switch( msg->d.song.status ) {
            case PB_STATUS__PLAYING:
                _D2( "PB_STATUS__PLAYING\n" );
                *current_track = (*song)->track_number;
                *device_status = IRP_STATE__PLAYING;
                break;
            case PB_STATUS__PAUSED:
                _D2( "PB_STATUS__PAUSED\n" );
                *device_status = IRP_STATE__PAUSED;
                break;
            case PB_STATUS__STOPPED:
                _D2( "PB_STATUS__STOPPED\n" );
                *device_status = IRP_STATE__STOPPED;
                break;

            case PB_STATUS__ERROR:
                _D2( "PB_STATUS__ERROR\n" );
                /* Without this we seem to deadlock on "full error" testing. */
                /* The real error seems to be due to the physical ibus driver. */
                vTaskDelay( 100 );

            case PB_STATUS__END_OF_SONG:
                _D2( "PB_STATUS__END_OF_SONG\n" );
                *device_status = IRP_STATE__SEEKING__NEXT;
                ri_send_state( *device_status, disc_map, *current_disc, *current_track );
                __find_song( song, IRP_CMD__SEEK__NEXT, 0 );
                ri_playback_play( *song );
                break;
        }
        ri_send_state( *device_status, disc_map, *current_disc, *current_track );
    }
}

/**
 *  Used to walk the available database and determine the output
 *  bitmask.
 *
 *  @return the bitmask of the discs available according to the database
 */
static uint8_t __map_get( void )
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
                if( 0 == strcmp(__dir_map[i], song->album->artist->group->name) ) {
                    map |= 1 << i;
                    break;
                }
            }
        }
    }

done:

    return map;
}

/**
 *  Helper function that finds the lowest numbered disc.
 *
 *  @param map the map to analize
 *
 *  @return the lowest disc number, or 7 on error.
 */
static uint8_t __find_lowest_disc( const uint8_t map )
{
    int32_t i;
    int32_t lowest_disc;

    lowest_disc = 7;
    for( i = 1; i <= 6; i++ ) {
        bool disc_present;
        uint8_t active_map;

        disc_present = ((1 << (i - 1)) == (map & ((1 << (i - 1)))));
        active_map = (map & (0x3f >> (6 - i)));

        if( (true == disc_present) && (i < lowest_disc) ) {
            lowest_disc = i;
        }
    }

    return lowest_disc;
}

/**
 *  Helper function used to find the next song to play.
 *
 *  @param song the current song to base the next song from
 *  @param cmd the command to apply
 *  @param disc the new disc to play if the command needs the information
 */
static void __find_song( song_node_t **song, irp_cmd_t cmd, const uint8_t disc )
{
    db_status_t rv;
    
    switch( cmd ) {
        case IRP_CMD__FAST_PLAY__FORWARD:
            rv = next_song( song, DT_NEXT, DL_ALBUM );
            if( DS_END_OF_LIST == rv ) {
                next_song( song, DT_NEXT, DL_ARTIST );
            }
            break;

        case IRP_CMD__FAST_PLAY__REVERSE:
            rv = next_song( song, DT_PREVIOUS, DL_ALBUM );
            if( DS_END_OF_LIST == rv ) {
                next_song( song, DT_PREVIOUS, DL_ARTIST );
            }
            break;

        case IRP_CMD__SEEK__NEXT:
            rv = next_song( song, DT_NEXT, DL_SONG );
            if( DS_END_OF_LIST == rv ) {
                rv = next_song( song, DT_NEXT, DL_ALBUM );
            }
            if( DS_END_OF_LIST == rv ) {
                next_song( song, DT_NEXT, DL_ARTIST );
            }
            break;

        case IRP_CMD__SEEK__PREV:
            rv = next_song( song, DT_PREVIOUS, DL_SONG );
            if( DS_END_OF_LIST == rv ) {
                rv = next_song( song, DT_PREVIOUS, DL_ALBUM );
            }
            if( DS_END_OF_LIST == rv ) {
                next_song( song, DT_PREVIOUS, DL_ARTIST );
            }
            break;

        case IRP_CMD__CHANGE_DISC:
            *song = NULL;
            rv = DS_SUCCESS;
            while( DS_SUCCESS == rv ) {
                rv = next_song( song, DT_NEXT, DL_GROUP );
                if( DS_FAILURE != rv ) {
                    if( 0 == strcasecmp(__dir_map[disc - 1],
                                        (*song)->album->artist->group->name) )
                    {
                        rv = DS_END_OF_LIST;
                    }
                }
            }
            break;

        default:
            break;
    }
}
