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

#define MAX_PHONE_DISPLAY_SIZE   11
#define MAX_IBUS_PRINT_SIZE      ( 3 + MAX_PHONE_DISPLAY_SIZE )
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

    if( NULL == string ) {
        _D1( "%s:%d - NULL passed in\n", __FILE__, __LINE__ );
        return 0;
    }

    _D1( "%s:%d - string `%s`\n", __FILE__, __LINE__, string );

    length = strlen( string );
    chars_displayed = (length > MAX_PHONE_DISPLAY_SIZE) ? MAX_PHONE_DISPLAY_SIZE : length;

    _D1( "%s:%d - length(%d) chars to display(%d) MAX_SIXE(%d)\n", __FILE__, __LINE__,
         length, chars_displayed, MAX_PHONE_DISPLAY_SIZE );

    msg_length = chars_displayed + 3;
    _D1( "%s:%d - msg length(%d)\n", __FILE__, __LINE__, msg_length );

    memset( out, 0x20, sizeof(out) );
    out[0] = 0x23;
    out[1] = 0x43;
    out[2] = 0x07;
    memcpy( &out[3], string, chars_displayed );

    _D1( "%s:%d `%.*s`\n", __FILE__, __LINE__, chars_displayed, &out[3] );

    if( false == ibus_physical_send_message(IBUS_DEVICE__TEL, IBUS_DEVICE__IKE,
                                            out, msg_length) ) {
        return 0;
    }

    return chars_displayed;
}
