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
#ifndef __USER_INTERFACE_H__
#define __USER_INTERFACE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <database/database.h>
#include <ibus-radio-protocol/ibus-radio-protocol.h>

#include "radio-interface.h"

/**
 *  Used to get a pointer to any user data to be associated
 *  with this card session.  This data is passed to the other
 *  functions as user_data & is opaque.
 *
 *  @return a pointer to any user data to be passed around
 */
typedef void* (*ui_user_data_init_fct)( void );

/**
 *  Called with the user_data gotten from the ui_user_data_init_fct
 *  call to cleanup any card session data.
 *
 *  @param user_data the pointer to clean up
 */
typedef void (*ui_user_data_destroy_fct)( void *user_data );

/**
 *  Used to get the directory map list for the database
 *
 *  @note Returned map must remain valid until the release function
 *        is called.
 *
 *  @param size the size of the array in entries
 *
 *  @return map array of directories to map to buttons
 */
typedef const char** (*ui_dir_map_get_fct)( size_t *size );

/**
 *  Used to release the directory map list.
 *
 *  @param map the array of directories to release
 */
typedef void (*ui_dir_map_release_fct)( const char **map );

/**
 *  Called to get the desired disc and song information.
 *
 *  @param map the disc bitmap
 *  @param disc the starting disc
 *  @param track the starting track
 *  @param song the song node to initialize
 *  @param user_data the opaque user_data
 */
typedef void (*ui_get_disc_info_fct)( uint8_t *map,
                                      uint8_t *disc,
                                      uint8_t *track,
                                      song_node_t **song,
                                      void *user_data );

/**
 *  Called to get the starting disc for the memory card.
 *
 *  @param user_data the opaque user_data
 *
 */
typedef uint8_t (*ui_starting_disc_get_fct)( void *user_data );

/**
 *  Called to get the starting track for the memory card.
 *
 *  @param user_data the opaque user_data
 *
 */
typedef uint8_t (*ui_starting_track_get_fct)( void *user_data );

/**
 *  Called to allow the initial song to be set.
 *
 *  @param user_data the opaque user_data
 */
typedef void (*ui_initial_song_set_fct)( song_node_t **song, void *user_data );

/**
 *  Called to handle a command from the radio, or a playback event
 *  update.
 *
 *  @param device_status the current device status (should be changed to
 *                       reflect the new reality after the call)
 *  @param disc_map the disc bitmap
 *  @param current_disc the current disc (should be changed to reflect
 *                       the new reality after the call)
 *  @param current_track the current disc (should be changed to reflect
 *                       the new reality after the call)
 *  @param msg the message to process
 *  @param song the current song
 *  @param user_data the opaque user_data
 */
typedef void (*ui_process_command_fct)( irp_state_t *device_status,
                                        const uint8_t disc_map,
                                        uint8_t *current_disc,
                                        uint8_t *current_track,
                                        const ri_msg_t *msg,
                                        song_node_t **song,
                                        void *user_data );

typedef struct {
    const char *name;
    const ui_user_data_init_fct     ui_user_data_init_fn;       /* Optional */
    const ui_user_data_destroy_fct  ui_user_data_destroy_fn;    /* Optional */
    const ui_dir_map_get_fct        ui_dir_map_get_fn;
    const ui_dir_map_release_fct    ui_dir_map_release_fn;      /* Optional */
    const ui_get_disc_info_fct      ui_get_disc_info_fn;
    const ui_process_command_fct    ui_process_command_fn;

    const void (*impl_free)( void *);                           /* Optional */
} ui_impl_t;

/**
 *  Used to initialize the user interface system.
 *
 *  @return true on success, false otherwise
 */
bool ui_init( void );

/**
 *  Used to initialize a user interface implementation.
 *
 *  @note The memory passed to ui_register is owned by this function.  When
 *        the function is done with the memory the (*impl_free)() function
 *        will be called if not NULL to release the memory.
 *
 *  @param impl the user interface implementation
 *
 *  @return true on success, false otherwise
 */
bool ui_register( const ui_impl_t *impl );

/**
 *  Used to select the implementation to use.
 *
 *  @param name the name of the user interface implementation to use
 *
 *  @return true on success, false otherwise
 */
bool ui_select( const char *name );

/*----------------------------------------------------------------------------*/
/*              Used by the consumer of the user interface code.              */
/*----------------------------------------------------------------------------*/
void* ui_user_data_init( void );
void ui_user_data_destroy( void *user_data );
const char** ui_dir_map_get( size_t *size );
void ui_dir_map_release( const char **map );
void ui_get_disc_info( uint8_t *map, uint8_t *disc, uint8_t *track,
                       song_node_t **song, void *user_data );
void ui_process_command( irp_state_t *device_status, const uint8_t disc_map,
                         uint8_t *current_disc, uint8_t *current_track,
                         const ri_msg_t *msg, song_node_t **song,
                         void *user_data );
#endif
