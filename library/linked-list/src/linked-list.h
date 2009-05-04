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

#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__

typedef enum {
    LL_IR__CONTINUE,
    LL_IR__DELETE_AND_CONTINUE,
    LL_IR__STOP,
    LL_IR__DELETE_AND_STOP
} ll_ir_t;

typedef struct ll_node_t ll_node_t;

struct ll_node_t {
    ll_node_t *next;
    ll_node_t *prev;
    void *data;
};

typedef struct {
    ll_node_t *head;
    ll_node_t *tail;
} ll_list_t;

/**
 *  Used to initialize the list.
 *
 *  @param list the list to initialize
 */
#define ll_init_list( list )    \
{                               \
    if( NULL != (list) ) {      \
        (list)->head = NULL;    \
        (list)->tail = NULL;    \
    }                           \
}

/**
 *  Used to initialize the list node.
 *
 *  @param node the node to initialize
 *  @param user_data the user data to associate with this node
 */
#define ll_init_node( node, user_data ) \
{                                       \
    if( NULL != (node) ) {              \
        (node)->next = NULL;            \
        (node)->prev = NULL;            \
        (node)->data = user_data;       \
    }                                   \
}

/**
 *  Used to append a node to a list.
 *
 *  @param list the list to append to
 *  @param node the node to append to the list
 */
void ll_append( volatile ll_list_t *list, ll_node_t *node );

/**
 *  Used to prepend a node to a list.
 *
 *  @param list the list to prepend to
 *  @param node the node to prepend to the list
 */
void ll_prepend( volatile ll_list_t *list, ll_node_t *node );

/**
 *  Used to iterate over a list and optionally delete nodes
 *  from the list during the iteration.
 *
 *  @note No memory is freed during this operation unless it is done
 *  so by the deleter function.
 *
 *  @todo Add more details to the iterator
 *
 *  @param list the list to iterate over
 *  @param iterator the function to call for each node
 *  @param deleter the function to call when deleting a node
 *  @param user_data the user_data to pass as an arguement to the deleter
 */
void ll_iterate( volatile ll_list_t *list,
                 ll_ir_t (*iterator)(ll_node_t *node, volatile void *user_data),
                 void (*deleter)(ll_node_t *node, volatile void *user_data),
                 volatile void *user_data );

/**
 *  Used to remove a node from the list.
 *
 *  @param list the list to remove from
 *  @param node the node to remove from the list
 */
void ll_remove( volatile ll_list_t *list, ll_node_t *node );

/**
 *  Used to remove the head node from the list and return it.
 *
 *  @param list the list to remove from
 *
 *  @return the head node, or NULL if the list is invlid or empty
 */
ll_node_t* ll_remove_head( volatile ll_list_t *list );

/**
 *  Used to delete all the nodes in a list.
 *
 *  @note No memory is freed during this operation unless it is done
 *  so by the deleter function.
 *
 *  @param the list to empty
 *  @param deleter the function to call when deleting a node
 *  @param user_data optional user data to pass the deleter function
 */
void ll_delete_list( volatile ll_list_t *list,
                     void (*deleter)(ll_node_t *node, volatile void *user_data),
                     volatile void *user_data );

/**
 *  Used to insert a node at an arbitrary point in a list.
 *
 *  @param list the list to insert into
 *  @param node the node to insert
 *  @param after the node to insert after, NULL to insert at the head
 */
void ll_insert_after( volatile ll_list_t *list,
                      ll_node_t *node,
                      ll_node_t *after );

#endif
