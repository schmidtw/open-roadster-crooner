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
#ifndef __IPP_PROTOCOL_H__
#define __IPP_PROTOCOL_H__

#include <stddef.h>

/**
 *  Used to initialize the ibus phone system
 */
void ipp_init( void );

/**
 * Displays the string on the radio using the phone
 * interface.
 * 
 * @param string Pointer to the string which is to be
 *        displayed.
 * 
 * @return Number of characters from the string which
 *         were actually sent to the radio.
 */
size_t ibus_phone_display( char *string );

#endif /* __IPP_PROTOCOL_H__ */
