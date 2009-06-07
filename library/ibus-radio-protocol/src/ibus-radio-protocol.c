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

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef enum {
    IBUS_DEVICE__BROADCAST_LOW  = 0x00,
    IBUS_DEVICE__CD_CHANGER     = 0x18,
    IBUS_DEVICE__RADIO          = 0x68,
    IBUS_DEVICE__BROADCAST_HIGH = 0xff
} ibus_device_t;

typedef enum {
    IRP_AUDIO_STATUS__STOPPED = 0x02,
    IRP_AUDIO_STATUS__PLAYING = 0x09,
    IRP_AUDIO_STATUS__PAUSED  = 0x0c
} irp_audio_status_t;

typedef enum {
    IRP_RX_CMD__STATUS          = 0x00,
    IRP_RX_CMD__STOP            = 0x01,
    IRP_RX_CMD__PAUSE           = 0x02,
    IRP_RX_CMD__PLAY            = 0x03,
    IRP_RX_CMD__FAST_PLAY       = 0x04,
    IRP_RX_CMD__SEEK            = 0x05,
    IRP_RX_CMD__CHANGE_DISC     = 0x06,
    IRP_RX_CMD__SCAN_DISC       = 0x07,
    IRP_RX_CMD__RANDOMIZE       = 0x08
} irp_rx_command_t;

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
    IRP_MAGAZINE_STATE__NORMAL      = 0x00,
    IRP_MAGAZINE_STATE__NO_DISCS    = 0x10,
    IRP_MAGAZINE_STATE__NO_MAGAZINE = 0x18
} irp_magazine_state_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static irp_status_t create_message( const ibus_device_t src,
                                    const ibus_device_t dst,
                                    const uint8_t *payload,
                                    const size_t payload_length,
                                    uint8_t *out,
                                    const size_t out_length );
static uint8_t bcd_track_converter( const uint8_t track );

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
    msg->disc = 0;

    done = false;
    while( false == done ) {
        ibus_io_msg_t *ibus_msg;
        ibus_msg = ibus_physical_get_message();

        if( IBUS_IO_STATUS__OK == ibus_msg->status ) {
            uint8_t checksum;
            int32_t i;


            checksum = 0;
            for( i = 0; i < ibus_msg->size; i++ ) {
                checksum ^= ibus_msg->buffer[i];
            }
            if( 0 == checksum ) {
                uint8_t dst = ibus_msg->buffer[2];
                if( (((uint8_t) IBUS_DEVICE__CD_CHANGER)     == dst) ||
                    (((uint8_t) IBUS_DEVICE__BROADCAST_LOW)  == dst) ||
                    (((uint8_t) IBUS_DEVICE__BROADCAST_HIGH) == dst) )
                {
                    uint8_t src = ibus_msg->buffer[0];

                    /* Message is to us */
                    if( ((uint8_t) IBUS_DEVICE__CD_CHANGER) != src ) {
                        /* Message is not from us */
                        uint8_t *payload = &ibus_msg->buffer[3];
                        uint8_t length = ibus_msg->buffer[1];

                        if( (3 == length) && (0x01 == *payload) ) {
                            msg->command = IRP_CMD__POLL;
                            done = true;
                        } else if( (5 == length) && (0x38 == *payload) ) {
                            switch( payload[1] ) {
                                case IRP_RX_CMD__STATUS:
                                    msg->command = IRP_CMD__GET_STATUS;
                                    done = true;
                                    break;
                                case IRP_RX_CMD__STOP:
                                    msg->command = IRP_CMD__STOP;
                                    done = true;
                                    break;
                                case IRP_RX_CMD__PAUSE:
                                    msg->command = IRP_CMD__PAUSE;
                                    done = true;
                                    break;
                                case IRP_RX_CMD__PLAY:
                                    msg->command = IRP_CMD__PLAY;
                                    done = true;
                                    break;
                                case IRP_RX_CMD__FAST_PLAY:
                                    if( 0 == payload[2] ) {
                                        msg->command = IRP_CMD__FAST_PLAY__REVERSE;
                                        done = true;
                                    } else if( 1 == payload[2] ) {
                                        msg->command = IRP_CMD__FAST_PLAY__FORWARD;
                                        done = true;
                                    }
                                    break;
                                case IRP_RX_CMD__SEEK:
                                    if( 0 == payload[2] ) {
                                        msg->command = IRP_CMD__SEEK__NEXT;
                                        done = true;
                                    } else if( 1 == payload[2] ) {
                                        msg->command = IRP_CMD__SEEK__PREV;
                                        done = true;
                                    }
                                    break;
                                case IRP_RX_CMD__CHANGE_DISC:
                                    msg->command = IRP_CMD__CHANGE_DISC;
                                    if( (0 < payload[2]) && (payload[2] < 7) ) {
                                        msg->disc = payload[2];
                                        done = true;
                                    }
                                    break;
                                case IRP_RX_CMD__SCAN_DISC:
                                    if( 0 == payload[2] ) {
                                        msg->command = IRP_CMD__SCAN_DISC__DISABLE;
                                        done = true;
                                    } else if( 1 == payload[2] ) {
                                        msg->command = IRP_CMD__SCAN_DISC__ENABLE;
                                        done = true;
                                    }
                                    break;
                                case IRP_RX_CMD__RANDOMIZE:
                                    if( 0 == payload[2] ) {
                                        msg->command = IRP_CMD__RANDOMIZE__DISABLE;
                                        done = true;
                                    } else if( 1 == payload[2] ) {
                                        msg->command = IRP_CMD__RANDOMIZE__ENABLE;
                                        done = true;
                                    }
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }
            }
        }

        ibus_physical_release_message( ibus_msg );
    }

    return IRP_RETURN_OK;
}

/* See ibus-radio-protocol.h for details. */
void irp_send_announce( void )
{
    static const uint8_t payload[] = { 0x02, 0x01 };
    uint8_t msg[6];

    create_message( IBUS_DEVICE__CD_CHANGER,
                    IBUS_DEVICE__BROADCAST_HIGH,
                    payload, sizeof(payload),
                    msg, sizeof(msg) );

    ibus_physical_send_message( msg, sizeof(msg) );
}

/* See ibus-radio-protocol.h for details. */
void irp_send_poll_response( void )
{
    static const uint8_t payload[] = { 0x02, 0x00 };
    uint8_t msg[6];

    create_message( IBUS_DEVICE__CD_CHANGER,
                    IBUS_DEVICE__BROADCAST_HIGH,
                    payload, sizeof(payload),
                    msg, sizeof(msg) );

    ibus_physical_send_message( msg, sizeof(msg) );
}

/* See ibus-radio-protocol.h for details. */
irp_status_t irp_send_normal_status( const irp_state_t device_state,
                                     const bool magazine_present,
                                     const uint8_t discs_present,
                                     const uint8_t current_disc,
                                     const uint8_t current_track )
{
    uint8_t payload[] = { 0x39, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t msg[12];

    if( false == magazine_present ) {
        if( (IRP_STATE__STOPPED != device_state) || (0 != discs_present) ||
            (0 != current_disc) || (0 != current_track) )
        {
            return IRP_ERROR_PARAMETER;
        }

        payload[1] = (uint8_t) IBUS_STATE__NO_MAGAZINE;
        payload[2] = (uint8_t) IRP_AUDIO_STATUS__STOPPED;
        payload[3] = (uint8_t) IRP_MAGAZINE_STATE__NO_MAGAZINE;
    } else if( 0 == discs_present ) {
        if( (IRP_STATE__STOPPED != device_state) ||
            (0 != current_disc) || (0 != current_track) )
        {
            return IRP_ERROR_PARAMETER;
        }

        payload[1] = (uint8_t) IBUS_STATE__LOADING_DISC;
        payload[2] = (uint8_t) IRP_AUDIO_STATUS__STOPPED;
        payload[3] = (uint8_t) IRP_MAGAZINE_STATE__NO_DISCS;
    } else {
        if( (0 == current_track) || (current_disc < 1) || (6 < current_disc) ||
            (0 == (discs_present && (1 << (current_disc - 1)))) )
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
                break; 
            case IRP_STATE__FAST_PLAYING__FORWARD:
            case IRP_STATE__FAST_PLAYING__REVERSE:
                payload[1] = (uint8_t) IBUS_STATE__FAST_PLAYING;
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
        }

        payload[4] = discs_present;
        payload[6] = current_disc;
        payload[7] = bcd_track_converter( current_track );
    }

    create_message( IBUS_DEVICE__CD_CHANGER,
                    IBUS_DEVICE__RADIO,
                    payload, sizeof(payload),
                    msg, sizeof(msg) );

    ibus_physical_send_message( msg, sizeof(msg) );

    return IRP_RETURN_OK;
}

/* See ibus-radio-protocol.h for details. */
irp_status_t irp_going_to_check_disc( const uint8_t disc,
                                      const uint8_t active_map )
{
    return irp_completed_disc_check( disc, false, active_map );
}

/* See ibus-radio-protocol.h for details. */
irp_status_t irp_completed_disc_check( const uint8_t disc,
                                       const bool disc_present,
                                       const uint8_t active_map )
{
    uint8_t payload[] = { 0x39, 0x09, 0x09, 0, 0, 0, 0, 0 };
    uint8_t msg[12];

    /* Make sure the disc is in range & also the active map is
     * less than or equal to the current disc. */
    if( (disc < 1) || (6 < disc) ||
        (0 != ((0xff << disc) & active_map)) )
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

    payload[3] = (true == disc_present) ? 0x08 : 0x00;
    payload[4] = active_map;
    payload[6] = disc;

    create_message( IBUS_DEVICE__CD_CHANGER,
                    IBUS_DEVICE__RADIO,
                    payload, sizeof(payload),
                    msg, sizeof(msg) );

    ibus_physical_send_message( msg, sizeof(msg) );

    return IRP_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static irp_status_t create_message( const ibus_device_t src,
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

static uint8_t bcd_track_converter( const uint8_t track )
{
    uint8_t rv;
    uint8_t tmp;

    tmp = track;
    while( 99 < tmp ) {
        tmp -= 99;
    }

    rv = tmp / 10;
    rv <<= 4;
    rv += (tmp % 10);

    return rv;
}
