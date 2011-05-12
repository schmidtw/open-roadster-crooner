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
#include <stdlib.h>

#include <bsp/boards/boards.h>
#include <bsp/cpu.h>
#include <freertos/os.h>
#ifdef SUPPORT_TEXT
#include <display/display.h>
#endif
#include <database/database.h>

#include "user-interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define DIR_MAP_SIZE    0
#define UI_HISTORY      4

#define UI_T_DEBUG      0

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

typedef struct {
    size_t valid;
    ri_msg_t msg[UI_HISTORY];
} history_t;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void* __get_user_data();
static void __get_disc_info( uint8_t *map, uint8_t *disc, uint8_t *track,
                             song_node_t **song, void *user_data );
static void __process_command( irp_state_t *device_status,
                               irp_mode_t *device_mode,
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
                                       const irp_mode_t device_mode,
                                       const uint8_t disc_map,
                                       const uint8_t disc,
                                       uint8_t *track );
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
static void history_push( history_t *history, const ri_msg_t *msg );
static bool history_get_last_cmd( history_t *history, irp_cmd_t *cmd );

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static const ui_impl_t __impl = {
    .name = "t",
    .ui_user_data_init_fn = __get_user_data,
    .ui_user_data_destroy_fn = NULL,
    .ui_dir_map_release_fn = NULL,
    .ui_get_disc_info_fn = &__get_disc_info,
    .ui_process_command_fn = &__process_command,
    .impl_free = NULL
};

static bool display_text_state = true;
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
static void* __get_user_data()
{
    static history_t h;

    h.valid = 0;
    return &h;
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
                               irp_mode_t *device_mode,
                               const uint8_t disc_map,
                               uint8_t *current_disc,
                               uint8_t *current_track,
                               const ri_msg_t *msg,
                               song_node_t **song,
                               void *user_data )
{
    irp_cmd_t last;
    bool shouldSendText = false;
    bool send_status = true;
    history_t *history = (history_t *) user_data;

    _D2( "device_status: 0x%04x -- %s\n", *device_status, irp_state_to_string(*device_status) );

    /* Just make it not match the search below... */
    last = IRP_CMD__GET_STATUS;
    history_get_last_cmd( history, &last );

    _D2( "Last cmd: %s\n", irp_cmd_to_string(last) );

    if( RI_MSG_TYPE__IBUS_CMD == msg->type ) {

        _D2( "RI_MSG_TYPE__IBUS_CMD:%s\n", irp_cmd_to_string(msg->d.ibus.command) );

        switch( msg->d.ibus.command ) {
            case IRP_CMD__SCAN_DISC__ENABLE:
                *device_mode = IRP_MODE__SCANNING;
                enable_scan_state();
                break;
            case IRP_CMD__SCAN_DISC__DISABLE:
                if( IRP_MODE__SCANNING == *device_mode ) {
                    *device_mode = IRP_MODE__NORMAL;
                }
                disable_scan_state();
                break;
            case IRP_CMD__RANDOMIZE__ENABLE:
                *device_mode = IRP_MODE__RANDOM;
                set_random_state( true );
                break;
            case IRP_CMD__RANDOMIZE__DISABLE:
                if( IRP_MODE__RANDOM == *device_mode ) {
                    *device_mode = IRP_MODE__NORMAL;
                }
                set_random_state( false );
                break;

            case IRP_CMD__STOP:
                if( NULL == *song ) {
                    *device_status = IRP_STATE__STOPPED;
                } else {
                    if( (IRP_CMD__STOP != last) &&
                        (IRP_CMD__PAUSE != last) &&
                        (IRP_STATE__STOPPED != *device_status) )
                    {
                        ri_playback_command( PB_CMD__PAUSE );
                    }
                    send_status = false;
                }
                break;

            case IRP_CMD__PAUSE:
                if( NULL == *song ) {
                    *device_status = IRP_STATE__PAUSED;
                } else {
                    send_status = false;

                    if( (IRP_CMD__STOP != last) &&
                        (IRP_CMD__PAUSE != last) )
                    {
                        ri_playback_command( PB_CMD__PAUSE );
                    }
                    send_status = false;
                }
                break;

            case IRP_CMD__PLAY:
                if( NULL == *song ) {
                    if( is_random_enabled() ) {
                        next_song( song, DT_RANDOM, DL_ARTIST );
                    } else {
                        /* Not in the random mode */
                        next_song( song, DT_NEXT, DL_SONG );
                    }
                    ri_playback_play( *song );
                } else if( (IRP_CMD__SEEK__PREV == last) ||
                           (IRP_CMD__SEEK__NEXT == last) )
                {
                    ri_playback_play( *song );
                } else {
                    ri_playback_command( PB_CMD__RESUME );
                }
                send_status = false;
                break;

            case IRP_CMD__FAST_PLAY__FORWARD:
                if( IRP_STATE__FAST_PLAYING__FORWARD != *device_status ) {
                    *device_status = IRP_STATE__FAST_PLAYING__FORWARD;
                    if( __find_song( song, msg->d.ibus.command, *current_disc ) ) {
                        ri_playback_play( *song );
                    } else {
                        update_text_display_state(device_status, *device_mode, disc_map, *current_disc, current_track);
                    }
                }
                break;

            case IRP_CMD__FAST_PLAY__REVERSE:
                if( IRP_STATE__FAST_PLAYING__REVERSE != *device_status ) {
                    *device_status = IRP_STATE__FAST_PLAYING__REVERSE;
                    if( __find_song(song, msg->d.ibus.command, *current_disc) ) {
                        ri_playback_play( *song );
                    } else {
                        update_text_display_state(device_status, *device_mode, disc_map, *current_disc, current_track);
                    }
                }
                break;

            case IRP_CMD__SEEK__NEXT:
                ri_send_state( IRP_STATE__SEEKING__NEXT, *device_mode, disc_map, *current_disc, *current_track );
                *device_status = IRP_STATE__SEEKING;
                if( !__find_song(song, msg->d.ibus.command, *current_disc) ) {
                    update_text_display_state(device_status, *device_mode, disc_map, *current_disc, current_track);
                }
                send_status = false;
                break;

            case IRP_CMD__SEEK__ALT_NEXT:
                ri_send_state( IRP_STATE__SEEKING__NEXT, *device_mode, disc_map, *current_disc, *current_track );
                *device_status = IRP_STATE__SEEKING;
                if( !__find_song(song, msg->d.ibus.command, *current_disc) ) {
                    update_text_display_state(device_status, *device_mode, disc_map, *current_disc, current_track);
                }
                ri_playback_play( *song );
                send_status = false;
                break;

            case IRP_CMD__SEEK__PREV:
                ri_send_state( IRP_STATE__SEEKING__PREV, *device_mode, disc_map, *current_disc, *current_track );
                *device_status = IRP_STATE__SEEKING;
                if( !__find_song(song, msg->d.ibus.command, *current_disc) ) {
                    update_text_display_state(device_status, *device_mode, disc_map, *current_disc, current_track);
                }
                send_status = false;
                break;

            case IRP_CMD__SEEK__ALT_PREV:
                ri_send_state( IRP_STATE__SEEKING__PREV, *device_mode, disc_map, *current_disc, *current_track );
                *device_status = IRP_STATE__SEEKING;
                if( !__find_song(song, msg->d.ibus.command, *current_disc) ) {
                    update_text_display_state(device_status, *device_mode, disc_map, *current_disc, current_track);
                }
                ri_playback_play( *song );
                send_status = false;
                break;

            case IRP_CMD__CHANGE_DISC:
                *device_status = IRP_STATE__LOADING_DISC;
                ri_send_state( *device_status, *device_mode, disc_map, *current_disc, *current_track );
                *current_disc = msg->d.ibus.disc;
                *current_track = __find_display_number(*song, *current_disc);
                *device_status = IRP_STATE__PLAYING;
                shouldSendText = true;
                break;

            case IRP_CMD__GET_STATUS:   /* Never sent. */
            case IRP_CMD__POLL:         /* Never sent. */
            case IRP_CMD__TRAFFIC:      /* Never sent. */
                break;
        }

        if( true == send_status ) {
            ri_send_state( *device_status, *device_mode, disc_map, *current_disc, *current_track );
        }
    } else {
        switch( msg->d.song.status ) {
            case PB_STATUS__PLAYING:
                _D2( "RI_MSG_TYPE__PLAYBACK_STATUS:PB_STATUS__PLAYING\n" );

                *current_track = __find_display_number( *song, *current_disc );
                *device_status = IRP_STATE__PLAYING;
                shouldSendText = true;
                break;

            case PB_STATUS__PAUSED:
                _D2( "RI_MSG_TYPE__PLAYBACK_STATUS:PB_STATUS__PAUSED\n" );
                if( IRP_CMD__PAUSE == last ) {
                    *device_status = IRP_STATE__PAUSED;
                } else {
                    *device_status = IRP_STATE__STOPPED;
                }
                break;

            case PB_STATUS__STOPPED:
                _D2( "RI_MSG_TYPE__PLAYBACK_STATUS:PB_STATUS__STOPPED\n" );
                if( IRP_CMD__PAUSE == last ) {
                    *device_status = IRP_STATE__PAUSED;
                } else {
                    *device_status = IRP_STATE__STOPPED;
                }
                send_status = false;
                break;

            case PB_STATUS__ERROR:
                _D2( "RI_MSG_TYPE__PLAYBACK_STATUS:PB_STATUS__ERROR\n" );
                /* Without this we seem to deadlock on "full error" testing. */
                /* The real error seems to be due to the physical ibus driver. */
                os_task_delay_ticks( 100 );

            case PB_STATUS__END_OF_SONG:
                _D2( "RI_MSG_TYPE__PLAYBACK_STATUS:PB_STATUS__END_OF_SONG\n" );
                if( (IRP_STATE__SEEKING__NEXT == *device_status) ||
                    (IRP_STATE__SEEKING__PREV == *device_status) )
                {
                    send_status = false;
                } else {
                    *device_status = IRP_STATE__SEEKING__NEXT;
                }

                if( is_random_enabled() ) {
                    __find_song( song, IRP_CMD__SEEK__NEXT, *current_disc );
                } else {
                    __find_song( song, IRP_CMD__SEEK__NEXT, (uint8_t)DM_SONG );
                }
                ri_playback_play( *song );
                shouldSendText = true;
                break;
//            case PB_STATUS__SCAN_NEXT_SONG:
//                _D2( "RI_MSG_TYPE__PLAYBACK_STATUS:PB_STATUS__SCAN_NEXT_SONG\n" );
//                *device_status = IRP_CMD__SEEK__NEXT;
//                ri_send_state( *device_status, *device_mode, disc_map, *current_disc, *current_track);
//                __find_song( song, IRP_CMD__SEEK__NEXT, (uint8_t)DM_SONG );
//                ri_playback_play( *song );
//                shouldSendText = true;
//                break;
        }
        if( true == send_status ) {
            ri_send_state( *device_status, *device_mode, disc_map, *current_disc, *current_track );
        }
    }
    if( true == shouldSendText ) {
        __update_song_display_info(*song, *current_disc);
    }

    history_push( history, msg );
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
#ifdef SUPPORT_TEXT
            *val = DM_TEXT_DISPLAY;
            break;
#endif
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
    
    if( DS_SUCCESS == next_song( &song, DT_NEXT, DL_ARTIST) ) {
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
            tn = song->d.parent->d.list.index;
            break;
        }
        case DM_ARTIST:
        {
            tn = song->d.parent->parent->d.list.index;
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
    uint8_t disc_temp = disc;
    
    switch( cmd ) {
        case IRP_CMD__FAST_PLAY__FORWARD:
            disc_temp = DM_SONG;
            direction = DT_NEXT;
            break;
        case IRP_CMD__SEEK__NEXT:
        case IRP_CMD__SEEK__ALT_NEXT:
            direction = DT_NEXT;
            if( is_random_enabled() ) {
                direction = DT_RANDOM;
            }
            break;
        case IRP_CMD__FAST_PLAY__REVERSE:
            disc_temp = DM_SONG;
            direction = DT_PREVIOUS;
            break;
        case IRP_CMD__SEEK__PREV:
        case IRP_CMD__SEEK__ALT_PREV:
            direction = DT_PREVIOUS;
            if( is_random_enabled() ) {
                direction = DT_RANDOM;
            }
            break;
        default:
            return false;
    }
    
    switch( disc_temp ) {
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
#ifdef SUPPORT_TEXT
    switch( disc ) {
        case DM_SONG:
            display_start_text(song->d.name.song);
            break;
        case DM_ALBUM:
            display_start_text(song->d.parent->name.album);
            break;
        case DM_ARTIST:
            display_start_text(song->d.parent->parent->name.artist);
            break;
        case DM_TEXT_DISPLAY:
            display_start_text( "Text Enabled\0" );
            break;
        default:
            break;
    }
#endif
}

static void update_text_display_state( irp_state_t *device_status,
                                       const irp_mode_t device_mode,
                                       const uint8_t disc_map,
                                       const uint8_t disc,
                                       uint8_t *track )
{
    if( DM_TEXT_DISPLAY == disc )
    {
        *device_status = IRP_STATE__STOPPED;
        ri_send_state( *device_status, device_mode, disc_map, disc, *track );
        
        *track = get_dispaly_track( is_display_enabled() );
        *device_status = IRP_STATE__PLAYING;
        ri_send_state( *device_status, device_mode, disc_map, disc, *track );
    } else if( DM_RANDOM == disc ) {
        *device_status = IRP_STATE__STOPPED;
        ri_send_state( *device_status, device_mode, disc_map, disc, *track );
        *track = get_random_track( is_random_enabled() );
        *device_status = IRP_STATE__PLAYING;
        ri_send_state( *device_status, device_mode, disc_map, disc, *track );
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
#ifdef SUPPORT_TEXT
    display_text_state = new_state;
    if( false == new_state ) {
        display_stop_text();
    }
#endif
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
 * state.  When we enable random, we reseed the random
 * number generator with the current tick count since
 * boot.
 * 
 * @param new_state true - enable the random next song
 *        false - disable the random next song
 */
static void set_random_state( bool new_state )
{
    random_state = new_state;
    if( true == random_state ) {
        srand( cpu_get_sys_count() );
    }
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

static void history_push( history_t *history, const ri_msg_t *msg )
{
    if( (NULL != history) && (NULL != msg) ) {
        int i;

        for( i = UI_HISTORY - 1; 0 < i; i-- ) {
            memcpy( &history->msg[i], &history->msg[i - 1], sizeof(ri_msg_t) );
        }
        memcpy( &history->msg[0], msg, sizeof(ri_msg_t) );

        history->valid++;
        if( UI_HISTORY <= history->valid ) {
            history->valid = UI_HISTORY;
        }
    }
}

static bool history_get_last_cmd( history_t *history, irp_cmd_t *cmd )
{
    if( NULL != history ) {
        int i;

        for( i = 0; i < history->valid; i++ ) {
            if( RI_MSG_TYPE__IBUS_CMD == history->msg[i].type ) {
                if( NULL != cmd ) {
                    *cmd = history->msg[i].d.ibus.command;
                }
                return true;
            }
        }
    }

    return false;
}
