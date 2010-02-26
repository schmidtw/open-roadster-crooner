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
#ifndef __RADIO_INTERFACE_H__
#define __RADIO_INTERFACE_H__

#include <stdbool.h>
#include <stdint.h>

#include <playback/playback.h>
#include <memcard/memcard.h>
#include <ibus-radio-protocol/ibus-radio-protocol.h>

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

/**
 *  Used to initialize the radio interface subsystem.
 *
 *  @return true on success, false otherwise.
 */
bool ri_init( void );

/**
 *  Used to send state from the user interface implementations.
 *
 *  @param device_status the status to report to the radio
 *  @param disc_map the bitmask of discs to report to the radio
 *  @param current_disc the current disc to report to the radio
 *  @param current_track the current track to report to the radio
 */
void ri_send_state( const irp_state_t device_status,
                    const irp_mode_t mode,
                    const uint8_t disc_map,
                    const uint8_t current_disc,
                    const uint8_t current_track );

/**
 *  Used to play a song from a user interface implementation.
 *
 *  @note Only use this function for playback - DO NOT DIRECTLY USE
 *        playback_play() because the messaging system will fail.
 *
 *  @param song the song to play
 *
 *  @return See playback_play() for details.
 */
int32_t ri_playback_play( song_node_t *song );

/**
 *  Used to control a song's playback from a user interface implementation.
 *
 *  @note Only use this function for playback control - DO NOT DIRECTLY USE
 *        playback_command() because the messaging system will fail.
 *
 *  @param song the song to play
 *  @return See playback_command() for details.
 */
int32_t ri_playback_command( const pb_command_t command );

#endif
