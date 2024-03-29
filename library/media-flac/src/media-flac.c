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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <dsp/dsp.h>
#include <file-stream/file-stream.h>
#include <freertos/os.h>

#include "media-flac.h"
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
    int32_t decode_0[MAX_BLOCKSIZE];
    int32_t decode_1[MAX_BLOCKSIZE];
    queue_handle_t idle;
} flac_data_node_t;

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

typedef enum {
    FM__ALBUM,
    FM__ARTIST,
    FM__DISCNUMBER,
    FM__TITLE,
    FM__TRACKNUMBER,
    FM__REPLAYGAIN_ALBUM_PEAK,
    FM__REPLAYGAIN_ALBUM_GAIN,
    FM__REPLAYGAIN_TRACK_PEAK,
    FM__REPLAYGAIN_TRACK_GAIN,
    FM__REPLAYGAIN_REFERENCE_LOUDNESS,
    FM__UNKNOWN
} flac_metadata_t;

typedef media_status_t (*stream__block_handler_t)( FLACContext *fc,
                                                   const uint32_t length );
typedef media_status_t (*file__block_handler_t)( int fd,
                                                 const uint32_t length,
                                                 media_metadata_t *metadata );

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static media_status_t play_song( FLACContext *fc,
                                 queue_handle_t idle,
                                 const int32_t gain,
                                 media_command_fn_t command_fn );
static void process_metadata_block_header( uint8_t *header,
                                           bool *last,
                                           flac_block_type_t *type,
                                           uint32_t *length );

/*---------- Stream based metadata handlers ----------*/
static media_status_t stream__process_file( FLACContext *fc,
                                            queue_handle_t idle,
                                            const int32_t gain,
                                            media_command_fn_t command_fn );
static media_status_t stream__process_metadata( FLACContext *fc );
static media_status_t stream__metadata_block_ignore( FLACContext *fc,
                                                     const uint32_t length );
static media_status_t stream__metadata_streaminfo_block( FLACContext *fc,
                                                         const uint32_t length );

/*---------- File based metadata handlers ----------*/
static media_status_t file__process_metadata( int fd,
                                              media_metadata_t *metadata );
static media_status_t file__metadata_vorbis_comment( int fd,
                                                     const uint32_t length,
                                                     media_metadata_t *metadata );
static media_status_t file__metadata_block_ignore( int fd,
                                                   const uint32_t length,
                                                   media_metadata_t *metadata );
static bool file__read_uint32_t( int fd, uint32_t *out );
static flac_metadata_t flac_get_key_value( int fd,
                                           uint32_t *len );
static bool flac_get_int32_t( int fd, uint32_t *len, int32_t *out );
static bool flac_get_double( int fd, uint32_t *len, double *out );
static void dsp_callback( int32_t *left, int32_t *right, void *data );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/** See media-interface.h for details. */
media_status_t media_flac_play( const char *filename,
                                const double gain,
                                const double peak,
                                queue_handle_t idle,
                                const size_t queue_size,
                                media_malloc_fn_t malloc_fn,
                                media_free_fn_t free_fn,
                                media_command_fn_t command_fn )
{
    FLACContext  fc;
    media_status_t rv;
    int32_t node_count;
    int32_t i = 0;
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

    node_count = MIN( queue_size, NODE_COUNT );
    i = node_count;
    while( 0 < i-- ) {
        flac_data_node_t *node;

        node = (flac_data_node_t*) (*malloc_fn)( sizeof(flac_data_node_t) );
        if( NULL == node ) {
            rv = MI_ERROR_OUT_OF_MEMORY;
            goto error_1;
        }
        node->idle = idle;
        os_queue_send_to_back( idle, &node, NO_WAIT );
    }
    i = node_count;

    /* Initialize the FLACContext data */
    memset( &fc, 0, sizeof(FLACContext) );

    /* From above we've already read 4 bytes of metadata */
    fc.filesize = fstream_get_filesize();
    fc.metadatalength = 4;

    rv = stream__process_file( &fc, idle, dsp_scale_factor, command_fn );

error_1:

    while( 0 < i-- ) {
        flac_data_node_t *node;

        os_queue_receive( idle, &node, WAIT_FOREVER );
        (*free_fn)( node );
    }

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
    media_status_t rv;
    int fd;

    rv = MI_RETURN_OK;

    if( (NULL == filename) || (NULL == metadata) ) {
        rv = MI_ERROR_PARAMETER;
        goto error_0;
    }

    fd = open( filename, O_RDONLY );
    if( -1 == fd ) {
        rv = MI_ERROR_PARAMETER;
        goto error_0;
    }

    {
        uint8_t buf[4];
        if( 4 != read(fd, buf, 4) ) {
            rv = MI_ERROR_INVALID_FORMAT;
            goto error_1;
        }

        if( 0 != memcmp(buf, "fLaC", 4) ) {
            rv = MI_ERROR_INVALID_FORMAT;
            goto error_1;
        }
    }

    rv = file__process_metadata( fd, metadata );

error_1:
    close( fd );

error_0:

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
static media_status_t stream__process_file( FLACContext *fc,
                                            queue_handle_t idle,
                                            const int32_t gain,
                                            media_command_fn_t command_fn )
{
    media_status_t status;

    status = stream__process_metadata( fc );
    if( MI_RETURN_OK != status ) {
        return status;
    }

    status = play_song( fc, idle, gain, command_fn );

    return status;
}

static media_status_t stream__process_metadata( FLACContext *fc )
{
    media_status_t status;
    bool end;
    bool found_stream_info;
    flac_block_type_t type;
    uint32_t block_length;

    stream__block_handler_t handler[FLAC__SIZE];

    handler[FLAC__STREAMINFO]     = stream__metadata_streaminfo_block;
    handler[FLAC__PADDING]        = stream__metadata_block_ignore;
    handler[FLAC__APPLICATION]    = stream__metadata_block_ignore;
    handler[FLAC__SEEKTABLE]      = stream__metadata_block_ignore;
    handler[FLAC__VORBIS_COMMENT] = stream__metadata_block_ignore;
    handler[FLAC__CUESHEET]       = stream__metadata_block_ignore;
    handler[FLAC__PICTURE]        = stream__metadata_block_ignore;
    handler[FLAC__INVALID]        = stream__metadata_block_ignore;

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

        status = (*handler[type])( fc, block_length );
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
 *  Used to process the file after the basic error checking has been done.
 *
 *  @param fc the flac context data
 *
 *  @retval MI_RETURN_OK
 */
static media_status_t file__process_metadata( int fd,
                                              media_metadata_t *metadata )
{
    media_status_t status;
    bool end;
    bool found_stream_info;
    flac_block_type_t type;
    uint32_t block_length;

    file__block_handler_t handler[FLAC__SIZE];

    handler[FLAC__STREAMINFO]     = file__metadata_block_ignore;
    handler[FLAC__PADDING]        = file__metadata_block_ignore;
    handler[FLAC__APPLICATION]    = file__metadata_block_ignore;
    handler[FLAC__SEEKTABLE]      = file__metadata_block_ignore;
    handler[FLAC__VORBIS_COMMENT] = file__metadata_vorbis_comment;
    handler[FLAC__CUESHEET]       = file__metadata_block_ignore;
    handler[FLAC__PICTURE]        = file__metadata_block_ignore;
    handler[FLAC__INVALID]        = file__metadata_block_ignore;

    end = false;
    found_stream_info = false;

    while( !end ) {
        {
            uint8_t buf[4];
            if( 4 != read(fd, buf, 4) ) {
                return MI_ERROR_DECODE_ERROR;
            }

            process_metadata_block_header( buf, &end, &type, &block_length );
        }

        status = (*handler[type])( fd, block_length, metadata );
        if( MI_RETURN_OK != status ) {
            return status;
        }
        if( FLAC__STREAMINFO == type ) {
            found_stream_info = true;
        }
    }

    if( false == found_stream_info ) {
        return MI_ERROR_DECODE_ERROR;
    }

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
static media_status_t stream__metadata_block_ignore( FLACContext *fc,
                                                     const uint32_t length )
{
    fstream_skip( length );
    return MI_RETURN_OK;
}

/**
 *  Skips over the dat in the block.
 *
 *  @param fc ignored
 *  @param length the number of bytes in the block
 *  @param buf ignored
 */
static media_status_t file__metadata_block_ignore( int fd,
                                                   const uint32_t length,
                                                   media_metadata_t *metadata )
{
    if( -1 != lseek(fd, length, SEEK_CUR) ) {
        return MI_RETURN_OK;
    }

    return MI_ERROR_DECODE_ERROR;
}
/**
 *  Processes the data in a STREAMINFO block.
 *
 *  @param fc the FLAC context data
 *  @param length the number of bytes in the block
 *  @param buf the scratch buffer to read into
 */
static media_status_t stream__metadata_streaminfo_block( FLACContext *fc,
                                                         const uint32_t length )
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

static media_status_t file__metadata_vorbis_comment( int fd,
                                                     const uint32_t length,
                                                     media_metadata_t *metadata )
{
    uint32_t list_size;
    
    bzero( metadata, sizeof(media_metadata_t) );
    metadata->track_number = -1;
    metadata->disc_number = -1;
    metadata->gain.track_gain = 0.0;
    metadata->gain.track_peak = 0.0;
    metadata->gain.album_gain = 0.0;
    metadata->gain.album_peak = 0.0;

    /* Skip the vendor information */
    {   uint32_t vendor_length;
        if( false == file__read_uint32_t(fd, &vendor_length) ) {
            return MI_ERROR_DECODE_ERROR;
        }
        if( -1 == lseek(fd, vendor_length, SEEK_CUR) ) {
            return MI_ERROR_DECODE_ERROR;
        }
    }

    if( false == file__read_uint32_t(fd, &list_size) ) {
        return MI_ERROR_DECODE_ERROR;
    }

    while( 0 < list_size ) {
        uint32_t comment_length;
        flac_metadata_t type;

        list_size--;

        if( false == file__read_uint32_t(fd, &comment_length) ) {
            return MI_ERROR_DECODE_ERROR;
        }

        type = flac_get_key_value( fd, &comment_length );
        switch( type ) {
            case FM__ALBUM:
            {
                uint32_t album_length;

                album_length = MIN( MEDIA_ALBUM_LENGTH, comment_length );
                if( album_length != read(fd, &metadata->album, album_length) ) {
                    return MI_ERROR_DECODE_ERROR;
                }
                comment_length -= album_length;
                break;
            }

            case FM__ARTIST:
            {
                uint32_t artist_length;

                artist_length = MIN( MEDIA_ARTIST_LENGTH, comment_length );
                if( artist_length != read(fd, &metadata->artist, artist_length) ) {
                    return MI_ERROR_DECODE_ERROR;
                }
                comment_length -= artist_length;
                break;
            }

            case FM__DISCNUMBER:
                if( false == flac_get_int32_t(fd, &comment_length, &metadata->disc_number) ) {
                    return MI_ERROR_DECODE_ERROR;
                }
                break;

            case FM__TITLE:
            {
                uint32_t title_length;

                title_length = MIN( MEDIA_TITLE_LENGTH, comment_length );
                if( title_length != read(fd, &metadata->title, title_length) ) {
                    return MI_ERROR_DECODE_ERROR;
                }
                comment_length -= title_length;
                break;
            }

            case FM__TRACKNUMBER:
                if( false == flac_get_int32_t(fd, &comment_length, &metadata->track_number) ) {
                    return MI_ERROR_DECODE_ERROR;
                }
                break;

            case FM__REPLAYGAIN_ALBUM_PEAK:
                if( false == flac_get_double(fd, &comment_length, &metadata->gain.album_peak) ) {
                    return MI_ERROR_DECODE_ERROR;
                }
                break;

            case FM__REPLAYGAIN_ALBUM_GAIN:
                if( false == flac_get_double(fd, &comment_length, &metadata->gain.album_gain) ) {
                    return MI_ERROR_DECODE_ERROR;
                }
                break;

            case FM__REPLAYGAIN_TRACK_PEAK:
                if( false == flac_get_double(fd, &comment_length, &metadata->gain.track_peak) ) {
                    return MI_ERROR_DECODE_ERROR;
                }
                break;

            case FM__REPLAYGAIN_TRACK_GAIN:
                if( false == flac_get_double(fd, &comment_length, &metadata->gain.track_gain) ) {
                    return MI_ERROR_DECODE_ERROR;
                }
                break;

            default:
                break;
        }

        if( -1 == lseek(fd, comment_length, SEEK_CUR) ) {
            return MI_ERROR_DECODE_ERROR;
        }
    }

    return MI_RETURN_OK;
}

static media_status_t play_song( FLACContext *fc,
                                 queue_handle_t idle,
                                 const int32_t gain,
                                 media_command_fn_t command_fn )
{
    dsp_status_t status;
    flac_data_node_t *node;
    media_status_t rv;

    rv = MI_END_OF_SONG;
    node = NULL;

    while( 1 ) {
        int32_t consumed;
        uint8_t *read_buffer;
        int32_t bytes_left;

        if( NULL == node ) {
            os_queue_receive( idle, &node, WAIT_FOREVER );
        }

        read_buffer = (uint8_t*) fstream_get_buffer( fc->max_framesize, (size_t*) &bytes_left );

        if( 0 == bytes_left ) {
            goto done;
        }

        if( false == (*command_fn)() ) {
            rv = MI_STOPPED_BY_REQUEST;
            goto done;
        }

        memset( node->decode_0, 0, sizeof(node->decode_0) );
        memset( node->decode_1, 0, sizeof(node->decode_1) );

        if( 0 != flac_decode_frame(fc, node->decode_0, node->decode_1, read_buffer, bytes_left) ) {
            rv = MI_ERROR_DECODE_ERROR;
            goto done;
        }

        consumed = fc->gb.index / 8;

        fstream_release_buffer( consumed );

        if( 0 < fc->blocksize ) {
            status = dsp_queue_data( node->decode_0, node->decode_1, fc->blocksize,
                                     fc->samplerate, gain, &dsp_callback, node );
            if( DSP_RETURN_OK != status ) {
                os_queue_send_to_back( idle, &node, NO_WAIT );
                rv = MI_ERROR_DECODE_ERROR;
                goto done;
            }

            node = NULL;
        }
    }

done:
    fstream_release_buffer( 0 );
    if( NULL != node ) {
        os_queue_send_to_back( idle, &node, NO_WAIT );
    }
    dsp_data_complete( NULL, NULL );
    return rv;
}

/**
 *  Used to read a uint32_t from a file and advance the file
 *  pointer.
 *
 *  @param fd the file descriptor to read from
 *  @param out the uint32_t data to output
 *
 *  @return true on success, false otherwise
 */
static bool file__read_uint32_t( int fd, uint32_t *out )
{
    uint8_t buf[4];

    if( 4 == read(fd, buf, 4) ) {
        *out = buf[0];
        *out |= (buf[1] << 8);
        *out |= (buf[2] << 16);
        *out |= (buf[3] << 24);

        return true;
    }

    return false;
}

/**
 *  Used to read out of a vorbis comment the key and point the
 *  next file character to the value if it is a known type.  If
 *  the type is unknown, the entire comment is skipped.
 *
 *  @param fd the file descriptor to read from
 *  @param len the bytes in the comment (in), then the bytes
 *             left after processing
 *
 *  @return the metadata type of this comment
 */
static flac_metadata_t flac_get_key_value( int fd, uint32_t *len )
{
    char c;

    if( 0 < *len ) {
        if( 1 == read(fd, &c, 1) ) {
            (*len)--;
            switch( c ) {
                case 'a':
                case 'A':
                    if( 5 < *len ) {
                        uint8_t buf[5];
                        if( 5 == read(fd, buf, 5) ) {
                            *len -= 5;
                            if( 0 == strncasecmp((char*) buf, "LBUM=", 5) ) {
                                return FM__ALBUM;
                            }
                            if( 0 == strncasecmp((char*) buf, "RTIST", 5) ) {
                                if( 1 == read(fd, &c, 1) ) {
                                    (*len)--;
                                    if( '=' == c ) {
                                        return FM__ARTIST;
                                    }
                                }
                            }
                        }
                    }
                    break;

                case 'd':
                case 'D':
                    if( 10 < *len ) {
                        uint8_t buf[10];
                        if( 10 == read(fd, buf, 10) ) {
                            *len -= 10;
                            if( 0 == strncasecmp((char*) buf, "ISCNUMBER=", 10) ) {
                                return FM__DISCNUMBER;
                            }
                        }
                    }
                    break;

                case 'r':
                case 'R':
                    if( 21 < *len ) {
                        uint8_t buf[21];
                        if( 21 == read(fd, buf, 21) ) {
                            *len -= 21;
                            if( 0 == strncasecmp((char*) buf, "EPLAYGAIN_ALBUM_PEAK=", 21) ) {
                                return FM__REPLAYGAIN_ALBUM_PEAK;
                            } else if( 0 == strncasecmp((char*) buf, "EPLAYGAIN_ALBUM_GAIN=", 21) ) {
                                return FM__REPLAYGAIN_ALBUM_GAIN;
                            } else if( 0 == strncasecmp((char*) buf, "EPLAYGAIN_TRACK_PEAK=", 21) ) {
                                return FM__REPLAYGAIN_TRACK_PEAK;
                            } else if( 0 == strncasecmp((char*) buf, "EPLAYGAIN_TRACK_GAIN=", 21) ) {
                                return FM__REPLAYGAIN_TRACK_GAIN;
                            } else if( 0 == strncasecmp((char*) buf, "EPLAYGAIN_REFERENCE_L", 21) ) {
                                if( 8 < *len ) {
                                    if( 8 == read(fd, buf, 8) ) {
                                        *len -= 8;
                                        if( 0 == strncasecmp((char*) buf, "OUDNESS=", 8) ) {
                                            return FM__REPLAYGAIN_REFERENCE_LOUDNESS;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;

                case 't':
                case 'T':
                    if( 5 < *len ) {
                        uint8_t buf[6];
                        if( 5 == read(fd, buf, 5) ) {
                            *len -= 5;
                            if( 0 == strncasecmp((char*) buf, "ITLE=", 5) ) {
                                return FM__TITLE;
                            }
                            if( 0 == strncasecmp((char*) buf, "RACKN", 5) ) {
                                if( 6 < *len ) {
                                    if( 6 == read(fd, buf, 6) ) {
                                        *len -= 6;
                                        if( 0 == strncasecmp((char*) buf, "UMBER=", 6) ) {
                                            return FM__TRACKNUMBER;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;

                default:
                    break;
            }
        }
    }

    lseek( fd, *len, SEEK_CUR );
    *len = 0;
    return FM__UNKNOWN;
}

/**
 *  Used to read in an int32_t from a file as an ASCII string.
 *
 *  @param fd the file descriptor to read and process
 *  @param len the maximum valid number of of bytes
 *  @param out the output int32_t data
 *
 *  @return true if successful, false otherwise
 */
static bool flac_get_int32_t( int fd, uint32_t *len, int32_t *out )
{
    bool positive;
    bool got_sign;

    *out = 0;
    positive = true;
    got_sign = false;

    while( 0 < *len ) {
        char c;

        if( 1 != read(fd, &c, 1) ) {
            return false;
        }

        (*len)--;

        if( ('0' <= c) && (c <= '9') ) {
            if( ((*out) * 10) < (*out) ) {
                /* we overflowed the output. */
                return false;
            }
            *out *= 10;
            *out += (c - '0');
        } else if( '-' == c ) {
            if( true == got_sign ) {
                return false;
            }
            got_sign = true;
            positive = false;
        } else if( '+' == c ) {
            if( true == got_sign ) {
                return false;
            }
            got_sign = true;
        } else {
            return false;
        }
    }

    if( false == positive ) {
        *out *= -1;
    }

    return true;
}

/**
 *  Used to read in double from a file as an ASCII string.
 *
 *  @param fd the file descriptor to read and process
 *  @param len the maximum valid number of of bytes
 *  @param out the output double data
 *
 *  @return true if successful, false otherwise
 */
static bool flac_get_double( int fd, uint32_t *len, double *out )
{
    bool positive;
    uint32_t number;
    uint32_t decimal_point;

    *out = 0.0;
    positive = true;
    decimal_point = 0;
    number = 0;

    while( 0 < *len ) {
        char c;

        if( 1 != read(fd, &c, 1) ) {
            return false;
        }

        (*len)--;

        switch( c ) {
            case '+':
                break;
            case '-':
                positive = false;
                break;
            case '.':
                decimal_point = 1;
                break;
            case '0': case '1': case '2':
            case '3': case '4': case '5':
            case '6': case '7': case '8': case '9':
                number *= 10;
                number += (c - '0');
                decimal_point *= 10;
                break;
            default:
                goto done;
        }
    }

done:
    *out = ((double) number) / decimal_point;
    if( false == positive ) {
        *out *= -1;
    }

    return true;
}

static void dsp_callback( int32_t *left, int32_t *right, void *data )
{
    flac_data_node_t *node = (flac_data_node_t*) data;

    os_queue_send_to_back( node->idle, &node, NO_WAIT );
}
