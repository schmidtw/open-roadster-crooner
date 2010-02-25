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
#ifndef __IRP_PROTOCOL_H__
#define __IRP_PROTOCOL_H__

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    IRP_RETURN_OK       = 0x0000,
    IRP_ERROR_PARAMETER = 0x0001
} irp_status_t;

typedef enum {
    IRP_CMD__GET_STATUS,
    IRP_CMD__STOP,
    IRP_CMD__PAUSE,
    IRP_CMD__PLAY,
    IRP_CMD__FAST_PLAY__FORWARD,
    IRP_CMD__FAST_PLAY__REVERSE,
    IRP_CMD__SEEK__NEXT,
    IRP_CMD__SEEK__PREV,
    IRP_CMD__SCAN_DISC__ENABLE,
    IRP_CMD__SCAN_DISC__DISABLE,
    IRP_CMD__RANDOMIZE__ENABLE,
    IRP_CMD__RANDOMIZE__DISABLE,
    IRP_CMD__CHANGE_DISC,
    IRP_CMD__POLL,
    IRP_CMD__TRAFFIC
} irp_cmd_t;

typedef enum {
    IRP_STATE__STOPPED,
    IRP_STATE__PAUSED,
    IRP_STATE__PLAYING,
    IRP_STATE__FAST_PLAYING__FORWARD,
    IRP_STATE__FAST_PLAYING__REVERSE,
    IRP_STATE__SEEKING,
    IRP_STATE__SEEKING__NEXT,
    IRP_STATE__SEEKING__PREV,
    IRP_STATE__LOADING_DISC
} irp_state_t;

typedef struct {
    irp_cmd_t command;
    uint8_t disc;       /* The new disc number if command is
                         * IRP_CMD__CHANGE_DISC, 0 otherwise. */
} irp_rx_msg_t;

/**
 *  Used to initialize the ibus radio system
 */
void irp_init( void );

/**
 *  Used to get the next message off the wire.
 *
 *  @note This function blocks the current thread until a new
 *        message is available
 *
 *  @param msg the pointer to populate with the message information
 *
 *  @return Status
 *      @retval IRP_RETURN_OK       Success
 *      @retval IRP_ERROR_PARAMETER Failure
 */
irp_status_t irp_get_message( irp_rx_msg_t *msg );

/**
 *  Used to send an announcement message to the network.
 *
 *  @note This function blocks the current thread until it is
 *        able to queue the message for sending to the radio.
 */
void irp_send_announce( void );

/**
 *  Used to send a response to a poll request
 *
 *  @note This function blocks the current thread until it is
 *        able to queue the message for sending to the radio.
 */
void irp_send_poll_response( void );

/**
 *  Used to send a normal response message to the radio.
 *
 *  @note This function blocks the current thread until it is
 *        able to queue the message for sending to the radio.
 *
 *  @return Status
 *      @retval IRP_RETURN_OK       Success
 *      @retval IRP_ERROR_PARAMETER Failure
 */
irp_status_t irp_send_normal_status( const irp_state_t device_state,
                                     const bool magazine_present,
                                     const uint8_t discs_present,
                                     const uint8_t current_disc,
                                     const uint8_t current_track );

/**
 *  Used to indicate that a particular disc is being checked for.
 *  This is typically called once for each disc, prior to the
 *  irp_completed_disc_check() call is made for each disc.
 *
 *  @note This function blocks the current thread until it is
 *        able to queue the message for sending to the radio.
 *
 *  @example
 *
 *  irp_going_to_check_disc( 1, 0x00 );
 *  irp_completed_disc_check( 1, true, 0x01 );
 *  irp_going_to_check_disc( 2, 0x01 );
 *  irp_completed_disc_check( 2, false, 0x01 );
 *  irp_going_to_check_disc( 3, 0x01 );
 *  irp_completed_disc_check( 3, true, 0x05 );
 *
 *  @param disc the disc being checked for
 *  @param active_map the current map of active discs
 *
 *  @return Status
 *      @retval IRP_RETURN_OK       Success
 *      @retval IRP_ERROR_PARAMETER Failure
 */
irp_status_t irp_going_to_check_disc( const uint8_t disc,
                                      const uint8_t active_map );

/**
 *  Used to indicate that a disc has been checked for.
 *
 *  @note This function blocks the current thread until it is
 *        able to queue the message for sending to the radio.
 *
 *  @param disc the disc that was checked for
 *  @param disc_present true if there is disc in that slot, false otherwise
 *  @param active_map the current map of active discs (including this disc)
 *
 *  @return Status
 *      @retval IRP_RETURN_OK       Success
 *      @retval IRP_ERROR_PARAMETER Failure
 */
irp_status_t irp_completed_disc_check( const uint8_t disc,
                                       const bool disc_present,
                                       const uint8_t active_map );

/**
 *  Used to convert the state type into a string.
 *
 *  @param state the state to convert to a string
 *
 *  @return string form of the state
 */
const char* irp_state_to_string( const irp_state_t state );

/**
 *  Used to convert the command type into a string.
 *
 *  @param cmd the command to convert to a string
 *
 *  @return string form of the state
 */
const char* irp_cmd_to_string( const irp_cmd_t cmd );
#endif
