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
#ifndef __PLAYBACK_H__
#define __PLAYBACK_H__

#include <stdint.h>
#include <media-interface/media-interface.h>

typedef enum {
    PB_CMD__RESUME,
    PB_CMD__PAUSE,
    PB_CMD__STOP
} pb_command_t;

typedef enum {
    PB_STATUS__PLAYING,
    PB_STATUS__PAUSED,
    PB_STATUS__STOPPED,
    PB_STATUS__END_OF_SONG,
    PB_STATUS__ERROR
} pb_status_t;

typedef void (*playback_callback_fn_t)( const pb_status_t status,
                                        const int32_t tx_id );

/**
 *  Used to initialize the playback system.
 *
 *  @param priority the priority of the thread to run at
 *
 *  @returns 0 on success, -1 on error
 */
int32_t playback_init( const uint32_t priority );

/**
 *  Used to start playing a song.
 *
 *  @param filename the song to start playing
 *  @param cb_fn the callback to call with information
 *
 *  @returns -1 on error, transaction id otherwise
 */
int32_t playback_play( const char *filename,
                       const uint32_t gain,
                       const uint32_t peak,
                       media_play_fn_t play_fn,
                       playback_callback_fn_t cb_fn );

/**
 *  Used to command the playback system.
 *
 *  @param command the command to apply to the system
 *  @param cb_fn the callback to call with information
 *
 *  @returns -1 on error, transaction id otherwise
 */
int32_t playback_command( const pb_command_t command,
                          playback_callback_fn_t cb_fn );

#endif
