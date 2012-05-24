/*
 * Copyright (c) 2012  Weston Schmidt
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

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "fillable-buffer.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int fillbuf_append( fillable_buffer_t *fb, const uint8_t *data,
                    const size_t size )
{
    size_t space;
    size_t to_send;
    uint32_t offset;

    if( (NULL == fb) || (NULL == fb->buf) ) {
        return -1;
    }

    offset = 0;
    to_send = size;
    while( 0 < to_send ) {
        size_t send;

        space = fb->size - fb->offset;
        send = to_send;
        if( space < send ) {
            send = space;
        }

        memcpy( &fb->buf[fb->offset], &data[offset], send );
        fb->offset += send;
        to_send -= send;
        offset += send;

        if( fb->offset == fb->size ) {
            fillbuf_flush( fb );
        }
    }

    return 0;
}


int fillbuf_flush( fillable_buffer_t *fb )
{
    if( (NULL == fb) || (NULL == fb->buf) ) {
        return -1;
    }

    if( NULL != fb->flush ) {
        if( 0 < fb->offset ) {
            (*fb->flush)( fb->data, fb->buf, fb->offset );
        }
    }
    fb->offset = 0;

    return 0;
}


/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
