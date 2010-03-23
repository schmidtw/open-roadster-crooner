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
#ifndef __MESSAGE_CONVERTER_H__
#define __MESSAGE_CONVERTER_H__

#include <stdbool.h>

#include <ibus-physical/ibus-physical.h>

#include "ibus-radio-protocol.h"

/**
 *  Used to convert a ibus message into a cd changer command or message.
 *
 *  @param in the input ibus message to process
 *  @param out the cd changer output command (if available)
 *
 *  @return true if the request results in a cd changer command or general ibus
 *               traffic
 *          false if the message originated from the cd changer or there was
 *                a problem processing the message (format, corruption, etc.)
 */
bool message_converter( const ibus_io_msg_t *in, irp_rx_msg_t *out );

#endif
