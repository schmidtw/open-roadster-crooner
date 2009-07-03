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
#ifndef __MEDIA_INTERFACE_H__
#define __MEDIA_INTERFACE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <freertos/semphr.h>
#include <freertos/queue.h>

#define MEDIA_TITLE_LENGTH  127
#define MEDIA_ALBUM_LENGTH  127
#define MEDIA_ARTIST_LENGTH 127

typedef enum {
    MI_PLAY,
    MI_PAUSE,
    MI_STOP
} media_command_t;

typedef enum {
    MI_RETURN_OK            = 0x0000,
    MI_STOPPED_BY_REQUEST   = 0x0001,
    MI_END_OF_SONG          = 0x0002,

    MI_ERROR_PARAMETER      = 0x1000,
    MI_ERROR_NOT_SUPPORTED  = 0x1001,
    MI_ERROR_DECODE_ERROR   = 0x1002,
    MI_ERROR_INVALID_FORMAT = 0x1003,
    MI_ERROR_OUT_OF_MEMORY  = 0x1004
} media_status_t;

typedef struct {
    int32_t track_number;
    int32_t disc_number;
    uint8_t title[MEDIA_TITLE_LENGTH + 1];
    uint8_t album[MEDIA_ALBUM_LENGTH + 1];
    uint8_t artist[MEDIA_ARTIST_LENGTH + 1];

    double reference_loudness;
    double track_gain;
    double track_peak;
    double album_gain;
    double album_peak;
} media_metadata_t;

typedef void media_interface_t;

/**
 *  Called by the media playback to cause the current task to suspend while
 *  waiting for data.
 */
typedef void* (*media_malloc_fn_t)( const size_t size );

/**
 *  Called by the media playback during an ISR to wake up the task and
 *  alert the task that there is data available.
 */
typedef void (*media_free_fn_t)( void *ptr );

/**
 *  Used to command the decoder while it is busy decoding.
 *
 *  @note There may only be 1 instance of playback decoder
 *
 *  @param cmd the command to apply to the current decoder
 *
 *  @return the status of the request
 */
typedef media_status_t (*media_command_fn_t)( const media_command_t cmd );

/**
 *  The media decoder function.
 *
 *  @note There may only be 1 instance of playback decoder
 *
 *  @param file the open file to play
 *  @param suspend the function to call to suspend the thread
 *  @param resume the function to call to resume the thread from an ISR
 *
 *  @return the status of the decode
 */
typedef media_status_t (*media_play_fn_t)( const char *filename,
                                           const double gain,
                                           const double peak,
                                           xQueueHandle idle,
                                           const size_t queue_size,
                                           media_malloc_fn_t malloc_fn,
                                           media_free_fn_t free_fn );

/**
 *  Used to determine if a file is if a particular media type.
 *
 *  @param filename the filename to test
 *
 *  @return true if the file is of this media type, false otherwise
 */
typedef bool (*media_get_type_fn_t)( const char *filename );

/**
 *  Used to harvest the metatdata from a file.
 *
 *  @param file the file to harvest metadata from
 *  @param metadata the pointer to the structure to populate
 *
 *  @return the status of the metadata request
 */
typedef media_status_t (*media_get_metadata_fn_t)( const char *filename,
                                                   media_metadata_t *metadata );

/**
 *  Used to initialize the media interface.
 *
 *  @return the initialized media_interface_t object
 */
media_interface_t* media_new( void );

/**
 *  @param interface pointer to the interface list pointer
 *  @param filename pointer to the string of the file name.
 *         Must be '\0' terminated.  If the extension of the
 *         filename will be used to determine which media
 *         interface to use.
 *  @param file pointer to the EmbeddedFile structure
 *  @param metadata pointer to location where metadata from
 *         a successful media tag can be placed
 *  @param command_fn pointer to the command function for this
 *         codec (Pause/Play/etc)
 *  @param play_fn pointer to the play function which starts
 *         a song playing
 */
media_status_t media_get_information( media_interface_t *interface,
                                      const char *filename,
                                      media_metadata_t *metadata,
                                      media_command_fn_t *command_fn,
                                      media_play_fn_t *play_fn );

/**
 *  @param interface pointer to the interface list pointer
 *  @param name The name of the codec which is being registered
 *  @param command function which is be used by this codec for issuing
 *         commands (Pause/Play/etc)
 *  @param play function which is to be used by this coded for starting a song
 *  @param get_type function which will used to see if a filename is supported
 *         by this codec
 *  @param get_metadata function which will get the metadata for this codec
 */
media_status_t media_register_codec( media_interface_t *interface,
                                     const char *name,
                                     media_command_fn_t command,
                                     media_play_fn_t play,
                                     media_get_type_fn_t get_type,
                                     media_get_metadata_fn_t get_metadata );

/**
 *  Used to remove all the regestered codecs & free any associated memory.
 *
 *  @param media the media interface instance to destroy
 *
 *  @retval MI_ERROR_PARAMETER
 *          MI_RETURN_OK
 */
media_status_t media_delete( media_interface_t *interface );
#endif
