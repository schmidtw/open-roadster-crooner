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
#include <string.h>

#include <bsp/abdac.h>
#include <bsp/intc.h>
#include <linked-list/linked-list.h>

#include "media-mp3.h"
#include "decoder.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define BUFFER_COUNT        10
#define BUFFER_SIZE         4608
#define INPUT_BUFFER_SIZE   (512*6)
#define SAMPLE_COUNT        (BUFFER_SIZE/4)

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;
    uint8_t input_buffer[INPUT_BUFFER_SIZE];

    abdac_node_t nodes[BUFFER_COUNT];
    uint8_t      buffers[BUFFER_COUNT][BUFFER_SIZE];
} mp3_data_t;

typedef struct {
    EmbeddedFile *file;
    bool setup;
    bool dac_started;
} mp3_playback_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static volatile ll_list_t __idle;
static volatile bool __done;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static enum mad_flow header_fn( void *data, struct mad_header const *header );
static enum mad_flow input( void *data, unsigned char *buffer,
                            unsigned int *size );
static enum mad_flow output( void *data, struct mad_header const *header,
                             struct mad_pcm *pcm );
static void __buffer_done( abdac_node_t *node, const bool last );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/** See media-interface.h for details. */
media_status_t media_mp3_command( const media_command_t cmd )
{
    return MI_ERROR_NOT_SUPPORTED;
}

/** See media-interface.h for details. */
media_status_t media_mp3_play( EmbeddedFile *file )
{
    media_status_t rv;
    int32_t i, result;
    mp3_data_t *d;
    mp3_playback_t pb;
    bool last;

    rv = MI_RETURN_OK;

    if( NULL == file ) {
        rv = MI_ERROR_PARAMETER;
        goto error_0;
    }

    d = (mp3_data_t*) malloc( sizeof(mp3_data_t) );
    if( NULL == d ) {
        rv = MI_ERROR_OUT_OF_MEMORY;
        goto error_0;
    }

    for( i = 0; i < BUFFER_COUNT; i++ ) {
        ll_init_node( &d->nodes[i].node, &d->nodes[i] );
        d->nodes[i].buffer = d->buffers[i];
        ll_append( &__idle, &d->nodes[i].node );
    }

    pb.file = file;
    pb.setup = false;
    pb.dac_started = false;

    result = mad_decoder_run( &pb, input, header_fn, NULL, output, NULL,
                              &d->stream, &d->frame, &d->synth,
                              (unsigned char *) d->input_buffer,
                              INPUT_BUFFER_SIZE, 0 );

    last = false;

    while( false == __done ) {
        if( false == last ) {
            bool isrs_enabled;
            ll_node_t *node;

            interrupts_save_and_disable( isrs_enabled );
            node = ll_remove_head( &__idle );
            interrupts_restore( isrs_enabled );

            if( NULL != node ) {
                abdac_node_t *n = (abdac_node_t *) node->data;
                n->buffer[0] = 0;
                n->buffer[1] = 0;
                n->buffer[2] = 0;
                n->buffer[3] = 0;
                n->size = 4;
                last = true;
                abdac_queue_data( n, true );
            }
        }
    }

    free( d );

error_0:

    return rv;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  Used to handle the first header of the mp3 data, set the bitrate, and
 *  queue the starting silence.
 *
 *  @param data our data pointer for use
 *  @param header the header information to use
 *
 *  @retval MAD_FLOW_CONTINUE on success
 *  @retval MAD_FLOW_BREAK if there is an error
 */
static enum mad_flow header_fn( void *data, struct mad_header const *header )
{
    mp3_playback_t *pb = (mp3_playback_t *) data;
    abdac_sample_rate_t sample_rate;

    if( true == pb->setup ) {
        return MAD_FLOW_CONTINUE;
    }

    switch( header->samplerate ) {
        case 48000: sample_rate = ABDAC_SAMPLE_RATE__48000; break;
        case 44100: sample_rate = ABDAC_SAMPLE_RATE__44100; break;
        case 32000: sample_rate = ABDAC_SAMPLE_RATE__32000; break;
        case 24000: sample_rate = ABDAC_SAMPLE_RATE__24000; break;
        case 22050: sample_rate = ABDAC_SAMPLE_RATE__22050; break;
        case 16000: sample_rate = ABDAC_SAMPLE_RATE__16000; break;
        case 12000: sample_rate = ABDAC_SAMPLE_RATE__12000; break;
        case 11025: sample_rate = ABDAC_SAMPLE_RATE__11025; break;
        case 800:   sample_rate = ABDAC_SAMPLE_RATE__8000;  break;
        default:
            return MAD_FLOW_BREAK;
    }

    abdac_init( false, &__buffer_done );

    if( BSP_RETURN_OK != abdac_set_sample_rate(sample_rate) ) {
        return MAD_FLOW_BREAK;
    }

    abdac_queue_silence();

    pb->setup = true;
    __done = false;

    return MAD_FLOW_CONTINUE;
}

/**
 *  Used to get input data for the mp3 decoder.
 *
 *  @param data our data pointer for use
 *  @param buffer the data buffer to fill
 *  @param size the size of the buffer we can fill, when incoming, the number of
 *              new bytes read outgoing
 *
 *  @retval MAD_FLOW_CONTINUE on success
 *  @retval MAD_FLOW_STOP if there is no more data
 */
static enum mad_flow input( void *data, unsigned char *buffer, unsigned int *size )
{
    mp3_playback_t *pb = (mp3_playback_t *) data;
    uint32_t read;

    read = file_read( pb->file, *size, buffer );

    *size = read;
    if( 0 == *size ) {
        if( 0 != file_eof(pb->file) ) {
            return MAD_FLOW_STOP;
        } else {
            return MAD_FLOW_BREAK;
        }
    }

    return MAD_FLOW_CONTINUE;
}

/**
 *  Used to scale the output data for the DAC.
 *
 *  @param sample the sample to scale
 *
 *  @return the 16 bit scaled sample value
 */
static inline int16_t scale( mad_fixed_t sample )
{
    /* Round down to 16 bits. */
    sample += (1L << (MAD_F_FRACBITS - 16));

    /* Clip down to 16 bits. */
    if( sample >= MAD_F_ONE ) {
        sample = MAD_F_ONE - 1;
    } else if( sample < -MAD_F_ONE ) {
        sample = -MAD_F_ONE;
    }

    /* Quantize */
    return (int16_t) (sample >> (MAD_F_FRACBITS + 1 - 16));
}

/**
 *  Used to output the data from the mp3 decoder.
 *
 *  @param data our data pointer for use
 *  @param header the header information about this audio frame
 *  @param pcm the audio data to output to the DAC
 *
 *  @retval MAD_FLOW_CONTINUE always
 */
static enum mad_flow output( void *data, struct mad_header const *header,
                             struct mad_pcm *pcm )
{
    mp3_playback_t *pb = (mp3_playback_t *) data;
    uint32_t samples;
    ll_node_t *node;

    node = NULL;
    samples = pcm->length;

    while( (NULL == node) && (0 < samples) ) {
        bool isrs_enabled;
        bool possibly_start_dac;

        possibly_start_dac = false;

        interrupts_save_and_disable( isrs_enabled );
        node = ll_remove_head( &__idle );
        if( NULL == __idle.head ) {
            possibly_start_dac = true;
        }
        interrupts_restore( isrs_enabled );

        if( (true == possibly_start_dac) && (false == pb->dac_started) ) {
            pb->dac_started = true;
            abdac_start();
        }

        if( NULL != node ) {
            abdac_node_t *n;
            int16_t *d;
            uint32_t count;
            mad_fixed_t const *left_ch, *right_ch;

            n = (abdac_node_t *) node->data;

            d = (int16_t *) n->buffer;
            left_ch  = pcm->samples[0];
            right_ch = pcm->samples[1];

            count = samples;
            if( SAMPLE_COUNT < count ) {
                count = SAMPLE_COUNT;
            }

            n->size = count * 4;
            samples -= count;

            if( 2 == pcm->channels ) {
                while( count-- ) {
                    *d++ = scale( *left_ch++ );
                    *d++ = scale( *right_ch++ );
                }
            } else {    /* Mono */
                int16_t mono;
                while( count-- ) {
                    mono = scale( *left_ch++ );
                    *d++ = mono;
                    *d++ = mono;
                }
            }

            abdac_queue_data( n, false );
        }

        node = NULL;
    }

    return MAD_FLOW_CONTINUE;
}

/**
 *  Used to move the nodes from the 'playing' queue into
 *  our local idle queue & indicate the end of the audio playback.
 *
 *  @param node the node that is now idle
 *  @param last if this is the last node in the playback queue
 */
static void __buffer_done( abdac_node_t *node, const bool last )
{
    ll_append( &__idle, &node->node );
    if( true == last ) {
        __done = true;
    }
}
