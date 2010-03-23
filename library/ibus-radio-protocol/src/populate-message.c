/*
 * Copyright (c) 2010  Weston Schmidt
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

#include <string.h>

#include "populate-message.h"

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
irp_status_t populate_message( const ibus_device_t src,
                               const ibus_device_t dst,
                               const uint8_t *payload,
                               const size_t payload_length,
                               uint8_t *out,
                               const size_t out_length )
{
    int32_t i;
    int8_t checksum;

    if( (NULL == payload) ||
        (0 == payload_length) || (255 <= (payload_length + 4)) ||
        (NULL == out) || (out_length < (payload_length + 4)) )
    {
        return IRP_ERROR_PARAMETER;
    }

    out[0] = (uint8_t) src;
    out[1] = (uint8_t) (payload_length + 2);
    out[2] = (uint8_t) dst;

    memcpy( &out[3], payload, payload_length );

    checksum = 0;
    for( i = 0; i < (payload_length + 3); i++ ) {
        checksum ^= out[i];
    }

    out[i] = checksum;

    return IRP_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
