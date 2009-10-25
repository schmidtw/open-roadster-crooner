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
#include <display/display.h>
#include <database/database.h>

#include "user-interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define DIR_MAP_SIZE  0
#define UI_T_DEBUG    0

#define _D1(...)
#define _D2(...)
#define _D3(...)

#if (defined(UI_T_DEBUG) && (0 < UI_T_DEBUG))
#include <stdio.h>
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif

#if (defined(UI_T_DEBUG) && (1 < UI_T_DEBUG))
#undef  _D2
#define _D2(...) printf( __VA_ARGS__ )
#endif

#if (defined(UI_T_DEBUG) && (2 < UI_T_DEBUG))
#undef  _D3
#define _D3(...) printf( __VA_ARGS__ )
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef enum {
    DM_ARTIST = 1,
    DM_ALBUM = 2,
    DM_SONG = 3,
    DM_RANDOM = 4,
    DM_TEXT_DISPLAY = 5
} disc_mode_t;

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
static bool __get_next_disc_mode_element(disc_mode_t *val);
static uint8_t __map_get( void );
static uint8_t __find_display_number( song_node_t *song, const uint8_t disc );
static bool __find_song( song_node_t **song, irp_cmd_t cmd, const uint8_t disc );
static void __update_song_display_info( song_node_t *song, const uint8_t disc );
static void update_text_display_state( irp_state_t *device_status,
        const uint8_t disc_map, const uint8_t disc, uint8_t *track );
static void set_display_state( bool new_state );
static bool is_display_enabled( void );
static uint8_t get_dispaly_track( bool disp_state );
static uint8_t get_random_track( bool random_state );
static void set_random_state( bool new_state );
static bool is_random_enabled( void );
static void enable_scan_state( void );
static void disable_scan_state( void );
static bool is_scan_enabled( void );
static void start_scan_timer( void );

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static const ui_impl_t __impl = {
    .name = "t",
    .ui_user_data_init_fn = NULL,
    .ui_user_data_destroy_fn = NULL,
    .ui_dir_map_get_fn = &__dir_map_get,
    .ui_dir_map_release_fn = NULL,
    .ui_get_disc_info_fn = &__get_disc_info,
    .ui_process_command_fn = &__process_command,
    .impl_free = NULL
};

static bool display_text_state;
static bool random_state;
static bool scan_state;

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See ui-default.h for details. */
bool ui_t_init( void )
{
    _D2("ui_t_init() - %d\n", __LINE__);
    return ui_register( &__impl );
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* See user-interface.h for details. */
static const char** __dir_map_get( size_t *size )
{
    _D2("__dir_map_get() - %d\n", __LINE__);
    *size = DIR_MAP_SIZE;

    return NULL;
}

/* See user-interface.h for details. */
static void __get_disc_info( uint8_t *map, uint8_t *disc, uint8_t *track,
                             song_node_t **song, void *user_data )
{
    _D2("__get_disc_info() - %d\n", __LINE__);
    
    *map = __map_get();
    *disc = 1;
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
    bool shouldSendText = false;
    _D2( "device_status: 0x%04x\n", *device_status );

    if( RI_MSG_TYPE__IBUS_CMD == msg->type ) {
        _D2( "msg->type == RI_MSG_TYPE__IBUS_CMD\n" );
        switch( msg->d.ibus.command ) {
            case IRP_CMD__SCAN_DISC__ENABLE:
            case IRP_CMD__SCAN_DISC__DISABLE:
            case IRP_CMD__RANDOMIZE__ENABLE:
            case IRP_CMD__RANDOMIZE__DISABLE:
                if( IRP_CMD__SCAN_DISC__ENABLE == msg->d.ibus.command ) {
                    _D2( "IRP_CMD__SCAN_DISC__ENABLE\n" );
                    *device_status = IRP_CMD__RANDOMIZE__ENABLE;
                    enable_scan_state();
                } else if( IRP_CMD__SCAN_DISC__DISABLE == msg->d.ibus.command ) {
                    _D2( "IRP_CMD__SCAN_DISC__DISABLE\n" );
                    *device_status = IRP_CMD__RANDOMIZE__DISABLE;
                    disable_scan_state();
                } else if( IRP_CMD__RANDOMIZE__ENABLE == msg->d.ibus.command ) {
                    _D2( "IRP_CMD__RANDOMIZE__ENABLE\n" );
                    *device_status = IRP_CMD__RANDOMIZE__ENABLE;
                    set_random_state(true);
                } else {
                    _D2( "IRP_CMD__RANDOMIZE__DISABLE\n" );
                    *device_status = IRP_CMD__RANDOMIZE__DISABLE;
                    set_random_state(false);
                }
                ri_send_state( *device_status, disc_map, *current_disc, *current_track );
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
                        if( is_random_enabled() ) {
                            next_song( song, DT_RANDOM, DL_GROUP );
                        } else {
                            /* Not in the random mode */
                            next_song( song, DT_NEXT, DL_SONG );
                        }
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
                    ri_send_state( *device_status, disc_map, *current_disc, *current_track );
                    if( __find_song( song, msg->d.ibus.command, *current_disc ) ) {
                        ri_playback_play( *song );
                    } else {
                        update_text_display_state(device_status, disc_map, *current_disc, current_track);
                    }
                }
                break;

            case IRP_CMD__FAST_PLAY__REVERSE:
                _D2( "IRP_CMD__FAST_PLAY__REVERSE\n" );
                if( IRP_STATE__FAST_PLAYING__REVERSE != *device_status ) {
                    *device_status = IRP_STATE__FAST_PLAYING__REVERSE;
                    ri_send_state( *device_status, disc_map, *current_disc, *current_track );
                    if( __find_song( song, msg->d.ibus.command, *current_disc ) ) {
                        ri_playback_play( *song );
                    } else {
                        update_text_display_state(device_status, disc_map, *current_disc, current_track);
                    }
                }
                break;

            case IRP_CMD__SEEK__NEXT:
            {
                _D2( "IRP_CMD__SEEK__NEXT\n" );
                *device_status = IRP_STATE__SEEKING__NEXT;
                ri_send_state( *device_status, disc_map, *current_disc, *current_track );
                if( __find_song( song, msg->d.ibus.command, *current_disc ) ) {
                    ri_playback_play( *song );
                } else {
                    update_text_display_state(device_status, disc_map, *current_disc, current_track);
                }
                break;
            }

            case IRP_CMD__SEEK__PREV:
                _D2( "IRP_CMD__SEEK__PREV\n" );
                *device_status = IRP_STATE__SEEKING__PREV;
                ri_send_state( *device_status, disc_map, *current_disc, *current_track );
                if( __find_song( song, msg->d.ibus.command, *current_disc ) ) {
                    ri_playback_play( *song );
                } else {
                    update_text_display_state(device_status, disc_map, *current_disc, current_track);
                }
                break;

            case IRP_CMD__CHANGE_DISC:
                _D2( "IRP_CMD__CHANGE_DISC\n" );
                *device_status = IRP_STATE__LOADING_DISC;
                ri_send_state( *device_status, disc_map, *current_disc, *current_track );
                *current_disc = msg->d.ibus.disc;
                *current_track = __find_display_number(*song, *current_disc);
                *device_status = IRP_STATE__PLAYING;
                ri_send_state( *device_status, disc_map, *current_disc, *current_track );
                shouldSendText = true;
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
                *current_track = __find_display_number( *song, *current_disc );
                *device_status = IRP_STATE__PLAYING;
                shouldSendText = true;
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
                if( is_random_enabled() ) {
                    __find_song( song, IRP_CMD__SEEK__NEXT, *current_disc );
                } else {
                    __find_song( song, IRP_CMD__SEEK__NEXT, (uint8_t)DM_SONG );
                }
                ri_playback_play( *song );
                shouldSendText = true;
                break;
//            case PB_STATUS__SCAN_NEXT_SONG:
//                _D2( "PB_STATUS__SCAN_NEXT_SONG\n" );
//                *device_status = IRP_CMD__SEEK__NEXT;
//                ri_send_state( *device_status, disc_map, *current_disc, *current_track);
//                __find_song( song, IRP_CMD__SEEK__NEXT, (uint8_t)DM_SONG );
//                ri_playback_play( *song );
//                shouldSendText = true;
//                break;
        }
        ri_send_state( *device_status, disc_map, *current_disc, *current_track );
    }
    if( true == shouldSendText ) {
        __update_song_display_info(*song, *current_disc);
    }
}

/**
 * Used to get the next element in the disc_mode_t enum
 * 
 * @param val pointer to current enum constant which will be
 *        updated to the next element
 * 
 * @return true if the the *val param was updated properly
 *         false if there are no more constants in the enum
 *         or the passed in value is not part of the enum
 */
static bool __get_next_disc_mode_element(disc_mode_t *val)
{
    switch( *val ) {
        case DM_ARTIST:
            *val = DM_ALBUM;
            break;
        case DM_ALBUM:
            *val = DM_SONG;
            break;
        case DM_SONG:
            *val = DM_RANDOM;
            break;
        case DM_RANDOM:
            *val = DM_TEXT_DISPLAY;
            break;
        case DM_TEXT_DISPLAY:
        default:
            return false;
    }
    return true;
}

/**
 *  Used to walk the available database and determine the output
 *  bitmask.
 *
 *  @return the bitmask of the discs available according to the database
 */
static uint8_t __map_get( void )
{
    song_node_t *song;
    uint8_t map;
    _D2("__map_get() - %d\n", __LINE__);

    map = 0x00;
    song = NULL;
    
    if( DS_SUCCESS == next_song( &song, DT_NEXT, DL_GROUP ) ) {
        /* Generate the map file from the enum disc_mode_t */
        disc_mode_t dm = DM_ARTIST;
        _D2("__map_get - found database\n");
        do {
            map |= 1 << (dm-1);
        } while( __get_next_disc_mode_element(&dm) );
    }
    _D1("__map_get() - returning 0x%02x\n", map );
    return map;
}

/**
 * Based on the current song information and the disc (mode), this function
 * will return the track number which should be associated with this disc and
 * sent to the radio.
 * 
 * @param song pointer to the song node which is currently being used
 * @param disc will be treated as disc_mode_t
 * 
 * @return song number to be sent to radio and ui-t track number to be used
 */
static uint8_t __find_display_number( song_node_t *song, const uint8_t disc )
{
    uint32_t tn = 1;
    uint8_t rv;
    switch( disc ) {
        case DM_SONG:
            if( 0 == song->track_number ) {
                return 1;
            }
            tn = song->track_number;
            break;
        case DM_ALBUM:
        {
            tn = song->album->index_in_list;
            break;
        }
        case DM_ARTIST:
        {
            tn = song->album->artist->index_in_list;
            break;
        }
        case DM_TEXT_DISPLAY:
            return get_dispaly_track( is_display_enabled() );
            
        case DM_RANDOM:
            return get_random_track( is_random_enabled() );
        default:
            break;
    }
    rv = (uint8_t)(tn % 100);
    if( rv == 0 ) { rv = 1; }
    return rv;
}

/**
 *  Helper function used to find the next song to play.
 *
 *  @param song the current song to base the next song from
 *  @param cmd the command to apply
 *  @param disc the new disc to play if the command needs the information
 *  
 *  @return true if a new song has been selected and should be played
 *          false if the currently playing song should continue to play
 */
static bool __find_song( song_node_t **song, irp_cmd_t cmd, const uint8_t disc )
{
    db_status_t rv;
    bool isNewSong = true;
    db_traverse_t direction;
    
    switch( cmd ) {
        case IRP_CMD__FAST_PLAY__FORWARD:
        case IRP_CMD__SEEK__NEXT:
            direction = DT_NEXT;
            break;
        case IRP_CMD__FAST_PLAY__REVERSE:
        case IRP_CMD__SEEK__PREV:
            direction = DT_PREVIOUS;
            break;
        default:
            return false;
    }
    
    if( is_random_enabled() ) {
        direction = DT_RANDOM;
    }
    
    switch( disc ) {
        case DM_SONG:
            rv = next_song( song, direction, DL_SONG );
            if( DS_END_OF_LIST != rv ) {
                break;
            }
        case DM_ALBUM:
            rv = next_song( song, direction, DL_ALBUM );
            if( DS_END_OF_LIST != rv ) {
                break;
            }
        case DM_ARTIST:
            next_song( song, direction, DL_ARTIST );
            break;
        case DM_TEXT_DISPLAY:
            set_display_state( !is_display_enabled());
            isNewSong = false;
            break;
        case DM_RANDOM:
            set_random_state( !is_random_enabled() );
            isNewSong = false;
            break;
        default:
            break;
    }
    return isNewSong;
}

/**
 * Helper function which should be how we send enable text for this song to
 * be sent to the display library.
 * 
 * @param song pointer to the currently playing song
 * @param disc will be treated as disc_mode_t
 */
static void __update_song_display_info( song_node_t *song, const uint8_t disc )
{
    if( false == is_display_enabled() ) {
        return;
    }
    switch( disc ) {
        case DM_SONG:
            display_start_text(song->title);
            break;
        case DM_ALBUM:
            display_start_text( (char*) &song->album->name);
            break;
        case DM_ARTIST:
            display_start_text((char *) &song->album->artist->name);
            break;
        case DM_TEXT_DISPLAY:
            display_start_text( "Text Enabled\0" );
            break;
        default:
            break;
    }
}

static void update_text_display_state( irp_state_t *device_status,
        const uint8_t disc_map, const uint8_t disc, uint8_t *track )
{
    if( DM_TEXT_DISPLAY == disc )
    {
        *device_status = IRP_STATE__STOPPED;
        ri_send_state( *device_status, disc_map, disc, *track );
        
        *track = get_dispaly_track( is_display_enabled() );
        *device_status = IRP_STATE__PLAYING;
        ri_send_state( *device_status, disc_map, disc, *track );
    } else if( DM_RANDOM == disc ) {
        *device_status = IRP_STATE__STOPPED;
        ri_send_state( *device_status, disc_map, disc, *track );
        *track = get_random_track( is_random_enabled() );
        *device_status = IRP_STATE__PLAYING;
        ri_send_state( *device_status, disc_map, disc, *track );
    }
}

/**
 * Helper function which enables or disables the use of the display
 * to the radio.
 * 
 * @param new_state true - enable displaying information on the radio
 *             false - disable displaying information on the radio
 */
static void set_display_state( bool new_state )
{
    display_text_state = new_state;
    if( false == new_state ) {
        display_stop_text();
    }
}

/**
 * Helper function which queries the state of the display
 * 
 * @return true the display is enabled, false otherwise
 */
static bool is_display_enabled( void )
{
    return display_text_state;
}


#define DISPLAY_TRACK_NOT_ENABLED 1
#define DISPLAY_TRACK_ENABLED     2
/**
 * Helper function which should be called to get the track
 * number for text display on/off
 * 
 * @param disp_state the state of the text display
 * 
 * @return track number which is associated with the disp_state
 *         parameter
 */
static uint8_t get_dispaly_track( bool disp_state )
{
    if( true == disp_state ) {
        return DISPLAY_TRACK_ENABLED;
    }
    return DISPLAY_TRACK_NOT_ENABLED;
}


#define RANDOM_TRACK_NOT_ENABLED 1
#define RANDOM_TRACK_ENABLED     2
/**
 * Helper function which should be called to get the track
 * number for the random on/off
 * 
 * @param random_state the state of the random
 * 
 * @return track number which is associated with the
 *  random_state parameter
 */
static uint8_t get_random_track( bool random_state )
{
   if( true == random_state ) {
       return RANDOM_TRACK_ENABLED;
   }
   return RANDOM_TRACK_NOT_ENABLED;
}

/**
 * Helper function which enables or disables the random
 * state
 * 
 * @param new_state true - enable the random next song
 *        false - disable the random next song
 */
static void set_random_state( bool new_state )
{
    random_state = new_state;
}

/**
 * Helper function which queries the state of the random
 * 
 * @return true if the random state is enabled, false otherwise
 */
static bool is_random_enabled( void )
{
    return random_state;
}

/**
 * Helper function which enables the scan state
 */
static void enable_scan_state( void )
{
    scan_state = true;
}

/**
 * Helper function which disables the scan state
 */
static void disable_scan_state( void )
{
    scan_state = false;
}

/**
 * Helper function which queries the state of the scan
 * 
 * @return true if the scan state is enabled, false otherwise
 */
static bool is_scan_enabled( void )
{
    return scan_state;
}

/**
 * Helper function which creates a message for handling the time
 * when to scan to the next song.
 */
static void start_scan_timer( void )
{
    return;
}
