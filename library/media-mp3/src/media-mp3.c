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
#include "decoder.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define MIN(a, b)   ((a) < (b)) ? (a) : (b)
#define NODE_COUNT  2

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    xQueueHandle idle;
} mp3_data_node_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static volatile media_command_t __cmd;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void dsp_callback( int32_t *left, int32_t *right, void *data );
static enum mad_flow mp3_input( void *user, struct mad_stream *stream );
static enum mad_flow mp3_header( void *user, struct mad_header const *header );
static enum mad_flow mp3_output( void *user,
                                 struct mad_header const *header,
                                 struct mad_pcm * pcm );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/** See media-interface.h for details. */
media_status_t media_mp3_command( const media_command_t cmd )
{
    if( (MI_PLAY == cmd) || (MI_STOP == cmd) ) {
        __cmd = cmd;
        return MI_RETURN_OK;
    }

    return MI_ERROR_NOT_SUPPORTED;
}

/** See media-interface.h for details. */
media_status_t media_mp3_play( const char *filename,
                               const double gain,
                               const double peak,
                               xQueueHandle idle,
                               const size_t queue_size,
                               media_malloc_fn_t malloc_fn,
                               media_free_fn_t free_fn )
{
    media_status_t rv;
    int32_t node_count;
    int32_t i;
    int32_t dsp_scale_factor;

    rv = MI_RETURN_OK;

    if( (NULL == filename) || (NULL == idle) || (0 == queue_size) ||
        (NULL == malloc_fn) || (NULL == free_fn) )
    {
        rv = MI_ERROR_PARAMETER;
        goto error_0;
    }

    dsp_scale_factor = dsp_determine_scale_factor( peak, gain );

    __cmd = MI_PLAY;

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

    /* Decode song */
#if 0
    rv = stream__process_file( &fc, idle, dsp_scale_factor );
#endif

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
                printf( "Yes to: '%s'\n", filename );
                return true;
            }
        }
    }

    printf( "No to: '%s'\n", filename );
    return false;
}

/** See media-interface.h for details. */
media_status_t media_mp3_get_metadata( const char *filename,
                                       media_metadata_t *metadata )
{
    static int32_t tn = 1;

    metadata->track_number = tn++;
    metadata->disc_number = 1;
    strcpy( metadata->title, "Fake Title" );
    strcpy( metadata->album, "Fake Album" );
    strcpy( metadata->artist, "Fake Artist" );
    metadata->reference_loudness = 0.0;
    metadata->track_gain = 0.0;
    metadata->track_peak = 0.0;
    metadata->album_gain = 0.0;
    metadata->album_peak = 0.0;

    return MI_RETURN_OK;
    //return MI_ERROR_INVALID_FORMAT;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

static void dsp_callback( int32_t *left, int32_t *right, void *data )
{
    mp3_data_node_t *node = (mp3_data_node_t*) data;

    //printf( "%s() : 0x%08x\n", __func__, node->idle );
    xQueueSendToBack( node->idle, &node, 0 );
}
