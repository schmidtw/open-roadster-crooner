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
#include <freertos/os.h>

#include <database/database.h>
#include <memcard/memcard.h>
#include <bsp/cpu.h>
#include <playback/playback.h>

#include "radio-interface.h"
#include "device-status.h"
#include "user-interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define RI_DEBUG    0
#define RI_TIMEOUT  25000

#define DEBUG_STACK_BUFFER  0
#if (defined(RI_DEBUG) && (0 < RI_DEBUG))
#undef DEBUG_STACK_BUFFER
#define DEBUG_STACK_BUFFER  100
#endif

#define RI_POLL_TASK_STACK_SIZE  (550+DEBUG_STACK_BUFFER)
#define RI_IBUS_TASK_STACK_SIZE  (600)
#define RI_BLU_TASK_STACK_SIZE   (600+DEBUG_STACK_BUFFER)
#define RI_DBASE_TASK_STACK_SIZE (900)

#define RI_TASK_PRIORITY    1
#define RI_POLL_TIMEOUT     15000   /* 15 seconds */

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

#define RI_MSG_MAX      20

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    irp_state_t device_status;
    irp_mode_t device_mode;
    bool magazine_present;
    uint8_t discs_present;
    uint8_t current_disc;
    uint8_t current_track;
    bool magazine_indexed;
} ri_state_t;


/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static semaphore_handle_t __poll_cmd;

static dbase_msg_t __dbase_msg;
static queue_handle_t __dbase_idle;
static queue_handle_t __dbase_active;

static ri_msg_t __ri_msg[RI_MSG_MAX];
static queue_handle_t __ri_idle;
static queue_handle_t __ri_active;
static volatile bool __connected_to_radio;

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
static void __command_loop( ri_state_t *state, song_node_t **song, void *user_data );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See radio-interface.h for details */
bool ri_init( void )
{
    int i;
    irp_init();

    __connected_to_radio = false;

     __poll_cmd = os_semaphore_create_binary();
    os_semaphore_take( __poll_cmd, NO_WAIT );

    __dbase_idle = os_queue_create( 1, sizeof(void*) );
    __dbase_active = os_queue_create( 1, sizeof(void*) );
    __ri_idle = os_queue_create( RI_MSG_MAX, sizeof(void*) );
    __ri_active = os_queue_create( RI_MSG_MAX, sizeof(void*) );

    if( (NULL == __dbase_idle) || (NULL == __dbase_active) ||
        (NULL == __ri_idle) || (NULL == __ri_active) ||
        (NULL == __poll_cmd) )
    {
        goto failure;
    }

    {
        dbase_msg_t *cmd;
        cmd = &__dbase_msg;
        os_queue_send_to_back( __dbase_idle, &cmd, WAIT_FOREVER );
    }

    for( i = 0; i < RI_MSG_MAX; i++ ) {
        ri_msg_t *m;

        m = &__ri_msg[i];
        os_queue_send_to_back( __ri_idle, &m, WAIT_FOREVER );
    }

    os_task_create( __poll_task, "iBusPoll", RI_POLL_TASK_STACK_SIZE,
                    NULL, RI_TASK_PRIORITY, NULL );

    os_task_create( __blu_task, "BLU", RI_BLU_TASK_STACK_SIZE,
                    NULL, RI_TASK_PRIORITY, NULL );

    os_task_create( __ibus_task, "iBusMsg", RI_IBUS_TASK_STACK_SIZE,
                    NULL, RI_TASK_PRIORITY, NULL );

    os_task_create( __dbase_task, "dbase", RI_DBASE_TASK_STACK_SIZE,
                    NULL, RI_TASK_PRIORITY, NULL );

    mc_register( &__card_status );

    return true;

failure:
    return false;
}

/* See radio-interface.h for details */
void ri_send_state( const irp_state_t device_status,
                    const irp_mode_t device_mode,
                    const uint8_t disc_map,
                    const uint8_t current_disc,
                    const uint8_t current_track )
{
    ri_state_t state;

    state.device_status = device_status;
    state.device_mode = device_mode;
    state.magazine_present = true;
    state.discs_present = disc_map;
    state.current_disc = current_disc;
    state.current_track = current_track;

    __send_state( &state );
}

/* See radio-interface.h for details */
int32_t ri_playback_play( song_node_t *song )
{
    return playback_play( song->file_location, song->d.d.gain.track_gain,
                          song->d.d.gain.track_peak, song->play_fn, &__playback_cb );
}

/* See radio-interface.h for details */
int32_t ri_playback_command( const pb_command_t command )
{
    return playback_command( command, &__playback_cb );
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/**
 *  The poll response task.
 */
static void __poll_task( void *params )
{
    int32_t timeout;

    timeout = RI_TIMEOUT;

    while( 1 ) {
        if( true == os_semaphore_take(__poll_cmd, RI_POLL_TIMEOUT) ) {
            _D3( "Poll Request\n" );
            if( timeout <= 0 ) {
                irp_send_announce();
            }
            timeout = RI_TIMEOUT;
            irp_send_poll_response();
            __connected_to_radio = true;
            /* If nothing else is going on, set the output to normal,
             * otherwise the system will figure it out. */
            if( DS__NO_RADIO_CONNECTION == device_status_get() ) {
                device_status_set( DS__NORMAL );
            }
        } else {
            if( 0 < timeout ) {
                timeout -= RI_POLL_TIMEOUT;
            } else {
                ri_msg_t *ri_msg;

                _D3( "ibus timed out\n" );
                device_status_set( DS__NO_RADIO_CONNECTION );
                __connected_to_radio = false;
                os_queue_receive( __ri_idle, &ri_msg, WAIT_FOREVER );
                ri_msg->type = RI_MSG_TYPE__IBUS_CMD;
                ri_msg->d.ibus.command = IRP_CMD__STOP;
                os_queue_send_to_back( __ri_active, &ri_msg, WAIT_FOREVER );
            }
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
    state.device_status = IRP_MODE__NORMAL;
    state.magazine_present = false;
    state.magazine_indexed = false;
    state.discs_present = 0;
    state.current_disc = 0;
    state.current_track = 0;

    while( 1 ) {
        ri_msg_t *msg;

        os_queue_receive( __ri_active, &msg, WAIT_FOREVER );

        if( RI_MSG_TYPE__CARD_STATUS == msg->type ) {
            mc_card_status_t card;

            card = msg->d.card;
            os_queue_send_to_back( __ri_idle, &msg, WAIT_FOREVER );
            msg = NULL;

            switch( card ) {
                case MC_CARD__INSERTED:
                    _D2( "MC_CARD__INSERTED\n" );
                    state.magazine_present = true;
                    state.discs_present = 0;
                    state.current_disc = 0;
                    state.current_track = 0;
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
                    state.magazine_indexed = false;
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
            os_queue_send_to_back( __ri_idle, &msg, WAIT_FOREVER );
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

        os_queue_receive( __ri_idle, &msg, WAIT_FOREVER );

        msg->type = RI_MSG_TYPE__IBUS_CMD;

        do {
            irp_get_message( &msg->d.ibus );

            if( (IRP_CMD__POLL == msg->d.ibus.command) ||
                (false == __connected_to_radio) )
            {
                os_semaphore_give( __poll_cmd );
            }
        } while( (IRP_CMD__POLL == msg->d.ibus.command) ||
                 (IRP_CMD__TRAFFIC == msg->d.ibus.command) );

        os_queue_send_to_back( __ri_active, &msg, WAIT_FOREVER );
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

        os_queue_receive( __dbase_active, &dbase_msg, WAIT_FOREVER );
        os_queue_receive( __ri_idle, &ri_msg, WAIT_FOREVER );

        if( DBASE__POPULATE == dbase_msg->cmd ) {
            ri_msg->d.dbase.success = populate_database( "/" );
        } else {
            database_purge();
            ri_msg->d.dbase.success = true;
        }

        ri_msg->type = RI_MSG_TYPE__DBASE_STATUS;
        ri_msg->d.dbase.cmd = dbase_msg->cmd;

        os_queue_send_to_back( __dbase_idle, &dbase_msg, WAIT_FOREVER );
        os_queue_send_to_back( __ri_active, &ri_msg, WAIT_FOREVER );
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

    os_queue_receive( __ri_idle, &msg, WAIT_FOREVER );
    msg->type = RI_MSG_TYPE__CARD_STATUS;
    msg->d.card = status;
    os_queue_send_to_back( __ri_active, &msg, WAIT_FOREVER );
}

/**
 *  The database populate request helper function.
 */
static void __database_populate( void )
{
    dbase_msg_t *dbase_msg;
    os_queue_receive( __dbase_idle, &dbase_msg, WAIT_FOREVER );
    dbase_msg->cmd = DBASE__POPULATE;
    os_queue_send_to_back( __dbase_active, &dbase_msg, WAIT_FOREVER );
}

/**
 *  The database purge request helper function.
 */
static void __database_purge( void )
{
    dbase_msg_t *dbase_msg;
    os_queue_receive( __dbase_idle, &dbase_msg, WAIT_FOREVER );
    dbase_msg->cmd = DBASE__PURGE;
    os_queue_send_to_back( __dbase_active, &dbase_msg, WAIT_FOREVER );
}

/**
 *  The send ibus system state helper function.
 *
 *  @param state the state of the device to send.
 */
static void __send_state( ri_state_t *state )
{
    _D3( "%s:\n"
         "       Status: 0x%04x - %s\n"
         "     Magazine: %s\n"
         "        Discs: 0x%08x\n"
         " Current Disc: %d\n"
         "Current Track: %d\n",
         (true == __connected_to_radio) ? "Sending" : "Not sending",
         state->device_status,
         irp_state_to_string(state->device_status),
         (true == state->magazine_present) ? "present" : "not present",
         state->discs_present,
         state->current_disc,
         state->current_track );

    if( (true == state->magazine_present) && (0 == state->discs_present) ) {
        if( true == state->magazine_indexed ) {
            /* No discs */
            device_status_set( DS__CARD_UNUSABLE );
        }
    } else {
        /* No magazine / Normal operation */
        if( true == __connected_to_radio ) {
            device_status_set( DS__NORMAL );
        } else {
            device_status_set( DS__NO_RADIO_CONNECTION );
        }
    }

    if( true == __connected_to_radio ) {
        irp_send_normal_status( state->device_status,
                                state->device_mode,
                                state->magazine_present,
                                state->discs_present,
                                state->current_disc,
                                state->current_track );
    }
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

        if( true == __connected_to_radio ) {
            printf( "goal: %s\n", irp_state_to_string(state->device_status) );
            irp_completed_disc_check( i, disc_present, active_map, state->device_status );
        }
        if( i < 6 ) {
            if( true == __connected_to_radio ) {
                printf( "goal: %s\n", irp_state_to_string(state->device_status) );
                irp_going_to_check_disc( (i+1), active_map, state->device_status );
            }
        }
    }

    state->discs_present = found_map;
    if( 0x00 == found_map ) {
        state->device_status = IRP_STATE__STOPPED;
        state->current_disc = 0;
        state->current_track = 0;
    } else {
        irp_state_t goal_state;

        goal_state = state->device_status;

        /* If we change discs, send that event. */
        if( state->current_disc != starting_disc ) {
            state->device_status = IRP_STATE__LOADING_DISC;
            state->current_disc = starting_disc;
            state->current_track = 0;

            printf( "Sending disc: %d\n", state->current_disc );
            __send_state( state );
        }

        /* Send that we are seeking to the track */
        state->device_status = IRP_STATE__SEEKING;
        state->current_disc = starting_disc;
        state->current_track = starting_track;
        __send_state( state );

        /* Send that final status */
        state->device_status = goal_state;
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

    os_queue_receive( __ri_idle, &msg, WAIT_FOREVER );
    msg->type = RI_MSG_TYPE__PLAYBACK_STATUS;
    msg->d.song.status = status;
    msg->d.song.tx_id = tx_id;
    os_queue_send_to_back( __ri_active, &msg, WAIT_FOREVER );
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

    if( true == __connected_to_radio ) {
        printf( "goal: %s\n", irp_state_to_string(state->device_status) );
        irp_going_to_check_disc( 1, 0, state->device_status );
    }

    device_status_set( DS__CARD_BEING_SCANNED );

    __database_populate();

    keep_going = true;
    while( true == keep_going ) {
        ri_msg_t *msg;

        os_queue_receive( __ri_active, &msg, WAIT_FOREVER );

        if( (RI_MSG_TYPE__DBASE_STATUS == msg->type) &&
            (DBASE__POPULATE == msg->d.dbase.cmd) )
        {
            if( true == msg->d.dbase.success ) {
                uint8_t map;
                uint8_t starting_disc;
                uint8_t starting_track;
                song_node_t *song;
                void *user_data;

                state->magazine_indexed = true;
                keep_going = false;
                os_queue_send_to_back( __ri_idle, &msg, WAIT_FOREVER );

                user_data = ui_user_data_init();

                ui_get_disc_info( &map, &starting_disc, &starting_track,
                                  &song, user_data );

                __checking_complete( state, map, starting_disc, starting_track );

                __command_loop( state, &song, user_data );

                ui_user_data_destroy( user_data );
            } else {
                device_status_set( DS__CARD_UNUSABLE );
                __checking_complete( state, 0, 0, 0 );
                __no_discs_loop( state );
                return;
            }
        } else if( RI_MSG_TYPE__IBUS_CMD == msg->type ) {
            if( true == __connected_to_radio ) {
                printf( "goal: %s\n", irp_state_to_string(state->device_status) );
                irp_going_to_check_disc( 1, 0, state->device_status );
            }
            os_queue_send_to_back( __ri_idle, &msg, WAIT_FOREVER );
        } else {
            os_queue_send_to_back( __ri_idle, &msg, WAIT_FOREVER );
            return;
        }
    }

    playback_command( PB_CMD__STOP, NULL );

    __database_purge();

    keep_going = true;
    while( true == keep_going ) {
        ri_msg_t *msg;

        os_queue_receive( __ri_active, &msg, WAIT_FOREVER );

        if( (RI_MSG_TYPE__DBASE_STATUS == msg->type) &&
            (DBASE__PURGE == msg->d.dbase.cmd) )
        {
            keep_going = false;
        }

        os_queue_send_to_back( __ri_idle, &msg, WAIT_FOREVER );
        __send_state( state );
    }
    state->magazine_indexed = false;
    device_status_set( DS__NORMAL );
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

        os_queue_receive( __ri_active, &msg, WAIT_FOREVER );

        if( (RI_MSG_TYPE__PLAYBACK_STATUS != msg->type) &&
            (RI_MSG_TYPE__IBUS_CMD != msg->type) )
        {
            keep_going = false;
            __send_state( state );
            os_queue_send_to_front( __ri_active, &msg, WAIT_FOREVER );
        } else {
            os_queue_send_to_back( __ri_idle, &msg, WAIT_FOREVER );
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
static void __command_loop( ri_state_t *state, song_node_t **song, void *user_data )
{
    bool keep_going;

    keep_going = true;
    while( true == keep_going ) {
        ri_msg_t *msg;

        os_queue_receive( __ri_active, &msg, WAIT_FOREVER );

        if( (RI_MSG_TYPE__PLAYBACK_STATUS == msg->type) ||
            (RI_MSG_TYPE__IBUS_CMD == msg->type) )
        {
            if( (RI_MSG_TYPE__IBUS_CMD == msg->type) &&
                (IRP_CMD__GET_STATUS == msg->d.ibus.command) )
            {
                __send_state( state );
            } else {
                ui_process_command( &state->device_status,
                                    &state->device_mode,
                                    state->discs_present,
                                    &state->current_disc,
                                    &state->current_track,
                                    msg, song, user_data );
            }
        } else {
            _D2( "__command_loop[%d] - type 0x%04x\n\tibus.command 0x%04x\n",
                    __LINE__, msg->type, msg->d.ibus.command );
            keep_going = false;
            __send_state( state );
        }
        os_queue_send_to_back( __ri_idle, &msg, WAIT_FOREVER );
    }
}
