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
#include <file-stream/file-stream.h>

#include "media-flac.h"
#include "decoder.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define BUFFER_COUNT    20
#define FILL_TO         10

#define MIN(a, b)   ((a) < (b)) ? (a) : (b)

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    abdac_node_t nodes[BUFFER_COUNT];
    uint8_t      buffers[BUFFER_COUNT][MAX_BLOCKSIZE*4];    /* stereo + 16bit */
    int32_t      decode_0[MAX_BLOCKSIZE];
    int32_t      decode_1[MAX_BLOCKSIZE];
} flac_data_t;

typedef enum {
    FLAC__STREAMINFO     = 0,
    FLAC__PADDING        = 1,
    FLAC__APPLICATION    = 2,
    FLAC__SEEKTABLE      = 3,
    FLAC__VORBIS_COMMENT = 4,
    FLAC__CUESHEET       = 5,
    FLAC__PICTURE        = 6,
    FLAC__INVALID        = 7,
    FLAC__SIZE           = 8
} flac_block_type_t;

typedef media_status_t (*block_handler_t)( FLACContext *fc,
                                           const uint32_t length,
                                           media_metadata_t *metadata );

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static volatile ll_list_t __idle;
static volatile bool __done;
static media_resume_fn_t __resume;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static media_status_t process_file( FLACContext *fc,
                                    int32_t *decode_0,
                                    int32_t *decode_1,
                                    media_suspend_fn_t suspend );
static media_status_t process_metadata( FLACContext *fc,
                                        media_metadata_t *metadata );
static void process_metadata_block_header( uint8_t *header,
                                           bool *last,
                                           flac_block_type_t *type,
                                           uint32_t *length );
static media_status_t metadata_block_ignore( FLACContext *fc,
                                             const uint32_t length,
                                             media_metadata_t *metadata );
static media_status_t metadata_streaminfo_block( FLACContext *fc,
                                                 const uint32_t length,
                                                 media_metadata_t *metadata );
static media_status_t metadata_vorbis_comment( FLACContext *fc,
                                               const uint32_t length,
                                               media_metadata_t *metadata );
static media_status_t play_song( FLACContext *fc,
                                 int32_t *decode_0,
                                 int32_t *decode_1,
                                 media_suspend_fn_t suspend );
static void __buffer_done( abdac_node_t *node, const bool last );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/** See media-interface.h for details. */
media_status_t media_flac_command( const media_command_t cmd )
{
    return MI_ERROR_NOT_SUPPORTED;
}

/** See media-interface.h for details. */
media_status_t media_flac_play( const char *filename,
                                media_suspend_fn_t suspend,
                                media_resume_fn_t resume )
{
    FLACContext  fc;
    media_status_t rv;
    int32_t i;
    flac_data_t *d;

    rv = MI_RETURN_OK;

    if( (NULL == filename) || (NULL == suspend) || (NULL == resume) ) {
        rv = MI_ERROR_PARAMETER;
        goto error_0;
    }

    __resume = resume;

    if( true != fstream_open(filename) ) {
        rv = MI_ERROR_INVALID_FORMAT;
        goto error_0;
    }

    {
        uint8_t *buffer;
        size_t got;
        buffer = fstream_get_buffer( 4, &got );
        if( (4 != got) || (0 != memcmp(buffer, "fLaC", 4)) )
        {
            rv = MI_ERROR_INVALID_FORMAT;
            fstream_release_buffer( 0 );
            goto error_1;
        }
        fstream_release_buffer( 4 );
    }

    d = (flac_data_t*) malloc( sizeof(flac_data_t) );
    if( NULL == d ) {
        rv = MI_ERROR_OUT_OF_MEMORY;
        goto error_1;
    }

    /* Initialize the non-buffer sections of the memory allocation */
    memset( &fc, 0, sizeof(FLACContext) );

    /* From above we've already read 4 bytes of metadata */
    fc.filesize = fstream_get_filesize();
    fc.metadatalength = 4;

    ll_init_list( &__idle );

    for( i = 0; i < BUFFER_COUNT; i++ ) {
        ll_init_node( &d->nodes[i].node, &d->nodes[i] );
        d->nodes[i].buffer = d->buffers[i];
        ll_append( &__idle, &d->nodes[i].node );
    }

    rv = process_file( &fc, d->decode_0, d->decode_1, suspend );

    free( d );

error_1:

    fstream_close();

error_0:

    return rv;
}

/** See media-interface.h for details. */
bool media_flac_get_type( const char *filename )
{
    if( NULL != filename ) {
        size_t len = strlen( filename );

        if( 4 < len ) {
            if( (0 == strcasecmp("fla", &filename[len - 3])) ||
                (0 == strcasecmp("flac", &filename[len - 4])) )
            {
                return true;
            }
        }
    }

    return false;
}

/** See media-interface.h for details. */
media_status_t media_flac_get_metadata( const char *filename,
                                        media_metadata_t *metadata )
{
    FLACContext  fc;
    media_status_t rv;
    flac_data_t *d;

    rv = MI_RETURN_OK;

    if( (NULL == filename) || (NULL == metadata) ) {
        rv = MI_ERROR_PARAMETER;
        goto error_0;
    }

    if( true != fstream_open(filename) ) {
        rv = MI_ERROR_PARAMETER;
        goto error_0;
    }

    {
        size_t got;
        uint8_t *buf;
        buf = (uint8_t*) fstream_get_buffer( 4, &got );
        if( (4 != got) || (0 != memcmp(buf, "fLaC", 4)) )
        {
            rv = MI_ERROR_INVALID_FORMAT;
            fstream_release_buffer( 0 );
            goto error_0;
        }
        fstream_release_buffer( 4 );
    }

    d = (flac_data_t*) malloc( sizeof(flac_data_t) );
    if( NULL == d ) {
        rv = MI_ERROR_OUT_OF_MEMORY;
        goto error_0;
    }

    /* Initialize the non-buffer sections of the memory allocation */
    memset( &fc, 0, sizeof(FLACContext) );

    /* From above we've already read 4 bytes of metadata */
    fc.filesize = fstream_get_filesize();
    fc.metadatalength = 4;

    rv = process_metadata( &fc, metadata );

    free( d );

error_0:

    fstream_close();

    return rv;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  Used to process the file after the basic error checking has been done.
 *
 *  @param fc the flac context data
 *  @param decode_0 the channel 0 decode buffer
 *  @param decode_1 the channel 1 decode buffer
 *
 *  @retval MI_RETURN_OK
 */
static media_status_t process_file( FLACContext *fc,
                                    int32_t *decode_0,
                                    int32_t *decode_1,
                                    media_suspend_fn_t suspend )
{
    media_status_t status;
    abdac_sample_rate_t sample_rate;

    status = process_metadata( fc, NULL );
    if( MI_RETURN_OK != status ) {
        return status;
    }

    switch( fc->samplerate ) {
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
            return MI_ERROR_NOT_SUPPORTED;
    }

    abdac_init( false, &__buffer_done );

    if( BSP_RETURN_OK != abdac_set_sample_rate(sample_rate) ) {
        return MI_ERROR_NOT_SUPPORTED;
    }

    abdac_queue_silence();

    __done = false;

    status = play_song( fc, decode_0, decode_1, suspend );

    while( false == __done ) {
        (*suspend)();
    }

    return status;
}

/**
 *  Used to process the file after the basic error checking has been done.
 *
 *  @param fc the flac context data
 *
 *  @retval MI_RETURN_OK
 */
static media_status_t process_metadata( FLACContext *fc,
                                        media_metadata_t *metadata )
{
    media_status_t status;
    bool end;
    bool found_stream_info;
    flac_block_type_t type;
    uint32_t block_length;

    block_handler_t handler[FLAC__SIZE];

    handler[FLAC__STREAMINFO]     = metadata_streaminfo_block;
    handler[FLAC__PADDING]        = metadata_block_ignore;
    handler[FLAC__APPLICATION]    = metadata_block_ignore;
    handler[FLAC__SEEKTABLE]      = metadata_block_ignore;
    handler[FLAC__VORBIS_COMMENT] = metadata_vorbis_comment;
    handler[FLAC__CUESHEET]       = metadata_block_ignore;
    handler[FLAC__PICTURE]        = metadata_block_ignore;
    handler[FLAC__INVALID]        = metadata_block_ignore;

    end = false;
    found_stream_info = false;

    while( !end ) {
        {
            size_t got;
            uint8_t *buf;
            buf = (uint8_t*) fstream_get_buffer( 4, &got );
            if( 4 != got ) {
                fstream_release_buffer( 0 );
                return MI_ERROR_DECODE_ERROR;
            }

            process_metadata_block_header( buf, &end, &type, &block_length );
            fstream_release_buffer( 4 );
        }

        status = (*handler[type])( fc, block_length, metadata );
        if( MI_RETURN_OK != status ) {
            return status;
        }
        if( FLAC__STREAMINFO == type ) {
            found_stream_info = true;
        }
        fc->metadatalength += block_length + 4;
    }

    if( false == found_stream_info ) {
        return MI_ERROR_DECODE_ERROR;
    }

    fc->bitrate = ((fc->filesize - fc->metadatalength) * 8) / fc->length;

    return MI_RETURN_OK;
}

/**
 *  Used to process a FLAC METADATA_BLOCK_HEADER into it's components.
 *
 *  @param header the header data to process
 *  @param last last-metadata-block flag returned
 *  @param type the BLOCK_TYPE
 *  @param length the length of the metadata to follow the header
 */
static void process_metadata_block_header( uint8_t *header,
                                           bool *last,
                                           flac_block_type_t *type,
                                           uint32_t *length )
{
    uint8_t tmp;

    tmp = header[0];

    /* 1 = last block (true), 0 = more blocks (false) */
    *last = (0x80 == (0x80 & tmp));

    tmp &= 0x7f;
    if( tmp < 7 ) {
        *type = (flac_block_type_t) tmp;
    } else {
        *type = FLAC__INVALID;
    }

    *length = (header[1] << 16) | (header[2] << 8) | header[3];
}

/**
 *  Skips over the dat in the block.
 *
 *  @param fc ignored
 *  @param length the number of bytes in the block
 *  @param buf ignored
 */
static media_status_t metadata_block_ignore( FLACContext *fc,
                                             const uint32_t length,
                                             media_metadata_t *metadata )
{
    fstream_skip( length );
    return MI_RETURN_OK;
}

/**
 *  Processes the data in a STREAMINFO block.
 *
 *  @param fc the FLAC context data
 *  @param length the number of bytes in the block
 *  @param buf the scratch buffer to read into
 */
static media_status_t metadata_streaminfo_block( FLACContext *fc,
                                                 const uint32_t length,
                                                 media_metadata_t *metadata )
{
    size_t got;
    uint8_t *buf;

    buf = (uint8_t*) fstream_get_buffer( length, &got );
    if( length != got ) {
        fstream_release_buffer( 0 );
        return MI_ERROR_DECODE_ERROR;
    }

    fc->min_blocksize = (buf[0] << 8)  | buf[1];
    fc->max_blocksize = (buf[2] << 8)  | buf[3];
    fc->min_framesize = (buf[4] << 16) | (buf[5] << 8)  | buf[6];
    fc->max_framesize = (buf[7] << 16) | (buf[8] << 8) | buf[9];
    fc->samplerate    = (buf[10] << 12) | (buf[11] << 4) | (buf[12] >> 4);
    fc->channels      = ((buf[12] >> 1) & 0x07) + 1;
    fc->bps           = (((0x01 & buf[12]) << 4) | (buf[13] >> 4) ) + 1; 

    /* totalsamples is a 36-bit field, but we assume <= 32 bits are 
     * used */
    if( 0 != (0x0f & buf[13]) ) {
        fstream_release_buffer( got );
        return MI_ERROR_DECODE_ERROR;
    }
    fc->totalsamples = (buf[14] << 24) | (buf[15] << 16) | (buf[16] << 8) | buf[17];
    fc->length = (fc->totalsamples / fc->samplerate) * 1000;
    fstream_release_buffer( got );

    return MI_RETURN_OK;
}

static media_status_t metadata_vorbis_comment( FLACContext *fc,
                                               const uint32_t length,
                                               media_metadata_t *metadata )
{
    if( NULL != metadata ) {
        int32_t i;
        size_t got;
        uint8_t *buf;
        
        buf = (uint8_t*) fstream_get_buffer( length, &got );
        if( length != got ) {
            fstream_release_buffer( 0 );
            return MI_ERROR_DECODE_ERROR;
        }

        memset( metadata, 0, sizeof(media_metadata_t) );
        metadata->track_number = -1;

        /* Skip the vendor information */
        {   uint32_t vendor_length;
            vendor_length = *buf;           buf++;
            vendor_length |= (*buf << 8);   buf++;
            vendor_length |= (*buf << 16);  buf++;
            vendor_length |= (*buf << 24);  buf++;
            buf += vendor_length;
        }

        i = *buf;           buf++;
        i |= (*buf << 8);   buf++;
        i |= (*buf << 16);  buf++;
        i |= (*buf << 24);  buf++;

        for( ; 0 < i; i-- ) {
            uint32_t comment_length;

            comment_length = *buf;          buf++;
            comment_length |= (*buf << 8);  buf++;
            comment_length |= (*buf << 16); buf++;
            comment_length |= (*buf << 24); buf++;

            if( (comment_length >  6) &&
                (0 == strncasecmp((char*)buf, "TITLE=", 6)) ) {
                memcpy( &metadata->title, &buf[6],
                        MIN(MEDIA_TITLE_LENGTH, (comment_length-6)) );
            } else if( (comment_length >  6) &&
                       (0 == strncasecmp((char*)buf, "ALBUM=", 6)) )
            {
                memcpy( &metadata->album, &buf[6],
                        MIN(MEDIA_ALBUM_LENGTH, (comment_length-6)) );
            } else if( (comment_length >  7) &&
                       (0 == strncasecmp((char*)buf, "ARTIST=", 7)) )
            {
                memcpy( &metadata->artist, &buf[7],
                        MIN(MEDIA_ARTIST_LENGTH, (comment_length-7)) );
            } else if( (comment_length > 12) &&
                       (0 == strncasecmp((char*)buf, "TRACKNUMBER=", 12)) )
            {
                int32_t i;
                metadata->track_number = 0;
                for( i = 12; i < comment_length; i++ ) {
                    if( ('0' <= buf[i]) && (buf[i] <= '9') ) {
                        metadata->track_number *= 10;
                        metadata->track_number += (buf[i] - '0');
                    } else {
                        fstream_release_buffer( got );
                        return MI_ERROR_DECODE_ERROR;
                    }
                }
            }
            buf += comment_length;
        }
        fstream_release_buffer( got );
    } else {
        return metadata_block_ignore( fc, length, metadata );
    }

    return MI_RETURN_OK;
}

static media_status_t play_song( FLACContext *fc,
                                 int32_t *decode_0,
                                 int32_t *decode_1,
                                 media_suspend_fn_t suspend )
{
    bool started_song;
    int32_t count;

    started_song = false;
    count = 0;

    while( 1 ) {
        bool isrs_enabled;
        ll_node_t *node;

        isrs_enabled = interrupts_save_and_disable();
        node = ll_remove_head( &__idle );
        interrupts_restore( isrs_enabled );

        if( NULL == node ) {
            (*suspend)();
        } else {
            int32_t consumed;
            abdac_node_t *n = (abdac_node_t *) node->data;
            uint8_t *read_buffer;
            int32_t bytes_left;

            read_buffer = (uint8_t*) fstream_get_buffer( fc->max_framesize, (size_t*) &bytes_left );

            if( 0 == bytes_left ) {
                fstream_release_buffer( 0 );
                memset( (void*) n->buffer, 0, 1000 );
                n->size = 1000;
                abdac_queue_data( n, true );
                return MI_END_OF_SONG;
            }

            if( 0 != flac_decode_frame(fc, (int32_t *) n->buffer, decode_1, read_buffer, bytes_left) ) {
                fstream_release_buffer( 0 );
                memset( (void*) n->buffer, 0, 1000 );
                n->size = 1000;
                abdac_queue_data( n, true );
                return MI_ERROR_DECODE_ERROR;
            }

            n->size = fc->blocksize * 4;

            abdac_queue_data( n, false );

            consumed = fc->gb.index / 8;

            fstream_release_buffer( consumed );

            count++;
            if( (false == started_song) && (FILL_TO == count) ) {
                abdac_start();
                started_song = true;
            }
        }
    }
}

static void __buffer_done( abdac_node_t *node, const bool last )
{
    ll_append( &__idle, &node->node );
    if( true == last ) {
        __done = true;
    }
    (*__resume)();
}
