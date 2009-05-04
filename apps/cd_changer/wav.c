/*
 * Copyright (c) 2008  Weston Schmidt
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

#include <linked-list/linked-list.h>
#include <bsp/abdac.h>
#include <freertos/task.h>

#include "wav.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define WAV_BUFFER_SIZE     4096
#define WAV_BUFFER_COUNT    320
#define LITTLE_TO_BIG32( x )    { x = __builtin_bswap_32( x ); }
#define LITTLE_TO_BIG16( x )    { x = __builtin_bswap_16( x ); }

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    char id[4];
    uint32_t size;
} subchunk_header_t;

typedef struct {
    subchunk_header_t header;
    uint32_t type;
} riff_header_t;

typedef struct {
    subchunk_header_t header;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} format_header_t;

typedef struct {
    riff_header_t     riff;
    format_header_t   fmt;
    subchunk_header_t data;
} normal_wav_header_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static volatile ll_list_t __wav_idle;
static volatile bool __wave_done;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static int32_t __process_wav_info( EmbeddedFile *file,
                                   abdac_sample_rate_t *sample_rate,
                                   uint32_t *delay );
static uint16_t __read_and_queue( EmbeddedFile *file, abdac_node_t *node );
static void __buffer_done( abdac_node_t *node, const bool last );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See wav.h for details. */
void wav_play( EmbeddedFile *file )
{
    int32_t i;
    bool eof;
    abdac_node_t nodes[WAV_BUFFER_COUNT];
    //uint8_t __buffers[WAV_BUFFER_COUNT*WAV_BUFFER_SIZE];
    uint8_t *buffers;
    ll_node_t *node;
    uint32_t delay;
    abdac_sample_rate_t sample_rate;

    buffers = (uint8_t*) malloc(WAV_BUFFER_COUNT*WAV_BUFFER_SIZE); //__buffers;
    //buffers = __buffers;

    __wave_done = false;
    ll_init_list( &__wav_idle );

    if( NULL == file ) {
        return;
    }

    for( i = 0; i < WAV_BUFFER_COUNT; i++ ) {
        ll_init_node( &nodes[i].node, &nodes[i] );
        nodes[i].buffer = &buffers[i*WAV_BUFFER_SIZE];
        ll_append( &__wav_idle, &nodes[i].node );
    }

    if( 0 != __process_wav_info(file, &sample_rate, &delay) ) {
        return;
    }

    abdac_set_sample_rate( sample_rate );

    eof = false;
    abdac_queue_silence();

    while( NULL != (node = ll_remove_head(&__wav_idle)) ) {
        uint16_t size;

        size = __read_and_queue( file, (abdac_node_t *) node->data );

        if( size < WAV_BUFFER_SIZE ) {
            eof = true;
            break;
        }
    }

    abdac_start();

    while( false == eof ) {
        bool isrs_enabled;

        interrupts_save_and_disable( isrs_enabled );
        node = ll_remove_head( &__wav_idle );
        interrupts_restore( isrs_enabled );

        if( NULL == node ) {
            if( 0 < delay ) {
                vTaskDelay( delay );
            }
        } else {
            uint16_t size;

            size = __read_and_queue( file, (abdac_node_t *) node->data );
            if( size < WAV_BUFFER_SIZE ) {
                eof = true;
            }
        }
    }

    /* Wait until the song is complete. */
    while( false == __wave_done ) {
        vTaskDelay( 100 );
    }
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static int32_t __process_wav_info( EmbeddedFile *file,
                                   abdac_sample_rate_t *sample_rate,
                                   uint32_t *delay )
{
    normal_wav_header_t info;

    if( sizeof(normal_wav_header_t) !=
            file_read(file, sizeof(normal_wav_header_t), (uint8_t*) &info) )
    {
        return -1;
    }

    LITTLE_TO_BIG32( info.riff.header.size );
    LITTLE_TO_BIG32( info.riff.type );
    LITTLE_TO_BIG32( info.fmt.header.size );
    LITTLE_TO_BIG16( info.fmt.audio_format );
    LITTLE_TO_BIG16( info.fmt.num_channels );
    LITTLE_TO_BIG32( info.fmt.sample_rate );
    LITTLE_TO_BIG32( info.fmt.byte_rate );
    LITTLE_TO_BIG16( info.fmt.block_align );
    LITTLE_TO_BIG16( info.fmt.bits_per_sample );
    LITTLE_TO_BIG32( info.data.size );

    if( ('R' != info.riff.header.id[0]) ||
        ('I' != info.riff.header.id[1]) ||
        ('F' != info.riff.header.id[2]) ||
        ('F' != info.riff.header.id[3]) ||
        ('f' != info.fmt.header.id[0]) ||
        ('m' != info.fmt.header.id[1]) ||
        ('t' != info.fmt.header.id[2]) ||
        (' ' != info.fmt.header.id[3]) ||
        ('d' != info.data.id[0]) ||
        ('a' != info.data.id[1]) ||
        ('t' != info.data.id[2]) ||
        ('a' != info.data.id[3]) ||
        (1 != info.fmt.audio_format) ||
        (2 != info.fmt.num_channels) ||
        (16 != info.fmt.bits_per_sample) )
    {
        return -2;
    }

    printf( "----------------------------------------\n" );
    printf( "riff.header.  id: '%.4s'\n", info.riff.header.id );
    printf( "riff.header.size: %lu\n", info.riff.header.size );
    printf( "riff.type: 0x%08lx\n", info.riff.type );

    printf( "----------------------------------------\n" );
    printf( "fmt.header.  id: '%.4s'\n", info.fmt.header.id );
    printf( "fmt.header.size: %lu\n", info.fmt.header.size );
    printf( "fmt.   audio_format: 0x%04x\n", info.fmt.audio_format );
    printf( "fmt.   num_channels: %u\n", info.fmt.num_channels );
    printf( "fmt.    sample_rate: %lu\n", info.fmt.sample_rate );
    printf( "fmt.      byte_rate: %lu\n", info.fmt.byte_rate );
    printf( "fmt.    block_align: 0x%04x\n", info.fmt.block_align );
    printf( "fmt.bits_per_sample: %u\n", info.fmt.bits_per_sample );

    printf( "----------------------------------------\n" );
    printf( "data.header.  id: '%.4s'\n", info.data.id );
    printf( "data.header.size: %lu\n", info.data.size );

    *delay = 1;  /* Max ms until at least 1 buffer is free */

    abdac_init( false, &__buffer_done );

    switch( info.fmt.sample_rate ) {
        case 48000: *sample_rate = ABDAC_SAMPLE_RATE__48000; break;
        case 44100: *sample_rate = ABDAC_SAMPLE_RATE__44100; break;
        case 32000: *sample_rate = ABDAC_SAMPLE_RATE__32000; break;
        case 24000: *sample_rate = ABDAC_SAMPLE_RATE__24000; break;
        case 22050: *sample_rate = ABDAC_SAMPLE_RATE__22050; break;
        case 16000: *sample_rate = ABDAC_SAMPLE_RATE__16000; break;
        case 12000: *sample_rate = ABDAC_SAMPLE_RATE__12000; break;
        case 11025: *sample_rate = ABDAC_SAMPLE_RATE__11025; break;
        case 8000:  *sample_rate = ABDAC_SAMPLE_RATE__8000;  break;
        default:
            return -3;
    }

    return 0;
}

/**
 *  Used to read data from the file into the node & then queue the node into
 *  the correct queue, which is based on if we were able to queue the node
 *  in the ABDAC.
 *
 *  @param file the file to read from
 *  @param node the node to read into
 *
 *  @return the number of bytes read
 */
static uint16_t __read_and_queue( EmbeddedFile *file, abdac_node_t *node )
{
    uint16_t rv;
    uint16_t *sample;
    int i;

    rv = (uint16_t) file_read( file, WAV_BUFFER_SIZE, (uint8_t*) node->buffer );
    node->size = rv;

    sample = (uint16_t *) node->buffer;

    for( i = rv/2; 0 <= i; --i ) {
        LITTLE_TO_BIG16( *sample );
        sample++;
    }

    abdac_queue_data( node, (WAV_BUFFER_SIZE != node->size) );

    return rv;
}

/**
 *  Used to handle buffers that have been consumed.
 *
 *  @param node the buffer of the node that has been consumed
 */
static void __buffer_done( abdac_node_t *node, const bool last )
{
    ll_append( &__wav_idle, &node->node );
    if( true == last ) {
        __wave_done = true;
    }
}
