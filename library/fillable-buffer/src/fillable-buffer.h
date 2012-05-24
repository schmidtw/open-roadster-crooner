/*
 *  fillable-buffer.h - a buffer that can be easily filled & flushed
 *
 *  Written by Weston Schmidt (2008)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  In other words, you are welcome to use, share and improve this program.
 *  You are forbidden to forbid anyone else to use, share and improve
 *  what you give them.   Help stamp out software-hoarding!
 */
#ifndef _FILLABLE_BUFFER_H_
#define _FILLABLE_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

typedef void (*fillable_buffer_flush_fct)( void *data, const uint8_t *buf,
                                           const size_t size );

typedef struct {
    uint8_t *buf;
    size_t size;
    uint32_t offset;
    void *data;
    fillable_buffer_flush_fct flush;
} fillable_buffer_t;

/**
 *  Used to append data to the fillable buffer and flush the buffer if needed.
 *
 *  @note size & offset should be initialized to 0 & only the internals of
 *        these functions should change them.
 *
 *  @param fb the fillable buffer to fill and flush
 *  @param data the data to append
 *  @param size the size of the data to append
 *
 *  @return 0 on success, less then zero otherwise
 */
int fillbuf_append( fillable_buffer_t *fb, const uint8_t *data,
                    const size_t size );


/**
 *  Used to flush the data in the buffer if there is any.
 *
 *  @param fb the fillable buffer to fill and flush
 *
 *  @return 0 on success, less then zero otherwise
 */
int fillbuf_flush( fillable_buffer_t *fb );

#endif
