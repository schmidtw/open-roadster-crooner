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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dsp/dsp.h>
#include <file-stream/file-stream.h>
#include <fatfs/ff.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

#include "media-mp3.h"

#include "id3.h"

#include "stream.h"
#include "synth.h"
#include "frame.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define MIN(a, b)   ((a) < (b)) ? (a) : (b)
#define NODE_COUNT  2

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    int32_t left[1152];
    int32_t right[1152];
    xQueueHandle idle;
} mp3_data_node_t;

typedef struct {
    struct mad_frame frame;
    struct mad_synth synth;
} mp3_data_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void dsp_callback( int32_t *left, int32_t *right, void *data );
static media_status_t decode_song( xQueueHandle idle,
                                   const int32_t gain,
                                   mp3_data_t *data,
                                   media_command_fn_t command_fn );
static media_status_t output_data( xQueueHandle idle,
                                   const struct mad_pcm *pcm,
                                   const int32_t gain,
                                   const uint32_t bitrate );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/** See media-interface.h for details. */
media_status_t media_mp3_play( const char *filename,
                               const double gain,
                               const double peak,
                               xQueueHandle idle,
                               const size_t queue_size,
                               media_malloc_fn_t malloc_fn,
                               media_free_fn_t free_fn,
                               media_command_fn_t command_fn )
{
    media_status_t rv;
    mp3_data_t *data;
    int32_t node_count;
    int32_t i;
    int32_t dsp_scale_factor;

    rv = MI_RETURN_OK;

    if( (NULL == filename) || (NULL == idle) || (0 == queue_size) ||
        (NULL == malloc_fn) || (NULL == free_fn) || (NULL == command_fn) )
    {
        rv = MI_ERROR_PARAMETER;
        goto error_0;
    }

    dsp_scale_factor = dsp_determine_scale_factor( peak, gain );

    if( true != fstream_open(filename) ) {
        rv = MI_ERROR_INVALID_FORMAT;
        goto error_0;
    }

    node_count = MIN( queue_size, NODE_COUNT );
    i = node_count;
    while( 0 < i-- ) {
        mp3_data_node_t *node;

        node = (mp3_data_node_t*) (*malloc_fn)( sizeof(mp3_data_node_t) );
        if( NULL == node ) {
            rv = MI_ERROR_OUT_OF_MEMORY;
            goto error_1;
        }
        node->idle = idle;
        xQueueSendToBack( idle, &node, 0 );
    }
    i = node_count;

    data = (mp3_data_t *) (*malloc_fn)( sizeof(mp3_data_t) );
    if( NULL == data ) {
        goto error_1;
    }

    /* Decode song */
    rv = decode_song( idle, dsp_scale_factor, data, command_fn );

    (*free_fn)( data );

error_1:

    while( 0 < i-- ) {
        mp3_data_node_t *node;

        xQueueReceive( idle, &node, portMAX_DELAY );
        (*free_fn)( node );
    }

    fstream_close();

error_0:

    return rv;
}

/** See media-interface.h for details. */
bool media_mp3_get_type( const char *filename )
{
    if( NULL != filename ) {
        size_t len = strlen( filename );

        if( 3 < len ) {
            if( 0 == strcasecmp("mp3", &filename[len - 3]) ) {
                return true;
            }
        }
    }

    return false;
}

/** See media-interface.h for details. */
media_status_t media_mp3_get_metadata( const char *filename,
                                       media_metadata_t *metadata )
{
    media_status_t rv;
    struct mp3entry entry;
    FIL file;

    rv = MI_RETURN_OK;

    if( (NULL == filename) || (NULL == metadata) ) {
        rv = MI_ERROR_PARAMETER;
        goto error_0;
    }

    if( FR_OK != f_open(&file, filename, FA_READ|FA_OPEN_EXISTING) ) {
        rv = MI_ERROR_PARAMETER;
        goto error_0;
    }

    get_mp3_metadata( &file, &entry );

    metadata->track_number = entry.tracknum;
    metadata->disc_number = entry.discnum;

    if( NULL != entry.title ) {
        strncpy( (char*) metadata->title, entry.title, MEDIA_TITLE_LENGTH );
    } else {
        metadata->title[0] = '\0';
    }
    metadata->title[MEDIA_TITLE_LENGTH] = '\0';

    if( NULL != entry.album ) {
        strncpy( (char*) metadata->album, entry.album, MEDIA_ALBUM_LENGTH );
    } else {
        metadata->album[0] = '\0';
    }
    metadata->album[MEDIA_ALBUM_LENGTH] = '\0';

    if( NULL != entry.artist ) {
        strncpy( (char*) metadata->artist, entry.artist, MEDIA_ARTIST_LENGTH  );
    } else {
        metadata->artist[0] = '\0';
    }
    metadata->artist[MEDIA_ARTIST_LENGTH] = '\0';

    /* Not supported for now. */
    metadata->reference_loudness = 0.0;
    metadata->track_gain = entry.track_gain;
    metadata->track_peak = entry.track_peak;
    metadata->album_gain = entry.album_gain;
    metadata->album_peak = entry.album_peak;

    f_close( &file );
error_0:
    return rv;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

static void dsp_callback( int32_t *left, int32_t *right, void *data )
{
    mp3_data_node_t *node = (mp3_data_node_t*) data;

    xQueueSendToBack( node->idle, &node, 0 );
}

static void print_stream( struct mad_stream *stream, int line )
{
        printf( "- %d -----------------------------\n", line );
        printf( "stream->buffer: %p\n", stream->buffer );
        printf( "stream->bufend: %p\n", stream->bufend );
        printf( "stream->skiplen: %lu\n", stream->skiplen );
        printf( "stream->sync: %d\n", stream->sync );
        printf( "stream->freerate: %lu\n", stream->freerate );
        printf( "stream->this_frame: %p\n", stream->this_frame );
        printf( "stream->next_frame: %p\n", stream->next_frame );
        printf( "stream->md_len: %u\n", stream->md_len );
        printf( "stream->options: %d\n", stream->options );
        printf( "stream->error: 0x%08x\n", stream->error );
        printf( "------------------------------\n" );
}

static media_status_t decode_song( xQueueHandle idle,
                                   const int32_t gain,
                                   mp3_data_t *data,
                                   media_command_fn_t command_fn )
{
    media_status_t rv;

    struct mad_stream stream;   /* sizeof() = 64 */
    //struct mad_frame *frame;    /* sizeof() = 9268 */
    //struct mad_synth *synth;    /* sizeof() = 8716 */

    mad_stream_init( &stream );
    mad_frame_init( &data->frame );
    mad_synth_init( &data->synth );

    rv = MI_RETURN_OK;
    while( MI_RETURN_OK == rv ) {
        uint8_t *buffer;
        size_t got;
        uint32_t get;

        if( false == (*command_fn)() ) {
            rv = MI_STOPPED_BY_REQUEST;
            goto early_exit;
        }

        get = 3072;

        buffer = (uint8_t *) fstream_get_buffer( get, &got );
        if( 0 == got ) {
            rv = MI_ERROR_DECODE_ERROR;
            fstream_release_buffer( 0 );
            goto error;
        }
        if( get != got ) {
            rv = MI_END_OF_SONG;
        }

        mad_stream_buffer( &stream, buffer, got );

        if( -1 == mad_frame_decode(&data->frame, &stream) ) {
            if( !MAD_RECOVERABLE(stream.error) ) {
                rv = MI_ERROR_DECODE_ERROR;
                fstream_release_buffer( 0 );
                goto error;
            }
        } else {
            mad_synth_frame( &data->synth, &data->frame );

            if( 0 < data->synth.pcm.length ) {
                rv = output_data( idle, &data->synth.pcm, gain,
                                  data->frame.header.samplerate );
            }
        }

        if( stream.next_frame == stream.this_frame ) {
            fstream_release_buffer( got );
        } else {
            fstream_release_buffer( stream.next_frame - stream.this_frame );
        }
    }

early_exit:
error:

    dsp_data_complete( NULL, NULL );

    mad_synth_finish( &data->synth );
    mad_frame_finish( &data->frame );
    mad_stream_finish( &stream );

    return rv;
}

static media_status_t output_data( xQueueHandle idle,
                                   const struct mad_pcm *pcm,
                                   const int32_t gain,
                                   const uint32_t bitrate )
{
    dsp_status_t status;
    mp3_data_node_t *node;
    int i;

    xQueueReceive( idle, &node, portMAX_DELAY );

    for( i = 0; i < pcm->length; i++ ) {
        node->left[i] = ((int32_t) pcm->samples[0][i]);
        node->right[i] = ((int32_t) pcm->samples[1][i]);
    }

    status = dsp_queue_data( node->left, node->right, pcm->length,
                             bitrate, gain, &dsp_callback, node );
    if( DSP_RETURN_OK != status ) {
        xQueueSendToBack( idle, &node, 0 );
        return MI_ERROR_DECODE_ERROR;
    }
    return MI_RETURN_OK;
}
