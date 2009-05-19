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

#include <stdint.h>

#include "config.h"
#include "command.h"
#include "io.h"
#include "crc.h"
#include "memcard.h"
#include "timing-parameters.h"

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

/* See command.h for details. */
mc_status_t mc_command( const mc_cmd_type_t command,
                        const uint32_t argument,
                        uint8_t *response )
{
    uint8_t message[6];
    uint8_t c;
    int i;

    message[0] = 0x40 | (0x3f & ((uint8_t) command));
    message[1] = argument >> 24;
    message[2] = 0xff & (argument >> 16);
    message[3] = 0xff & (argument >> 8);
    message[4] = 0xff & argument;
    message[5] = (crc7(message, 5) << 1) | 0x01;

    for( i = 0; i < sizeof(message); i++ ) {
        io_send_read( message[i], &c );
    }

    /* Wait for a response - if no response has been
     * received after MC_Ncr attempts, the card has timed-out */
    for( i = 0; i < MC_Ncr; i++ ) {
        uint8_t tmp;
        io_send_read( 0xff, &tmp );

        if( 0xff != tmp ) {
            *response = (uint8_t) tmp;
            return MC_RETURN_OK;
        }
    }

    return MC_ERROR_TIMEOUT;
}

/* See command.h for details. */
mc_status_t mc_select_and_command( const mc_cmd_type_t command,
                                   const uint32_t argument,
                                   const mc_response_type_t type,
                                   uint8_t *response )
{
    mc_status_t status;
    size_t count;

    io_clean_select();

    status = mc_command( command, argument, response );

    if( MC_RETURN_OK == status ) {
        for( count = 1; count < (size_t) type; count++ ) {
            io_send_read( 0xff, &response[count] );
        }
    }

    io_clean_unselect();

    return status;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
