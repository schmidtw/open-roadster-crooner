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
#include <stddef.h>

#include "linked-list.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void ll_append( volatile ll_list_t *list, ll_node_t *node )
{
    if( (NULL != list) && (NULL != node) ) {
        if( NULL == list->tail ) {
            list->head = node;
            node->prev = NULL;
        } else {
            list->tail->next = node;
            node->prev = list->tail;
        }
        list->tail = node;
        node->next = NULL;
    }
}

void ll_prepend( volatile ll_list_t *list, ll_node_t *node )
{
    if( (NULL != list) && (NULL != node) ) {
        if( NULL == list->head ) {
            list->tail = node;
            node->next = NULL;
        } else {
            list->head->prev = node;
            node->next = list->head;
        }
        list->head = node;
        node->prev = NULL;
    }
}

void ll_iterate( volatile ll_list_t *list,
                 ll_ir_t (*iterator)(ll_node_t *node, volatile void *user_data),
                 void (*deleter)(ll_node_t *node, volatile void *user_data),
                 volatile void *user_data )
{
    if( NULL != list ) {
        /* If the iterator is NULL, don't call it, simply delete everything. */
        ll_node_t *n;
        ll_ir_t status;

        for( n = list->head; NULL != n; ) {
            ll_node_t *safe_n = n->next;

            status = LL_IR__DELETE_AND_CONTINUE;
            if( NULL != iterator ) {
                status = (*iterator)( n, user_data );
            }

            if( (LL_IR__DELETE_AND_CONTINUE == status) ||
                (LL_IR__DELETE_AND_STOP == status) )
            {
                ll_remove( list, n );
                if( NULL != deleter ) {
                    (*deleter)( n, user_data );
                }
            }

            if( (LL_IR__STOP == status) || (LL_IR__DELETE_AND_STOP == status) ) {
                return;
            }

            n = safe_n;
        }
    }
}

void ll_remove( volatile ll_list_t *list, ll_node_t *node )
{
    if( (NULL != list) && (NULL != node) ) {
        if( node == list->head ) {
            list->head = node->next;
            if( NULL != list->head ) {
                list->head->prev = NULL;
                node->next = NULL;
            }
        }

        if( node == list->tail ) {
            list->tail = node->prev;
            if( NULL != list->tail ) {
                list->tail->next = NULL;
                node->prev = NULL;
            }
        }

        /* We only need to check for one because since both 'end' conditions
         * result in both next & prev being NULLed out, they must both be
         * valid. */
        if( NULL != node->next ) {
            node->prev->next = node->next;
            node->next->prev = node->prev;
            node->prev = NULL;
            node->next = NULL;
        }
    }
}

ll_node_t* ll_remove_head( volatile ll_list_t *list )
{
    ll_node_t *node;

    node = NULL;

    if( NULL != list ) {

        node = list->head;

        ll_remove( list, node );
    }

    return node;
}

void ll_delete_list( volatile ll_list_t *list,
                     void (*deleter)(ll_node_t *node, volatile void *user_data),
                     volatile void *user_data )
{
    ll_iterate( list, NULL, deleter, user_data );
}

void ll_insert_after( volatile ll_list_t *list,
                      ll_node_t *node,
                      ll_node_t *after )
{
    if( (NULL != list) && (NULL != node) ) {
        if( NULL == after ) {
            ll_prepend( list, node );
        } else if( after == list->tail ) {
            ll_append( list, node );
        } else {
            node->next = after->next;
            after->next = node;
            node->prev = after;

            if( NULL != node->next ) {
                node->next->prev = node;
            }
        }
    }
}
/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
