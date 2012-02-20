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

#ifndef __BINARY_TREE_AVL_H__
#define __BINARY_TREE_AVL_H__

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BT_IR__CONTINUE,
    BT_IR__STOP
} bt_ir_t;

typedef enum {
    BT_GET__NEXT,
    BT_GET__PREVIOUS
} bt_get_t;

typedef struct bt_node_t bt_node_t;

struct bt_node_t {
    bt_node_t *left;
    bt_node_t *right;
    int height;
    void *data;
};

typedef int8_t (*bt_compare)( void * d1, void * d2 );

typedef struct {
    bt_node_t *root;
    bt_compare comparer;
} bt_list_t;

/**
 *  Used to initialize the list.
 *
 *  @param list the list to initialize
 *  @param compare_fn the function to use for determining how to populate the
 *         binary tree
 */
#define bt_init_list( list, compare_fn )    \
{                                           \
    if( NULL != (list) ) {                  \
        (list)->root = NULL;                \
        (list)->comparer = (compare_fn);    \
    }                                       \
}

#define bt_set_compare( list, compare_fn )  \
{                                           \
    if( NULL != (list) ) {                  \
        (list)->comparer = (compare_fn);    \
    }                                       \
}

/**
 *  Used to initialize the list node.
 *
 *  @param node the node to initialize
 *  @param user_data the user data to associate with this node
 */
#define bt_init_node( node, user_data ) \
{                                       \
    if( NULL != (node) ) {              \
        (node)->left = NULL;            \
        (node)->right = NULL;           \
        (node)->data = user_data;       \
    }                                   \
}

/**
 *  Used to add a node to a list.  If a node is found
 *  that is the same as the provided node, the new
 *  node will NOT be added to the list.
 *
 *  @param list the list to append to
 *  @param node the node to append to the list
 *
 *  @return true if the node was added, false if the node is already in the
 *          tree
 */
bool bt_add( bt_list_t *list, bt_node_t *node );

/**
 *  Find a node in the tree and return a pointer to the node.
 *
 *  @note the compare function passed into the init of the bt_list
 *        will be used to for the search
 *
 *  @param list the list to search
 *  @param data the data to compare against the tree data to find the
 *         matching node
 *  @return pointer to the node which matches the data passed in, or NULL
 *          if the data doesn't match any node in the tree.
 */
bt_node_t* bt_find( bt_list_t *list, void *data );

/**
 * Gets the node requested by next.  Looks in the binary tree list for node.
 *
 * @param list the list to search
 * @param node the node which is the base of the search (for next/previous)
 * @param next indicates the get operation
 * @return pointer to the next node in the list, NULL if there is no next node
 */
bt_node_t* bt_get( bt_list_t *list, bt_node_t *node, bt_get_t next );

/**
 * Get the head/tail of the list
 *
 * @param list pointer to the list to search
 */
bt_node_t* bt_get_head( bt_list_t *list );
bt_node_t* bt_get_tail( bt_list_t *list );

/**
 *  Used to iterate over a list and optionally delete nodes
 *  from the list during the iteration.
 *
 *  The iterator will iterate from smallest value to largest in a depth first
 *  manner
 *
 *  @note No memory is freed during this operation unless it is done
 *  so by the deleter function.
 *
 *  @param list the list to iterate over
 *  @param iterator the function to call for each node
 *  @param deleter the function to call when deleting a node
 *  @param user_data the user_data to pass as an argument to the deleter
 */
void bt_iterate( bt_list_t *list,
                 bt_ir_t (*iterator)(bt_node_t *node, void *user_data),
                 void (*deleter)(bt_node_t *node, void *user_data),
                 void *user_data );

/**
 *  Used to remove a node from the list.
 *
 *  @param list the list to remove from
 *  @param data pointer of data to be used to remove the node by
 *  @param deleter function pointer which is to be called with the bt_node_t*
 *         which is to be deleted
 *  @param user_data the user_data to pass as an argument to the deleter
 */
void bt_remove( bt_list_t *list, void *data,
        void (*deleter)(bt_node_t *node, void *user_data),
        void *user_data );

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
void bt_delete_list( bt_list_t *list,
                     void (*deleter)(bt_node_t *node, void *user_data),
                     void *user_data );

#endif
