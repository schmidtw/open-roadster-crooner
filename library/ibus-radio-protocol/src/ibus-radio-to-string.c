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

#include "ibus-radio-protocol.h"

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
/* See ibus-radio-protocol.h for details. */
const char* irp_state_to_string( const irp_state_t state )
{
    switch( state ) {
        case IRP_STATE__STOPPED:
            return "IRP_STATE__STOPPED";
        case IRP_STATE__PAUSED:
            return "IRP_STATE__PAUSED";
        case IRP_STATE__PLAYING:
            return "IRP_STATE__PLAYING";
        case IRP_STATE__FAST_PLAYING__FORWARD:
            return "IRP_STATE__FAST_PLAYING__FORWARD";
        case IRP_STATE__FAST_PLAYING__REVERSE:
            return "IRP_STATE__FAST_PLAYING__REVERSE";
        case IRP_STATE__SEEKING:
            return "IRP_STATE__SEEKING";
        case IRP_STATE__SEEKING__NEXT:
            return "IRP_STATE__SEEKING__NEXT";
        case IRP_STATE__SEEKING__PREV:
            return "IRP_STATE__SEEKING__PREV";
        case IRP_STATE__LOADING_DISC:
            return "IRP_STATE__LOADING_DISC";
    }

    return "unknown";
}

/* See ibus-radio-protocol.h for details. */
const char* irp_cmd_to_string( const irp_cmd_t cmd )
{
    switch( cmd ) {
        case IRP_CMD__GET_STATUS:
            return "IRP_CMD__GET_STATUS";
        case IRP_CMD__STOP:
            return "IRP_CMD__STOP";
        case IRP_CMD__PAUSE:
            return "IRP_CMD__PAUSE";
        case IRP_CMD__PLAY:
            return "IRP_CMD__PLAY";
        case IRP_CMD__FAST_PLAY__FORWARD:
            return "IRP_CMD__FAST_PLAY__FORWARD";
        case IRP_CMD__FAST_PLAY__REVERSE:
            return "IRP_CMD__FAST_PLAY__REVERSE";
        case IRP_CMD__SEEK__NEXT:
            return "IRP_CMD__SEEK__NEXT";
        case IRP_CMD__SEEK__PREV:
            return "IRP_CMD__SEEK__PREV";
        case IRP_CMD__SCAN_DISC__ENABLE:
            return "IRP_CMD__SCAN_DISC__ENABLE";
        case IRP_CMD__SCAN_DISC__DISABLE:
            return "IRP_CMD__SCAN_DISC__DISABLE";
        case IRP_CMD__RANDOMIZE__ENABLE:
            return "IRP_CMD__RANDOMIZE__ENABLE";
        case IRP_CMD__RANDOMIZE__DISABLE:
            return "IRP_CMD__RANDOMIZE__DISABLE";
        case IRP_CMD__CHANGE_DISC:
            return "IRP_CMD__CHANGE_DISC";
        case IRP_CMD__POLL:
            return "IRP_CMD__POLL";
        case IRP_CMD__TRAFFIC:
            return "IRP_CMD__TRAFFIC";
    }

    return "unknown";
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
