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
#ifndef __BCD_TRACK_CONVERTER_H__
#define __BCD_TRACK_CONVERTER_H__

#include <stdint.h>

/**
 *  Used to convert a track number into the BCD version of the
 *  track number taking into account that the valid track numbers
 *  are [00-99].
 *
 *  0 maps to 00
 *  1 maps to 01
 *  2 maps to 02
 *  ...
 *  99 maps to 99
 *  100 maps to 01
 *  101 maps to 02
 *  ...
 *  198 maps to 99
 *  199 maps to 01
 *  200 maps to 02
 *  ...
 *  255 maps to 57
 *
 *  @param track the number to convert
 *
 *  @return the [01-99] value to return.
 */
uint8_t bcd_track_converter( const uint8_t track );

#endif
