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

#ifndef __IBUS_DEVICE_H__
#define __IBUS_DEVICE_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define IBUS_MAX_MESSAGE_SIZE   64
#define IBUS_MAX_PAYLOAD_SIZE   (IBUS_MAX_MESSAGE_SIZE - 4)

typedef enum {
    IBUS_IO_STATUS__OK,
    IBUS_IO_STATUS__PARITY_ERROR,
    IBUS_IO_STATUS__BUFFER_OVERRUN_ERROR
} ibus_io_msg_status_t;

typedef struct {
    ibus_io_msg_status_t status;
    size_t size;
    uint8_t buffer[IBUS_MAX_MESSAGE_SIZE];
} ibus_io_msg_t;

typedef enum {
    IBUS_DEVICE__AB             = 0xa4, /* Airbag */
    IBUS_DEVICE__AHL            = 0x66, /* Adaptive headlight control unit */
    IBUS_DEVICE__ANZV           = 0xe7, /* Display group */
    IBUS_DEVICE__ASC            = 0x56, /* Anti-lock braking system with ASC */
    IBUS_DEVICE__BMBT           = 0xf0, /* On-board monitor control panel */
    IBUS_DEVICE__CCM            = 0x30, /* Check control module */
    IBUS_DEVICE__CDC            = 0x18, /* CD changer */
    IBUS_DEVICE__CDCD           = 0x76, /* CD changer (DIN) */
    IBUS_DEVICE__CID            = 0x46, /* Central information display */
    IBUS_DEVICE__CSU            = 0xf5, /* Center switch control unit */
    IBUS_DEVICE__CVM            = 0x9c, /* Convertible top module */
    IBUS_DEVICE__DIA            = 0x3f, /* Diagnostic */
    IBUS_DEVICE__DME            = 0x12, /* Digital motor electronics (ECU) */
    IBUS_DEVICE__DIS            = 0x3f, /* Display */
    IBUS_DEVICE__DSP            = 0x6a, /* Digital sound processor / amplifier */
    IBUS_DEVICE__DSPC           = 0xea, /* Digital signal processor controller */
    IBUS_DEVICE__EGS            = 0x32, /* Electronic transmission control */
    IBUS_DEVICE__EHC            = 0xac, /* Electronic height control */
    IBUS_DEVICE__EKM            = 0x02, /* Body electronics module */
    IBUS_DEVICE__EKP            = 0x65, /* Electronically controlled fuel pump control unit */
    IBUS_DEVICE__EWS            = 0x44, /* Immobiliser control unit */
    IBUS_DEVICE__FBZV           = 0x40, /* Remote central locking */
    IBUS_DEVICE__FHK            = 0xa7, /* Rear climate control */
    IBUS_DEVICE__FID            = 0xa0, /* Rear multi-information display */
    IBUS_DEVICE__FMBT           = 0x47, /* Rear monitor controls */
    IBUS_DEVICE__FUM            = 0x28, /* Radio clock control module */
    IBUS_DEVICE__GLO            = 0xbf, /* Global broadcast address */
    IBUS_DEVICE__GM             = 0x00, /* Body module */
    IBUS_DEVICE__GR             = 0xa6, /* Cruise control */
    IBUS_DEVICE__GT             = 0x3b, /* Graphics driver */
    IBUS_DEVICE__GTF            = 0x43, /* Rear graphics driver */
    IBUS_DEVICE__HAC            = 0x9a, /* Headlight aim control */
    IBUS_DEVICE__HKM            = 0x24, /* Trunk/boot lid control unit */
    IBUS_DEVICE__IHK            = 0x5b, /* Heater / AC control */
    IBUS_DEVICE__IKE            = 0x80, /* Instrument cluster electronics */
    IBUS_DEVICE__IRIS           = 0xe0, /* Integrated radio information system */
    IBUS_DEVICE__ISP            = 0xe8, /* Intelligent side protection */
    IBUS_DEVICE__LCM            = 0xd0, /* Light check module */
    IBUS_DEVICE__LOC            = 0xff, /* Local group broadcast address */
    IBUS_DEVICE__LWS            = 0x57, /* Steering angle sensor */
    IBUS_DEVICE__MFL            = 0x50, /* Multi-function steering wheel */
    IBUS_DEVICE__MID            = 0xc0, /* Multi-information display */
    IBUS_DEVICE__MID1           = 0x01, /* Multi-information display (1st generation) */
    IBUS_DEVICE__MML            = 0x51, /* Mirror memory - left */
    IBUS_DEVICE__MMR            = 0x9b, /* Mirror memory - right */
    IBUS_DEVICE__NAC            = 0xa8, /* Navigation system (Chinese) */
    IBUS_DEVICE__NAJ            = 0xbb, /* Navigation system (Japanese) */
    IBUS_DEVICE__NAV            = 0x7f, /* Navigation system (Europe) */
    IBUS_DEVICE__ONL            = 0x67, /* Unknown */
    IBUS_DEVICE__PDC            = 0x60, /* Park distance control */
    IBUS_DEVICE__PIC            = 0xf1, /* Programmable Controller (custom device) */
    IBUS_DEVICE__RAD            = 0x68, /* Radio */
    IBUS_DEVICE__RCSC           = 0x81, /* Revolution counter/steering column */
    IBUS_DEVICE__RDC            = 0x70, /* Tire pressure monitoring system */
    IBUS_DEVICE__RLS            = 0xe8, /* Rain and headlight sensor */
    IBUS_DEVICE__SDRS           = 0x73, /* Satelite digital radio */
    IBUS_DEVICE__SES            = 0xb0, /* Voice recognition unit */
    IBUS_DEVICE__SHD            = 0x08, /* Sunroof */
    IBUS_DEVICE__SM             = 0x72, /* Seat memory */
    IBUS_DEVICE__SMAD           = 0xda, /* Passenger seat memory */
    IBUS_DEVICE__SOR            = 0x74, /* Seat occupancy recognition unit */
    IBUS_DEVICE__STH            = 0x6b, /* Park heating */
    IBUS_DEVICE__TCU            = 0xca, /* Telematics Control Unit */
    IBUS_DEVICE__TEL            = 0xc8, /* Telephone */
    IBUS_DEVICE__VID            = 0xed, /* Video module */
    IBUS_DEVICE__ZKE            = 0x09, /* Central body electronics */
    IBUS_DEVICE__DEBUG          = 0xfe, /* Crooner debug to the iBus loggers */
    IBUS_DEVICE__BROADCAST_LOW  = 0x00, /* Will be removed in the future */
    IBUS_DEVICE__BROADCAST_MID  = 0xbf, /* Will be removed in the future */
    IBUS_DEVICE__BROADCAST_HIGH = 0xff  /* Will be removed in the future */
} ibus_device_t;

/**
 *  Used to initalize the iBus physical layer.
 */
void ibus_physical_init( void );

/**
 *  Used to shutdown & destroy of the system.
 *
 *  @note Don't call this in a real embedded system because it won't work right.
 */
void ibus_physical_destroy( void );

/**
 *  Used to get the next ibus message off the iBus.  This call
 *  will block the current thread until a new message is received.
 *
 *  @note the message must be released
 *
 *  @return the latest message
 */
ibus_io_msg_t* ibus_physical_get_message( void );

/**
 *  Used to release the ibus_io_msg that was received.
 *
 *  @note NULL msg values are ignored
 *
 *  @param msg the message to release
 */
void ibus_physical_release_message( ibus_io_msg_t *msg );

/**
 *  Used to send a message over the iBus.  This call only blocks long
 *  enough to get a buffer to queue the message with.
 *
 *  @note The message may go out at some later point in time.
 *  @note The message pointer passed in is copied, and is owned by the caller.
 *
 *  @return true on success, false otherwise
 */
bool ibus_physical_send_message( const ibus_device_t src,
                                 const ibus_device_t dst,
                                 const uint8_t *payload,
                                 const size_t payload_length );
#endif
