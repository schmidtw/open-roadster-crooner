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

#include "ibus-radio-protocol.h"
#include "bcd-track-converter.h"
#include "message-converter.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef enum {
    IRP_AUDIO_STATUS__STOPPED = 0x02,
    IRP_AUDIO_STATUS__PLAYING = 0x09,
    IRP_AUDIO_STATUS__PAUSED  = 0x0c
} irp_audio_status_t;

typedef enum {
    IBUS_STATE__STOPPED            = 0x00,
    IBUS_STATE__PAUSED             = 0x01,
    IBUS_STATE__PLAYING            = 0x02,
    IBUS_STATE__FAST_PLAYING       = 0x03,
    IBUS_STATE__REWINDING          = 0x04,
    IBUS_STATE__SEEKING_NEXT       = 0x05,
    IBUS_STATE__SEEKING_PREV       = 0x06,
    IBUS_STATE__SEEKING            = 0x07,
    IBUS_STATE__LOADING_DISC       = 0x08,
    IBUS_STATE__CHECKING_FOR_DISC  = 0x09,
    IBUS_STATE__NO_MAGAZINE        = 0x0a
} ibus_state_t;

typedef enum {
    IRP_MAGAZINE_STATE__NORMAL   = 0x00,
    IRP_MAGAZINE_STATE__NO_DISCS = 0x10,
    IRP_MAGAZINE_STATE__ERROR    = 0x18
} irp_magazine_state_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
void _irp_send_device_status_ready( void );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See ibus-radio-protocol.h for details. */
void irp_init( void )
{
    ibus_physical_init();
}

/* See ibus-radio-protocol.h for details. */
irp_status_t irp_get_message( irp_rx_msg_t *msg )
{
    bool done;

    if( NULL == msg ) {
        return IRP_ERROR_PARAMETER;
    }

    done = false;
    while( false == done ) {
        ibus_io_msg_t *ibus_msg;
        ibus_msg = ibus_physical_get_message();

        done = message_converter( ibus_msg, msg );

        ibus_physical_release_message( ibus_msg );
    }

    return IRP_RETURN_OK;
}

/* See ibus-radio-protocol.h for details. */
void irp_send_announce( void )
{
    _irp_send_device_status_ready();
}

/* See ibus-radio-protocol.h for details. */
void irp_send_poll_response( void )
{
    _irp_send_device_status_ready();
}

/* See ibus-radio-protocol.h for details. */
irp_status_t irp_send_normal_status( const irp_state_t device_state,
                                     const irp_mode_t mode,
                                     const bool magazine_present,
                                     const uint8_t discs_present,
                                     const uint8_t current_disc,
                                     const uint8_t current_track )
{
    uint8_t payload[] = { 0x39, 0, 0, 0, 0, 0, 0, 0 };

    if( false == magazine_present ) {
        payload[1] = (uint8_t) IBUS_STATE__NO_MAGAZINE;
        payload[2] = (uint8_t) IRP_AUDIO_STATUS__STOPPED;
        payload[3] = (uint8_t) IRP_MAGAZINE_STATE__NORMAL;
    } else if( 0 == discs_present ) {
        payload[1] = (uint8_t) IBUS_STATE__STOPPED;
        payload[2] = (uint8_t) IRP_AUDIO_STATUS__STOPPED;
        payload[3] = (uint8_t) IRP_MAGAZINE_STATE__NO_DISCS;
    } else {
        if( ((0 != current_track) && (IRP_STATE__LOADING_DISC == device_state)) ||
            ((0 == current_track) && (IRP_STATE__LOADING_DISC != device_state)) ||
            (current_disc < 1) || (6 < current_disc) ||
            (0 == (discs_present & (1 << (current_disc - 1)))) )
        {
            return IRP_ERROR_PARAMETER;
        }

        switch( device_state ) {
            case IRP_STATE__STOPPED:
                payload[1] = (uint8_t) IBUS_STATE__STOPPED;
                payload[2] = (uint8_t) IRP_AUDIO_STATUS__STOPPED;
                break; 
            case IRP_STATE__PAUSED:
                payload[1] = (uint8_t) IBUS_STATE__PAUSED;
                payload[2] = (uint8_t) IRP_AUDIO_STATUS__PAUSED;
                break;
            case IRP_STATE__PLAYING:
                payload[1] = (uint8_t) IBUS_STATE__PLAYING;
                payload[2] = (uint8_t) IRP_AUDIO_STATUS__PLAYING;
                switch( mode ) {
                    case IRP_MODE__SCANNING:
                        payload[2] |= 0x10;
                        break;
                    case IRP_MODE__RANDOM:
                        payload[2] |= 0x20;
                        break;
                    default:
                        break;
                    }
                break; 
            case IRP_STATE__FAST_PLAYING__FORWARD:
                payload[1] = (uint8_t) IBUS_STATE__FAST_PLAYING;
                payload[2] = (uint8_t) IRP_AUDIO_STATUS__PLAYING;
                break; 
            case IRP_STATE__FAST_PLAYING__REVERSE:
                payload[1] = (uint8_t) IBUS_STATE__REWINDING;
                payload[2] = (uint8_t) IRP_AUDIO_STATUS__PLAYING;
                break; 
            case IRP_STATE__SEEKING:
                payload[1] = (uint8_t) IBUS_STATE__SEEKING;
                payload[2] = (uint8_t) IRP_AUDIO_STATUS__PLAYING;
                break; 
            case IRP_STATE__SEEKING__NEXT:
                payload[1] = (uint8_t) IBUS_STATE__SEEKING_NEXT;
                payload[2] = (uint8_t) IRP_AUDIO_STATUS__PLAYING;
                break; 
            case IRP_STATE__SEEKING__PREV:
                payload[1] = (uint8_t) IBUS_STATE__SEEKING_PREV;
                payload[2] = (uint8_t) IRP_AUDIO_STATUS__PLAYING;
                break; 
            case IRP_STATE__LOADING_DISC:
                payload[1] = (uint8_t) IBUS_STATE__LOADING_DISC;
                payload[2] = (uint8_t) IRP_AUDIO_STATUS__PLAYING;
                break;

            default:
                return IRP_ERROR_PARAMETER;
        }

        payload[4] = discs_present;
        payload[6] = current_disc;
        payload[7] = bcd_track_converter( current_track );
    }


    ibus_physical_send_message( IBUS_DEVICE__CDC,
                                IBUS_DEVICE__RAD,
                                payload, sizeof(payload) );

    return IRP_RETURN_OK;
}

/* See ibus-radio-protocol.h for details. */
irp_status_t irp_going_to_check_disc( const uint8_t disc,
                                      const uint8_t active_map,
                                      const irp_state_t goal )
{
    uint8_t payload[] = { 0x39, 0x09, 0, 0, 0, 0, 0, 0 };

    /* Make sure the disc is in range & also the active map is
     * less than or equal to the current disc. */
    if( (disc < 1) || (6 < disc) ||
        (0 != ((0xff << disc) & active_map)) ||
        ((IRP_STATE__STOPPED != goal) && (IRP_STATE__PLAYING != goal)) )
    {
        return IRP_ERROR_PARAMETER;
    }

    payload[2] = (IRP_STATE__PLAYING == goal) ? 0x09 : 0x02;
    payload[4] = active_map;
    payload[6] = disc;

    ibus_physical_send_message( IBUS_DEVICE__CDC,
                                IBUS_DEVICE__RAD,
                                payload, sizeof(payload) );

    return IRP_RETURN_OK;
}

/* See ibus-radio-protocol.h for details. */
irp_status_t irp_completed_disc_check( const uint8_t disc,
                                       const bool disc_present,
                                       const uint8_t active_map,
                                       const irp_state_t goal )
{
    uint8_t payload[] = { 0x39, 0x09, 0, 0, 0, 0, 0, 0 };

    /* Make sure the disc is in range & also the active map is
     * less than or equal to the current disc. */
    if( (disc < 1) || (6 < disc) ||
        (0 != ((0xff << disc) & active_map)) ||
        ((IRP_STATE__STOPPED != goal) && (IRP_STATE__PLAYING != goal)) )
    {
        return IRP_ERROR_PARAMETER;
    }

    /* If the disc_present flag is set, make sure that the
     * disc is marked in the active_map. */
    if( (true == disc_present) &&
        (0 == ((1 << (disc - 1)) & active_map)) )
    {
        return IRP_ERROR_PARAMETER;
    }

    /* If the disc_present flag is not set, make sure that the
     * disc is not marked in the active_map. */
    if( (false == disc_present) &&
        (0 != ((1 << (disc - 1)) & active_map)) )
    {
        return IRP_ERROR_PARAMETER;
    }

    payload[2] = (IRP_STATE__PLAYING == goal) ? 0x09 : 0x02;
    payload[3] = (true == disc_present) ? 0x00 : 0x08;
    payload[4] = active_map;
    payload[6] = disc;

    ibus_physical_send_message( IBUS_DEVICE__CDC,
                                IBUS_DEVICE__RAD,
                                payload, sizeof(payload) );


    return IRP_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  Used to send the Ready after reset the first time we send this message,
 *  just ready every other time.
 */
void _irp_send_device_status_ready( void )
{
    static bool sent_after_reset = false;

    uint8_t payload[2];

    payload[0] = 0x02;
    payload[1] = 0x00;
    if( false == sent_after_reset ) {
        payload[1] = 0x01;
        sent_after_reset = true;
    }

    ibus_physical_send_message( IBUS_DEVICE__CDC,
                                IBUS_DEVICE__BROADCAST_HIGH,
                                payload, sizeof(payload) );
}
