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
 * @param size of the element
 * @param num number of elements which the circular buffer needs to support
 *
 * @return valid memory initialized and ready for use, NULL
 *         if there was a problem
 */
void *cb_create_list( size_t size, uint8_t num );

/**
 * Clears the circular buffer list resulting an empty
 * buffer
 *
 * @param list pointer to the circular buffer
 */
void cb_clear_list( void * list );


/**
 * Deletes and frees the circular buffer
 *
 * @param list double pointer to the circular buffer.  *list will be updated
 *             to point to NULL during this routine
 */
void cb_destroy_list( void ** list );

/**
 * Shows the last element pushed into the circular buffer
 *
 * @param list pointer to the circular buffer
 * @return NULL if there is no elements in the list
 */
void *cb_peek_tail( void * list );

/**
 * Removes the last pushed element from the circular buffer and copies
 * the data into the *item pointer
 *
 * @param list pointer to the circular buffer
 * @param item location where the tail element is copied to.  NULL will still
 *        perform the pop action, but no copy of the popped object will be
 *        saved.
 *
 * @return true if an element was removed from the list, false otherwise
 */
bool cb_pop( void * list, void * item );

/**
 * Pushes the element onto the list.  If there is
 * no room the oldest element will be overwritten.
 *
 * @param list pointer to the circular buffer
 * @param element to be pushed onto the buffer
 *
 * @note if either param passed in is NULL then there SHALL be
 *       no modifications to the buffer
 */
void cb_push( void * list, void * element );

#endif /* __CIRCULAR_BUFFER_H_ */
