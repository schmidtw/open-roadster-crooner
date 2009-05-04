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
#ifndef __DELAY_H__
#define __DELAY_H__

#include <stdint.h>

/**
 *  Delays a specified number of clock cycles before returning.
 *
 *  @note The number of cycles that is delayed shall be no fewer than
 *        the number specified, but it will be slightly over.
 *
 *  @param cycles the numbfer of clock cycles to wait
 */
void delay_cycles( const uint32_t cycles );

/**
 *  Delays a specified number of nanoseconds before returning.
 *
 *  @note The number of nanoseconds that is delayed shall be no
 *        fewer than the number specified, but it will be slightly over.
 *
 *  @param ns the number of nanoseconds to wait
 */
void delay_time( const uint64_t ns );
#endif
