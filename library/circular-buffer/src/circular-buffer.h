/*
 * Copyright (c) 2008  Weston Schmidt
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * In other words, you are welcome to use, share and improve this program.
 * You are forbidden to forbid anyone else to use, share and improve
 * what you give them.   Help stamp out software-hoarding!
 */

#ifndef __CIRCULAR_BUFFER_H_
#define __CIRCULAR_BUFFER_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Create a circular buffer which supports num elements of
 * size
 *
 * @return valid memory initialized and ready for use, NULL
 *         if there was a problem
 */
void *cb_create_list( size_t size, uint8_t num );

void cb_clear_list( void * list );

void cb_destroy_list( void ** list );

/**
 * @return NULL if there is no elements in the list
 */
void *cb_peek_tail( void * list );

/**
 * @item location where the tail element is copied to
 */
bool cb_pop( void * list, void * item );

/**
 * Pushes the element onto the list.  If there is
 * no room the oldest element will be overwritten.
 */
bool cb_push( void * list, void * element );

#endif /* __CIRCULAR_BUFFER_H_ */
