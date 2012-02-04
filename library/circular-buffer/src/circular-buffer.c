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

#include "circular-buffer.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    void * head;
    void * tail;
    size_t element_number;
    size_t element_size;
    void * data;
} cb_struct_t;

void *cb_create_list( size_t size, uint8_t num )
{
    void *ptr;
    if(    ( 0 == size )
        || ( 0 == num ) ) {
        return NULL;
    }
    ptr = malloc( sizeof(cb_struct_t) + (size * num) );
    ((cb_struct_t *)ptr)->element_number = num;
    ((cb_struct_t *)ptr)->element_size = size;
    ((cb_struct_t *)ptr)->data = ptr + sizeof(cb_struct_t);
    cb_clear_list(ptr);

    return ptr;
}

void cb_clear_list( void * list )
{
    cb_struct_t *ptr = (cb_struct_t*)list;
    size_t num = ptr->element_number;
    size_t element_size = ptr->element_size;
    bzero(ptr->data, num*element_size);
    ptr->head = NULL;
    ptr->tail = NULL;
}

void cb_destroy_list( void ** list )
{
    if(    ( NULL != list )
        && ( NULL != *list ) ) {
        free( *list );
        *list = NULL;
    }
}

void *cb_peek_tail( void * list )
{
    return ((cb_struct_t*)list)->tail;
}

void __move_ptr_next_element( void ** ptr, void *data_start, size_t element_size, size_t element_num )
{
    void * calc = data_start + (element_size * (element_num-1));
    *ptr += element_size;
    if( *ptr > calc ) {
        *ptr -= element_size * element_num;
    }
}

void __move_ptr_prev_element( void ** ptr, void *data_start, size_t element_size, size_t element_num )
{
    *ptr -= element_size;
    if( *ptr < data_start ) {
        *ptr += element_num * element_size;
    }
}

bool cb_pop( void * list, void *item )
{
    cb_struct_t * cb_list = (cb_struct_t*)list;
    if(    ( NULL == list )
        || ( NULL == item )
        || ( NULL == cb_list->head ) ) {
        return false;
    }
    memcpy(item, cb_list->tail, cb_list->element_size);

    if( cb_list->head == cb_list->tail ) {
        cb_clear_list(list);
    } else {
        __move_ptr_prev_element(&cb_list->tail,
                cb_list->data,
                cb_list->element_size,
                cb_list->element_number);
    }

    return true;
}

bool cb_push( void * list, void * element )
{
    cb_struct_t * cb_list = (cb_struct_t*)list;
    if(    ( NULL == list )
        || ( NULL == element ) ) {
        return false;
    }

    if( NULL == cb_list->tail ) {
        cb_list->head = cb_list->data;
        cb_list->tail = cb_list->head;
    } else {
        __move_ptr_next_element(&cb_list->tail,
                cb_list->data,
                cb_list->element_size,
                cb_list->element_number);
        if( cb_list->head == cb_list->tail ) {
            __move_ptr_next_element(&cb_list->head,
                    cb_list->data,
                    cb_list->element_size,
                    cb_list->element_number);
        }
    }
    memcpy( cb_list->tail, element, cb_list->element_size );
    return true;
}
