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
#ifndef __FAST_FILL_H__
#define __FAST_FILL_H__

/**
 *  Fills one of the 'fast' buffers of size MC_FAST_BUFFER_SIZE
 *  using integers with value 0xffffffff.
 *
 *  @note No error checking is done.
 *
 *  @param buffer the buffer to fill
 */
inline void fast_fill( const void *buffer );

#endif
