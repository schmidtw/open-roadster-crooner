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

#define IBUS_PHONE_PROTOCOL 0

#if (0 < IBUS_PHONE_PROTOCOL)
#include <string.h>
#define _D1(...) printf(__VA_ARGS__)
#else
#define _D1(...)
#endif

const static uint8_t beginning[] = {
        (uint8_t) IBUS_DEVICE__PHONE,
        0, // the number of characters in this message
        (uint8_t) IBUS_DEVICE__IKE,
        /* special character for displaying text */
        0x23,
        0x43,
        0x07 };

#define BEGINNING_SIZE sizeof(beginning)

#define MAX_PHONE_DISPLAY_SIZE   11
#define MAX_IBUS_PRINT_SIZE      ( MAX_PHONE_DISPLAY_SIZE + BEGINNING_SIZE + 1 )
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
    _D1( "%s - string `%s`\n", "ibus-phone-protocol.c", string );
    length = strlen( string );
    chars_displayed = (length > MAX_PHONE_DISPLAY_SIZE)?
                          MAX_PHONE_DISPLAY_SIZE:
                          length;
    _D1( "%s - length(%d) chars to display(%d) MAX_SIXE(%d)\n", "ibus-phone-protocol.c", length, chars_displayed, MAX_PHONE_DISPLAY_SIZE );
    
    msg_length = chars_displayed + BEGINNING_SIZE;
    _D1( "%s - msg length(%d)\n", "ibus-phone-protocol.c", msg_length );
    memcpy( out, beginning, BEGINNING_SIZE );
    out[1] = msg_length - 1;
    
    memcpy( &out[BEGINNING_SIZE], string, chars_displayed );
#ifdef IBUS_PHONE_PROTOCOL
    _D1("%s: `%.*s`\n", "ibus-phone-protocol.c", chars_displayed, &out[BEGINNING_SIZE]);
#endif
    _D1("%s: Final Message:\n", "ibus-phone-protocol.c" );
    for( i = 0; i < msg_length; i++ ) {
        _D1("[%d]0x%02x '%c' ", i, out[i], (out[i]>31?out[i]:' '));
        checksum ^= out[i];
    }
    out[i] = checksum;
    _D1("\nChecksum: out[%d] = 0x%02x\n", i, out[i]);
    
    if( false == ibus_physical_send_message( out, msg_length+1 ) ) {
        return 0;
    }

    return chars_displayed;
}
