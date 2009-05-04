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
 * $Id: decoder.h,v 1.17 2004/01/23 09:41:32 rob Exp $
 */

# ifndef LIBMAD_DECODER_H
# define LIBMAD_DECODER_H

# include "stream.h"
# include "frame.h"
# include "synth.h"

enum mad_flow {
  MAD_FLOW_CONTINUE = 0x0000,	/* continue normally */
  MAD_FLOW_STOP     = 0x0010,	/* stop decoding normally */
  MAD_FLOW_BREAK    = 0x0011,	/* stop decoding and signal an error */
  MAD_FLOW_IGNORE   = 0x0020	/* ignore the current frame */
};

int mad_decoder_run( void *data,
                     enum mad_flow (*input_func)(void *,
                                                 unsigned char *, /* where to put the data */
                                                 unsigned int *), /* how large the data was - max 512 */
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
                     int options );
# endif
