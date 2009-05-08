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

#ifndef __IBUS_PHYSICAL_H__
#define __IBUS_PHYSICAL_H__

#include <stddef.h>
#include <stdint.h>

#define IBUS_MAX_MESSAGE_SIZE   32

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
bool ibus_physical_send_message( const uint8_t *msg, const size_t size );
#endif
