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

#include <stddef.h>
#include <string.h>

#include <ibus-physical/ibus-physical.h>

#include "ibus-phone-protocol.h"


#define MAX_PHONE_DISPLAY_SIZE   16
#define MAX_IBUS_PRINT_SIZE      ( MAX_PHONE_DISPLAY_SIZE + 4 )

#define IBUS_DEVICES__PHONE      0xc8
#define IBUS_DEVICES__IKE        0x80
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/* See ibus-phone-protocol.h for details. */
void ipp_init( void )
{
}

/* See ibus-phone-protocol.h for details. */
size_t ibus_phone_display( char *string )
{
    uint8_t out[MAX_IBUS_PRINT_SIZE];
    size_t length;
    uint8_t chars_displayed;
    uint8_t msg_length;
    int8_t checksum = 0;
    uint8_t i;
    
    if( NULL == string ) {
        return 0;
    }
    
    length = strlen( string );
    chars_displayed = (length > MAX_IBUS_PRINT_SIZE)?
                          MAX_PHONE_DISPLAY_SIZE:
                          length;
    
    msg_length = chars_displayed + 6;
    
    out[0] = (uint8_t) IBUS_DEVICES__PHONE;
    out[1] = chars_displayed + 5;
    out[2] = (uint8_t) IBUS_DEVICES__IKE;
    /* Special characters for displaying the message */
    out[3] = 0x23;
    out[4] = 0x42;
    out[5] = 0x07;
    
    memcpy( &out[6], string, chars_displayed );

    for( i = 0; i < msg_length; i++ ) {
        checksum ^= out[i];
    }
    out[i] = checksum;
    
    if( false == ibus_physical_send_message( out, msg_length ) ) {
        return 0;
    }

    return chars_displayed;
}
