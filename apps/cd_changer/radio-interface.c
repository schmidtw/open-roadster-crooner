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
#include <stdio.h>

#include <ibus-radio-protocol/ibus-radio-protocol.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <database/database.h>
#include <memcard/memcard.h>
#include <bsp/cpu.h>
#include <playback/playback.h>

#include "radio-interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define RI_DEBUG 0

#define DEBUG_STACK_BUFFER  0
#if (defined(RI_DEBUG) && (0 < RI_DEBUG))
#undef DEBUG_STACK_BUFFER
#define DEBUG_STACK_BUFFER  100
#endif

#define RI_POLL_TASK_STACK_SIZE  (configMINIMAL_STACK_SIZE+DEBUG_STACK_BUFFER)
#define RI_IBUS_TASK_STACK_SIZE  (configMINIMAL_STACK_SIZE)
#define RI_MSG_TASK_STACK_SIZE   (configMINIMAL_STACK_SIZE+250+DEBUG_STACK_BUFFER)
#define RI_DBASE_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE+1200)

#define RI_TASK_PRIORITY    (tskIDLE_PRIORITY+1)
#define RI_POLL_TIMEOUT     (TASK_DELAY_S(15))  /* 15 seconds */

#define _D1(...)
#define _D2(...)
#define _D3(...)

#if (defined(RI_DEBUG) && (0 < RI_DEBUG))
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif

#if (defined(RI_DEBUG) && (1 < RI_DEBUG))
#undef  _D2
#define _D2(...) printf( __VA_ARGS__ )
#endif

#if (defined(RI_DEBUG) && (2 < RI_DEBUG))
#undef  _D3
#define _D3(...) printf( __VA_ARGS__ )
#endif

#define DIR_MAP_SIZE    6
#define RI_MSG_MAX      20

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    irp_state_t device_status;
    bool magazine_present;
    uint8_t discs_present;
    uint8_t current_disc;
    uint8_t current_track;
} ri_state_t;

typedef enum {
    DBASE__POPULATE,
    DBASE__PURGE
} dbase_cmd_t;

typedef struct {
    dbase_cmd_t cmd;
    bool success;
} dbase_msg_t;

typedef struct {
    pb_status_t status;
    int32_t tx_id;
} ri_playback_msg_t;

typedef enum {
    RI_MSG_TYPE__CARD_STATUS,
    RI_MSG_TYPE__DBASE_STATUS,
    RI_MSG_TYPE__PLAYBACK_STATUS,
    RI_MSG_TYPE__IBUS_CMD
} ri_msg_type_t;

typedef struct {
    ri_msg_type_t type;
    union {
        dbase_msg_t         dbase;
        irp_rx_msg_t        ibus;
        mc_card_status_t    card;
        ri_playback_msg_t   song;
    } d;
} ri_msg_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static xSemaphoreHandle __poll_cmd;

static dbase_msg_t __dbase_msg;
static xQueueHandle __dbase_idle;
static xQueueHandle __dbase_active;

static ri_msg_t __ri_msg[RI_MSG_MAX];
static xQueueHandle __ri_idle;
static xQueueHandle __ri_active;
static const char *dir_map[DIR_MAP_SIZE] = { "1", "2", "3", "4", "5", "6" };

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __poll_task( void *params );
static void __blu_task( void *params );
static void __ibus_task( void *params );
static void __dbase_task( void *params );
static void __card_status( const mc_card_status_t status );
static void __database_populate( void );
static void __database_purge( void );
static void __send_state( ri_state_t *state );
static void __checking_complete( ri_state_t *state,
                                 const uint8_t found_map,
                                 const uint8_t starting_disc,
                                 const uint8_t starting_track );
static void __playback_cb( const pb_status_t status, const int32_t tx_id );
static void __transition_db( ri_state_t *state );
static void __no_discs_loop( ri_state_t *state );
static void __command_loop( ri_state_t *state );
static uint8_t __find_lowest_disc( const uint8_t map );

static uint8_t __determine_map( void );
static uint8_t __determine_starting_disc( void );
static uint8_t __determine_starting_track( void );
static void* __init_user_data( void );
static void __destroy_user_data( void *user_data );
static void __process_command( ri_state_t *state,
                               const ri_msg_t *msg,
                               song_node_t **song,
                               void *user_data );
static void __find_song( song_node_t **song, irp_cmd_t cmd, const uint8_t disc );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See radio-interface.h for details */
bool ri_init( void )
{
    int i;
    irp_init();

    vSemaphoreCreateBinary( __poll_cmd );
    xSemaphoreTake( __poll_cmd, 0 );

    __dbase_idle = xQueueCreate( 1, sizeof(void*) );
    __dbase_active = xQueueCreate( 1, sizeof(void*) );
    __ri_idle = xQueueCreate( RI_MSG_MAX, sizeof(void*) );
    __ri_active = xQueueCreate( RI_MSG_MAX, sizeof(void*) );

    if( (NULL == __dbase_idle) || (NULL == __dbase_active) ||
        (NULL == __ri_idle) || (NULL == __ri_active) ||
        (NULL == __poll_cmd) )
    {
        goto failure;
    }

    {
        dbase_msg_t *cmd;
        cmd = &__dbase_msg;
        xQueueSendToBack( __dbase_idle, &cmd, portMAX_DELAY );
    }

    for( i = 0; i < RI_MSG_MAX; i++ ) {
        ri_msg_t *m;

        m = &__ri_msg[i];
        xQueueSendToBack( __ri_idle, &m, portMAX_DELAY );
    }

    xTaskCreate( __poll_task, (signed portCHAR *) "iBusPoll",
                 RI_POLL_TASK_STACK_SIZE, NULL, RI_TASK_PRIORITY, NULL );
    xTaskCreate( __blu_task, (signed portCHAR *) "BLU",
                 RI_MSG_TASK_STACK_SIZE, NULL, RI_TASK_PRIORITY, NULL );
    xTaskCreate( __ibus_task, (signed portCHAR *) "iBusMsg",
                 RI_IBUS_TASK_STACK_SIZE, NULL, RI_TASK_PRIORITY, NULL );
    xTaskCreate( __dbase_task, (signed portCHAR *) "dbase",
                 RI_DBASE_TASK_STACK_SIZE, NULL, RI_TASK_PRIORITY, NULL );

    mc_register( &__card_status );

    return true;

failure:
    return false;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/**
 *  The poll response task.
 */
static void __poll_task( void *params )
{
    irp_send_announce();

    while( 1 ) {
        if( pdTRUE == xSemaphoreTake(__poll_cmd, RI_POLL_TIMEOUT) ) {
            _D3( "Poll Request\n" );
            irp_send_poll_response();
        } else {
            _D3( "ibus timed out\n" );
            irp_send_announce();
        }
    }
}

/**
 *  The business logic unit task.
 */
static void __blu_task( void *params )
{
    ri_state_t state;

    state.device_status = IRP_STATE__STOPPED;
    state.magazine_present = false;
    state.discs_present = 0;
    state.current_disc = 0;
    state.current_track = 0;

    while( 1 ) {
        ri_msg_t *msg;

        xQueueReceive( __ri_active, &msg, portMAX_DELAY );

        if( RI_MSG_TYPE__CARD_STATUS == msg->type ) {
            mc_card_status_t card;

            card = msg->d.card;
            xQueueSendToBack( __ri_idle, &msg, portMAX_DELAY );
            msg = NULL;

            switch( card ) {
                case MC_CARD__INSERTED:
                    _D2( "MC_CARD__INSERTED\n" );
                    state.magazine_present = true;
                    state.discs_present = 0;
                    state.current_disc = 0;
                    state.current_track = 0;
                    __send_state( &state );
                    break;

                case MC_CARD__MOUNTED:
                    _D2( "MC_CARD__MOUNTED\n" );
                    __transition_db( &state );

                    /* Once we exit, the card has been removed. */
                case MC_CARD__REMOVED:
                    _D2( "MC_CARD__REMOVED\n" );
                    state.device_status = IRP_STATE__STOPPED;
                    state.discs_present = 0;
                    state.current_disc = 0;
                    state.current_track = 0;
                    state.magazine_present = false;
                    __send_state( &state );
                    break;

                case MC_CARD__UNUSABLE:
                    _D2( "MC_CARD__UNUSABLE\n" );
                    state.device_status = IRP_STATE__STOPPED;
                    state.discs_present = 0;
                    state.current_disc = 0;
                    state.current_track = 0;
                    __send_state( &state );
                    break;

                default:
                    break;
            }
        } else {
            xQueueSendToBack( __ri_idle, &msg, portMAX_DELAY );
            __send_state( &state );
        }
    }
}

/**
 *  The ibus message response task.
 */
static void __ibus_task( void *params )
{
    while( 1 ) {
        ri_msg_t *msg;

        xQueueReceive( __ri_idle, &msg, portMAX_DELAY );

        msg->type = RI_MSG_TYPE__IBUS_CMD;

        do {
            irp_get_message( &msg->d.ibus );

            if( IRP_CMD__POLL == msg->d.ibus.command ) {
                xSemaphoreGive( __poll_cmd );
            }
        } while( IRP_CMD__POLL == msg->d.ibus.command );

        xQueueSendToBack( __ri_active, &msg, portMAX_DELAY );
        msg = NULL;
    }
}

/**
 *  The database mounting / unmounting task.
 */
static void __dbase_task( void *params )
{
    
    while( 1 ) {
        dbase_msg_t *dbase_msg;
        ri_msg_t *ri_msg;

        xQueueReceive( __dbase_active, &dbase_msg, portMAX_DELAY );
        xQueueReceive( __ri_idle, &ri_msg, portMAX_DELAY );

        if( DBASE__POPULATE == dbase_msg->cmd ) {
            ri_msg->d.dbase.success = populate_database( dir_map, DIR_MAP_SIZE, "/" );
        } else {
            database_purge();
            ri_msg->d.dbase.success = true;
        }

        ri_msg->type = RI_MSG_TYPE__DBASE_STATUS;
        ri_msg->d.dbase.cmd = dbase_msg->cmd;

        xQueueSendToBack( __dbase_idle, &dbase_msg, portMAX_DELAY );
        xQueueSendToBack( __ri_active, &ri_msg, portMAX_DELAY );
    }
}

/**
 *  The card status callback function.
 *
 *  @param status the new status of the card.
 */
static void __card_status( const mc_card_status_t status )
{
    ri_msg_t *msg;

    xQueueReceive( __ri_idle, &msg, portMAX_DELAY );
    msg->type = RI_MSG_TYPE__CARD_STATUS;
    msg->d.card = status;
    xQueueSendToBack( __ri_active, &msg, portMAX_DELAY );
}

/**
 *  The database populate request helper function.
 */
static void __database_populate( void )
{
    dbase_msg_t *dbase_msg;
    xQueueReceive( __dbase_idle, &dbase_msg, portMAX_DELAY );
    dbase_msg->cmd = DBASE__POPULATE;
    xQueueSendToBack( __dbase_active, &dbase_msg, portMAX_DELAY );
}

/**
 *  The database purge request helper function.
 */
static void __database_purge( void )
{
    dbase_msg_t *dbase_msg;
    xQueueReceive( __dbase_idle, &dbase_msg, portMAX_DELAY );
    dbase_msg->cmd = DBASE__PURGE;
    xQueueSendToBack( __dbase_active, &dbase_msg, portMAX_DELAY );
}

/**
 *  The send ibus system state helper function.
 *
 *  @param state the state of the device to send.
 */
static void __send_state( ri_state_t *state )
{
    _D3( "Sending:\n"
         "       Status: 0x%04x\n"
         "     Magazine: %s\n"
         "        Discs: 0x%08x\n"
         " Current Disc: %d\n"
         "Current Track: %d\n",
         state->device_status,
         (true == state->magazine_present) ? "present" : "not present",
         state->discs_present,
         state->current_disc,
         state->current_track );

    irp_send_normal_status( state->device_status,
                            state->magazine_present,
                            state->discs_present,
                            state->current_disc,
                            state->current_track );
}

/**
 *  The function that completes the disc checking routine and
 *  lets the radio know the new state.
 *
 *  @param state the system state to adjust
 *  @param found_map the bitmask map of the available discs
 *  @param starting_disc the disc to report the playback is start with
 *  @param starting_track the track to report the playback is start with
 */
static void __checking_complete( ri_state_t *state,
                                 const uint8_t found_map,
                                 const uint8_t starting_disc,
                                 const uint8_t starting_track )
{
    int32_t i;

    for( i = 1; i <= 6; i++ ) {
        bool disc_present;
        uint8_t active_map;

        disc_present = ((1 << (i - 1)) == (found_map & ((1 << (i - 1)))));
        active_map = (found_map & (0x3f >> (6 - i)));

        _D1( "%ld - preset: %s active_map: 0x%02x\n", i,
             ((true == disc_present) ? "true" : "false"),
             active_map );

        irp_completed_disc_check( i, disc_present, active_map );
        if( i < 6 ) {
            irp_going_to_check_disc( (i+1), active_map );
        }
    }

    state->discs_present = found_map;
    if( 0x00 == found_map ) {
        state->device_status = IRP_STATE__STOPPED;
        state->current_disc = 0;
        state->current_track = 0;
    } else {
        state->current_disc = starting_disc;
        state->current_track = starting_track;
    }

    __send_state( state );
}

/**
 *  Function that takes the playback status and creates the message to 
 *  send to the blu task.
 *
 *  @param status the new playback status
 *  @param tx_id the id this callback is associated with
 */
static void __playback_cb( const pb_status_t status, const int32_t tx_id )
{
    ri_msg_t *msg;

    xQueueReceive( __ri_idle, &msg, portMAX_DELAY );
    msg->type = RI_MSG_TYPE__PLAYBACK_STATUS;
    msg->d.song.status = status;
    msg->d.song.tx_id = tx_id;
    xQueueSendToBack( __ri_active, &msg, portMAX_DELAY );
}

/**
 *  Used to transition the database into the populated state and then
 *  continue supporting the state machine.  When this functions returns,
 *  the database has been purged.
 *
 *  @param state the system state
 */
static void __transition_db( ri_state_t *state )
{
    bool keep_going;

    irp_going_to_check_disc( 1, 0 );

    __database_populate();

    keep_going = true;
    while( true == keep_going ) {
        ri_msg_t *msg;

        xQueueReceive( __ri_active, &msg, portMAX_DELAY );

        if( (RI_MSG_TYPE__DBASE_STATUS == msg->type) &&
            (DBASE__POPULATE == msg->d.dbase.cmd) )
        {
            if( true == msg->d.dbase.success ) {
                uint8_t map;
                uint8_t starting_disc;
                uint8_t starting_track;

                keep_going = false;
                xQueueSendToBack( __ri_idle, &msg, portMAX_DELAY );

                map = __determine_map();

                starting_disc = __determine_starting_disc();
                if( 6 < starting_disc ) {
                    starting_disc = __find_lowest_disc( map );
                }
                starting_track = __determine_starting_track();

                __checking_complete( state, map, starting_disc, starting_track );

                __send_state( state );

                __command_loop( state );
            } else {
                __checking_complete( state, 0, 0, 0 );
                __send_state( state );
                __no_discs_loop( state );
                return;
            }
        } else {
            xQueueSendToBack( __ri_idle, &msg, portMAX_DELAY );
        }
    }

    playback_command( PB_CMD__STOP, NULL );

    __database_purge();

    keep_going = true;
    while( true == keep_going ) {
        ri_msg_t *msg;

        xQueueReceive( __ri_active, &msg, portMAX_DELAY );

        if( (RI_MSG_TYPE__DBASE_STATUS == msg->type) &&
            (DBASE__PURGE == msg->d.dbase.cmd) )
        {
            keep_going = false;
        }

        xQueueSendToBack( __ri_idle, &msg, portMAX_DELAY );
        __send_state( state );
    }
}

/**
 *  The function that supports the case where no discs are present
 *  after walking the database.  This function only returns when the
 *  current disc is removed.
 *
 *  @param state the system state
 */
static void __no_discs_loop( ri_state_t *state )
{
    bool keep_going;

    keep_going = true;
    while( true == keep_going ) {
        ri_msg_t *msg;

        xQueueReceive( __ri_active, &msg, portMAX_DELAY );

        if( (RI_MSG_TYPE__PLAYBACK_STATUS != msg->type) &&
            (RI_MSG_TYPE__IBUS_CMD != msg->type) )
        {
            keep_going = false;
            __send_state( state );
            xQueueSendToFront( __ri_active, &msg, portMAX_DELAY );
        } else {
            xQueueSendToBack( __ri_idle, &msg, portMAX_DELAY );
        }
        __send_state( state );
    }
}

/**
 *  Provides the basic key handling functionality after the 
 *  card has been inserted, and the database has been populated.
 *  Only returns when the card has been removed.
 *
 *  @param state the system state
 */
static void __command_loop( ri_state_t *state )
{
    bool keep_going;
    song_node_t *song;
    void *user_data;

    song = NULL;
    user_data = __init_user_data();

    keep_going = true;
    while( true == keep_going ) {
        ri_msg_t *msg;

        xQueueReceive( __ri_active, &msg, portMAX_DELAY );

        if( (RI_MSG_TYPE__PLAYBACK_STATUS == msg->type) ||
            (RI_MSG_TYPE__IBUS_CMD == msg->type) )
        {
            if( (RI_MSG_TYPE__IBUS_CMD == msg->type) &&
                (IRP_CMD__GET_STATUS == msg->d.ibus.command) )
            {
                __send_state( state );
            } else {
                __process_command( state, msg, &song, user_data );
            }
        } else {
            keep_going = false;
            __send_state( state );
        }
        xQueueSendToBack( __ri_idle, &msg, portMAX_DELAY );
    }
    __destroy_user_data( user_data );
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

/*----------------------------------------------------------------------------*/
/*                          To be made library-itized                         */
/*----------------------------------------------------------------------------*/

/**
 *  Called to get the active map for the current disc.
 *
 *  @return the active map for the disc
 */
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

/**
 *  Called to get the starting disc for playback.
 *
 *  @return the starting disc, or 0xff for "auto detect" of the lowest
 *          numbered disc
 */
static uint8_t __determine_starting_disc( void )
{
    return 0xff;
}

/**
 *  Called to get the starting track for playback.
 *
 *  @return the starting track for playback (1-99)
 */
static uint8_t __determine_starting_track( void )
{
    return 1;
}

/**
 *  Called to allow for user data to be initialized and
 *  returned when the disc is first inserted.
 *
 *  @return the user_data to pass in other function calls, or NULL
 */
static void* __init_user_data( void )
{
    return NULL;
}

/**
 *  Called after the disc has been removed to allow for cleaning
 *  up the user data.
 *
 *  @param the user_data provided by __init_user_data()
 */
static void __destroy_user_data( void *user_data )
{
}

/**
 *  Called for each ibus command or playback status update to allow
 *  for multiple implementations.
 *
 *  @param state the system state
 *  @param msg the current message - must not be re-used or freed
 *  @param song the current song
 *  @param user_data any user data provided by __init_user_data()
 */
static void __process_command( ri_state_t *state,
                               const ri_msg_t *msg,
                               song_node_t **song,
                               void *user_data )
{
    _D2( "state->device_status: 0x%04x\n", state->device_status );

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
                if( (IRP_STATE__PLAYING == state->device_status) ||
                    (IRP_STATE__PAUSED == state->device_status) )
                {
                    playback_command( PB_CMD__STOP, __playback_cb );
                }
                break;

            case IRP_CMD__PAUSE:
                _D2( "IRP_CMD__PAUSE\n" );
                if( IRP_STATE__PLAYING == state->device_status ) {
                    playback_command( PB_CMD__PAUSE, __playback_cb );
                }
                break;

            case IRP_CMD__PLAY:
                _D2( "IRP_CMD__PLAY\n" );
                if( IRP_STATE__STOPPED == state->device_status ) {
                    if( NULL == *song ) {
                        __find_song( song, IRP_CMD__CHANGE_DISC, state->current_disc );
                    }
                    playback_play( (*song)->file_location, (*song)->track_gain, (*song)->track_peak,
                                   (*song)->play_fn, __playback_cb );
                } else if( IRP_STATE__PAUSED == state->device_status ) {
                    playback_command( PB_CMD__RESUME, __playback_cb );
                }
                break;

            case IRP_CMD__FAST_PLAY__FORWARD:
                _D2( "IRP_CMD__FAST_PLAY__FORWARD\n" );

                if( IRP_STATE__FAST_PLAYING__FORWARD != state->device_status ) {
                    state->device_status = IRP_STATE__FAST_PLAYING__FORWARD;
                    __find_song( song, msg->d.ibus.command, 0 );
                    playback_play( (*song)->file_location, (*song)->track_gain, (*song)->track_peak,
                                   (*song)->play_fn, __playback_cb );
                }
                break;

            case IRP_CMD__FAST_PLAY__REVERSE:
                _D2( "IRP_CMD__FAST_PLAY__REVERSE\n" );
                if( IRP_STATE__FAST_PLAYING__REVERSE != state->device_status ) {
                    state->device_status = IRP_STATE__FAST_PLAYING__REVERSE;
                    __find_song( song, msg->d.ibus.command, 0 );
                    playback_play( (*song)->file_location, (*song)->track_gain, (*song)->track_peak,
                                   (*song)->play_fn, __playback_cb );
                }
                break;

            case IRP_CMD__SEEK__NEXT:
                _D2( "IRP_CMD__SEEK__NEXT\n" );
                state->device_status = IRP_STATE__SEEKING__NEXT;
                __send_state( state );
                __find_song( song, msg->d.ibus.command, 0 );
                playback_play( (*song)->file_location, (*song)->track_gain, (*song)->track_peak,
                               (*song)->play_fn, __playback_cb );
                break;

            case IRP_CMD__SEEK__PREV:
                _D2( "IRP_CMD__SEEK__PREV\n" );
                state->device_status = IRP_STATE__SEEKING__PREV;
                __send_state( state );
                __find_song( song, msg->d.ibus.command, 0 );
                playback_play( (*song)->file_location, (*song)->track_gain, (*song)->track_peak,
                               (*song)->play_fn, __playback_cb );
                break;

            case IRP_CMD__CHANGE_DISC:
                _D2( "IRP_CMD__CHANGE_DISC\n" );
                state->device_status = IRP_STATE__LOADING_DISC;
                __send_state( state );
                __find_song( song, msg->d.ibus.command, msg->d.ibus.disc );
                state->current_disc = msg->d.ibus.disc;
                playback_play( (*song)->file_location, (*song)->track_gain, (*song)->track_peak,
                               (*song)->play_fn, __playback_cb );
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
                state->current_track = (*song)->track_number;
                state->device_status = IRP_STATE__PLAYING;
                break;
            case PB_STATUS__PAUSED:
                _D2( "PB_STATUS__PAUSED\n" );
                state->device_status = IRP_STATE__PAUSED;
                break;
            case PB_STATUS__STOPPED:
                _D2( "PB_STATUS__STOPPED\n" );
                state->device_status = IRP_STATE__STOPPED;
                break;

            case PB_STATUS__ERROR:
                _D2( "PB_STATUS__ERROR\n" );
                /* Without this we seem to deadlock on "full error" testing. */
                /* The real error seems to be due to the physical ibus driver. */
                vTaskDelay( 100 );

            case PB_STATUS__END_OF_SONG:
                _D2( "PB_STATUS__END_OF_SONG\n" );
                state->device_status = IRP_STATE__SEEKING__NEXT;
                __send_state( state );
                __find_song( song, IRP_CMD__SEEK__NEXT, 0 );
                playback_play( (*song)->file_location, (*song)->track_gain, (*song)->track_peak,
                               (*song)->play_fn, __playback_cb );
                break;
        }
        __send_state( state );
    }
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
                    if( 0 == strcasecmp(dir_map[disc - 1],
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
