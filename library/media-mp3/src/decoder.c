/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: decoder.c,v 1.22 2004/01/23 09:41:32 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
# endif

# ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
# endif

# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif

# include <stdlib.h>

# ifdef HAVE_ERRNO_H
#  include <errno.h>
# endif

#include <string.h>

# include "stream.h"
# include "frame.h"
# include "synth.h"
# include "decoder.h"

static enum mad_flow error_default( void *data, struct mad_stream *stream,
                                    struct mad_frame *frame )
{
    int *bad_last_frame = data;

    if( MAD_ERROR_BADCRC == stream->error ) {
        if( 0 != *bad_last_frame ) {
            mad_frame_mute( frame );
        } else {
            *bad_last_frame = 1;
        }

        return MAD_FLOW_IGNORE;
    }

    return MAD_FLOW_CONTINUE;
}

int mad_decoder_run( void *data,
                     enum mad_flow (*input_func)(void *,
                                                 unsigned char *,
                                                 unsigned int *),
                     enum mad_flow (*header_func)(void *,
                                                  struct mad_header const *),
                     enum mad_flow (*filter_func)(void *,
                                                  struct mad_stream const *,
                                                  struct mad_frame *),
                     enum mad_flow (*output_func)(void *,
                                                  struct mad_header const *,
                                                  struct mad_pcm *),
                     enum mad_flow (*error_func)(void *,
                                                 struct mad_stream *,
                                                 struct mad_frame *),
                     struct mad_stream *stream,
                     struct mad_frame *frame,
                     struct mad_synth *synth,
                     unsigned char *input_buffer,
                     const unsigned int input_buffer_length,
                     int options )
{
    enum mad_flow (*error_fn)(void *, struct mad_stream *, struct mad_frame *);
    void *error_data;
    int bad_last_frame = 0;
    unsigned int got;
    int consumed;
    int input_first_unused;
    int result = 0;

    if( (NULL == input_func) || (NULL == stream) ||
        (NULL == frame) || (NULL == synth) || (NULL == input_buffer) ||
        (0 == input_buffer_length) )
    {
        return -1;
    }

    if( NULL != error_func ) {
        error_fn   = error_func;
        error_data = data;
    } else {
        error_fn   = error_default;
        error_data = &bad_last_frame;
    }

    consumed = 0;
    input_first_unused = 0;

    mad_stream_init( stream );
    mad_frame_init( frame );
    mad_synth_init( synth );

    mad_stream_options( stream, options );

    do {
        /* We have left over data in the buffer. */
        if( NULL != stream->next_frame ) {
            consumed = stream->next_frame - input_buffer;
        }

        /* Move any remaining data to the front of the buffer. */
        if( 0 < consumed ) {
            input_first_unused -= consumed;
            memmove( input_buffer, &input_buffer[consumed], input_first_unused );
            consumed = 0;
        }

        got = input_buffer_length - input_first_unused;
        /* Read in some data. */
        switch(input_func(data, &input_buffer[input_first_unused], &got)) {
            case MAD_FLOW_STOP:     goto done;
            case MAD_FLOW_CONTINUE: break;
            default:                goto fail;
        }

        input_first_unused += got;

        /* We're full. */
        mad_stream_buffer( stream, input_buffer, input_first_unused );

        /* Find and process a frame of mp3 data. */
        while( 1 ) {
            /* Process the header if present. */
            if( NULL != header_func ) {
                if( -1 == mad_header_decode(&frame->header, stream) ) {
                    if( !MAD_RECOVERABLE(stream->error) ) {
                        break;  /* Skip frame. */
                    }

                    switch (error_fn(error_data, stream, frame)) {
                        case MAD_FLOW_STOP:     goto done;
                        case MAD_FLOW_BREAK:    goto fail;
                        default:                continue;
                    }
                }

                switch( header_func(data, &frame->header) ) {
                    case MAD_FLOW_BREAK:    goto fail; 
                    case MAD_FLOW_IGNORE:   continue; 
                    case MAD_FLOW_CONTINUE: break; 
                    default:                goto done;
                }
            }

            /* Decode the frame. */
            if( -1 == mad_frame_decode(frame, stream) ) {
                if( !MAD_RECOVERABLE(stream->error) ) {
                    break;  /* Skip frame. */
                }

                switch( error_fn(error_data, stream, frame) ) {
                    case MAD_FLOW_STOP:     goto done;
                    case MAD_FLOW_IGNORE:   break;
                    case MAD_FLOW_CONTINUE: continue; 
                    default:                goto fail;
                }
            } else {
                bad_last_frame = 0;
            }

            /* Perform filtering. */
            if( NULL != filter_func ) {
                switch( filter_func(data, stream, frame) ) {
                    case MAD_FLOW_STOP:     goto done;
                    case MAD_FLOW_IGNORE:   continue;
                    case MAD_FLOW_CONTINUE: break;
                    default:                goto fail;
                }
            }

            mad_synth_frame( synth, frame );

            /* Output the PCM data. */
            if( NULL != output_func ) {
                switch( output_func(data, &frame->header, &synth->pcm) ) {
                    case MAD_FLOW_STOP:     goto done;
                    case MAD_FLOW_IGNORE:
                    case MAD_FLOW_CONTINUE: break;
                    default:                goto fail;
                }
            }
        }

    } while( MAD_ERROR_BUFLEN == stream->error );

fail:
    result = -1;

done:
    mad_synth_finish( synth );
    mad_frame_finish( frame );
    mad_stream_finish( stream );

    return result;
}
