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
#ifndef __POPULATE_MESSAGE_H__
#define __POPULATE_MESSAGE_H__

#include <stdint.h>
#include <stddef.h>

#include <ibus-physical/ibus-physical.h>

#include "ibus-radio-protocol.h"

/**
 *  Used to populate the outgoing message with the specified information.
 *
 *  @param src the source of the message
 *  @param dst the destination of the message
 *  @param payload the message payload
 *  @param payload_length the length of the payload in bytes
 *  @param out the message to populate
 *  @param out_length the total outgoing message size available
 *
 *  @return IRP_RETURN_OK       on success
 *          IRP_ERROR_PARAMETER on failure
 */
irp_status_t populate_message( const ibus_device_t src,
                               const ibus_device_t dst,
                               const uint8_t *payload,
                               const size_t payload_length,
                               uint8_t *out,
                               const size_t out_length );
#endif
