/*
 * Copyright (c) 2008  Weston Schmidt
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
#include <stdint.h>
#include <stddef.h>

#include "crc.h"

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

/* See crc.h for details. */
uint8_t crc7( const uint8_t *message, const size_t length )
{
    size_t i, j;
    uint8_t crc = 0;
    uint8_t c;

    for( i = 0; i < length; i++ ) {
        c = *message++;

        for( j = 0; j < 8; j++ ) {
            crc <<= 1;
            crc |= (0x80 == (0x80 & c)) ? 1 : 0;
            c <<= 1;
            if( 0x80 == (0x80 & crc) ) {
                crc ^= 0x09;
            }
        }
    }

    /* Clear out the appended '0' bits. */
    for( j = 0; j < 7; j++ ) {
        crc <<= 1;
        if( 0x80 == (0x80 & crc) ) {
            crc ^= 0x09;
        }
    }

    return (0x7f & crc);
}

/* See crc.h for details. */
uint16_t crc16( const uint8_t *message, const size_t length )
{
    size_t i, j;
    uint16_t crc = 0;
    uint8_t c;

    for( i = 0; i < length; i++ ) {
        c = *message++;

        for( j = 0; j < 8; j++ ) {
            if( 0x8000 == (0x8000 & crc) ) {
                crc <<= 1;
                crc |= (0x80 == (0x80 & c)) ? 1 : 0;
                crc ^= 0x1021;
            } else {
                crc <<= 1;
                crc |= (0x80 == (0x80 & c)) ? 1 : 0;
            }
            c <<= 1;
        }
    }

    /* Clear out the appended '0' bits. */
    for( j = 0; j < 16; j++ ) {
        if( 0x8000 == (0x8000 & crc) ) {
            crc <<= 1;
            crc ^= 0x1021;
        } else {
            crc <<= 1;
        }
    }

    return crc;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
