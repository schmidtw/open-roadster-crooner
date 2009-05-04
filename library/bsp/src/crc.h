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
#ifndef __CRC_H__
#define __CRC_H__

#include <stdint.h>
#include <stddef.h>

/**
 *  Used to calculate CRC-7 for an array of bytes.
 *
 *  G(x) = x^7 + x^3 + 1
 *
 *  @param message The array of bytes to CRC.
 *  @param length The number of bytes to CRC.
 *
 *  @return The CRC-7 value for the array of bytes.
 */
uint8_t crc7( const uint8_t *message, const size_t length );

/**
 *  Used to calculate CRC-16 for an array of bytes.
 *
 *  G(x) = x^16 + x^12 + x^5 + 1
 *
 *  @param message The array of bytes to CRC.
 *  @param length The number of bytes to CRC.
 *
 *  @return The CRC-16 value for the array of bytes.
 */
uint16_t crc16( const uint8_t *message, const size_t length );

#endif
