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

#include <stdbool.h>
#include <stdint.h>

#include "message-converter.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef enum {
    IRP_RX_CMD__STATUS          = 0x00,
    IRP_RX_CMD__STOP            = 0x01,
    IRP_RX_CMD__PAUSE           = 0x02,
    IRP_RX_CMD__PLAY            = 0x03,
    IRP_RX_CMD__FAST_PLAY       = 0x04,
    IRP_RX_CMD__SEEK            = 0x05,
    IRP_RX_CMD__CHANGE_DISC     = 0x06,
    IRP_RX_CMD__SCAN_DISC       = 0x07,
    IRP_RX_CMD__RANDOMIZE       = 0x08,
    IRP_RX_CMD__ALT_SEEK        = 0x0a
} irp_rx_command_t;

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
bool message_converter( const ibus_io_msg_t *in, irp_rx_msg_t *out )
{
    uint8_t checksum;
    int32_t i;
    uint8_t src, dst, length;
    const uint8_t *payload;

    if( (NULL == in) || (NULL == out) ) {
        return false;
    }

    if( IBUS_IO_STATUS__OK != in->status ) {
        return false;
    }

    if( in->size < 4 ) {
        /* Message isn't large enough to contain the data needed. */
        return false;
    }

    checksum = 0;
    for( i = 0; i < in->size; i++ ) {
        checksum ^= in->buffer[i];
    }
    if( 0 != checksum ) {
        return false;
    }

    src     =  in->buffer[0];
    length  =  in->buffer[1];
    dst     =  in->buffer[2];
    payload = &in->buffer[3];

    /* Check the source */
    if( ((uint8_t) IBUS_DEVICE__CD_CHANGER) == src ) {
        /* If we sent this message, ignore it. */
        return false;
    }

    /* Check the destination */
    if( (((uint8_t) IBUS_DEVICE__CD_CHANGER)     != dst) &&
        (((uint8_t) IBUS_DEVICE__BROADCAST_LOW)  != dst) &&
        (((uint8_t) IBUS_DEVICE__BROADCAST_HIGH) != dst) )
    {
        /* Message is not to us - ignore & report traffic. */
        out->command = IRP_CMD__TRAFFIC;
        out->disc = 0;
        return true;
    }

    if( (3 == length) && (0x01 == *payload) ) {
        out->command = IRP_CMD__POLL;
    } else if( (5 == length) && (0x38 == *payload) ) {
        switch( payload[1] ) {
            case IRP_RX_CMD__STATUS:
                out->command = IRP_CMD__GET_STATUS;
                break;
            case IRP_RX_CMD__STOP:
                out->command = IRP_CMD__STOP;
                break;
            case IRP_RX_CMD__PAUSE:
                out->command = IRP_CMD__PAUSE;
                break;
            case IRP_RX_CMD__PLAY:
                out->command = IRP_CMD__PLAY;
                break;
            case IRP_RX_CMD__FAST_PLAY:
                if( 0 == payload[2] ) {
                    out->command = IRP_CMD__FAST_PLAY__REVERSE;
                } else if( 1 == payload[2] ) {
                    out->command = IRP_CMD__FAST_PLAY__FORWARD;
                } else {
                    return false;
                }
                break;
            case IRP_RX_CMD__SEEK:
                if( 0 == payload[2] ) {
                    out->command = IRP_CMD__SEEK__NEXT;
                } else if( 1 == payload[2] ) {
                    out->command = IRP_CMD__SEEK__PREV;
                } else {
                    return false;
                }
                break;
            case IRP_RX_CMD__ALT_SEEK:
                if( 0 == payload[2] ) {
                    out->command = IRP_CMD__SEEK__ALT_NEXT;
                } else if( 1 == payload[2] ) {
                    out->command = IRP_CMD__SEEK__ALT_PREV;
                } else {
                    return false;
                }
                break;
            case IRP_RX_CMD__CHANGE_DISC:
                if( (0 < payload[2]) && (payload[2] < 7) ) {
                    out->command = IRP_CMD__CHANGE_DISC;
                    out->disc = payload[2];
                } else {
                    return false;
                }
                break;
            case IRP_RX_CMD__SCAN_DISC:
                if( 0 == payload[2] ) {
                    out->command = IRP_CMD__SCAN_DISC__DISABLE;
                } else if( 1 == payload[2] ) {
                    out->command = IRP_CMD__SCAN_DISC__ENABLE;
                } else {
                    return false;
                }
                break;
            case IRP_RX_CMD__RANDOMIZE:
                if( 0 == payload[2] ) {
                    out->command = IRP_CMD__RANDOMIZE__DISABLE;
                } else if( 1 == payload[2] ) {
                    out->command = IRP_CMD__RANDOMIZE__ENABLE;
                } else {
                    return false;
                }
                break;
            default:
                return false;
        }
    } else {
        /* Message is to us but we don't understand it - ignore message completely. */
        return false;
    }

    if( IRP_CMD__CHANGE_DISC != out->command ) {
        out->disc = 0;
    }

    return true;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
