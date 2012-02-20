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

#include "binary-tree-avl.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define max(X,Y) ((X) > (Y) ? (X) : (Y))
#define min(X,Y) ((X) < (Y) ? (X) : (Y))


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
static int __height( bt_node_t * node );
static void __correct_height( bt_node_t **tree_root );
static void __single_rotate_right( bt_node_t **node );
static void __single_rotate_left( bt_node_t **node );
static void __double_rotate_right( bt_node_t **node );
static void __double_rotate_left( bt_node_t **node );
static bt_ir_t __iterate( bt_node_t *node,
                 bt_ir_t (*iterator)(bt_node_t *node,
                     void *user_data),
                 void (*deleter)(bt_node_t *node,
                         void *user_data),
                 void *user_data );
static void __delete_list( bt_node_t *list,
        void (*deleter)(bt_node_t *node, void *user_data),
        void *user_data );
static bt_node_t* __get_next( bt_list_t *list, bt_node_t *node,
        void *data, bt_get_t next, bool *isFound );
static bt_node_t* __get_head( bt_node_t *node );
static bt_node_t* __get_tail( bt_node_t *node );

/* Insert and Remove balance as the go */
static bool __insert( bt_node_t *node_to_add, bt_node_t **tree_root, bt_compare comparer );
static bool __remove( void *data, bt_node_t **tree_root,
        bt_compare comparer,
        void (*deleter)(bt_node_t *node, void *user_data),
        void *user_data);

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
bool bt_add( bt_list_t *list, bt_node_t *node )
{
    if(    ( NULL != list )
        && ( NULL != node ) ) {
        return __insert(node, &(list->root), list->comparer);
    }
    return false;
}

bt_node_t* bt_find( bt_list_t *list, void *data )
{
    if(    ( NULL != list )
        && ( NULL != data ) ) {
        bt_node_t *cur = list->root;
        while( NULL != cur ) {
            int8_t compare_rslt = list->comparer( data, cur->data );
            if( 0 == compare_rslt ) {
                return cur;
            } else if( -1 == compare_rslt ) {
                cur = cur->left;
            } else {
                cur = cur->right;
            }
        }
    }
    return NULL;
}

bt_node_t* bt_get( bt_list_t *list, bt_node_t *node, bt_get_t next )
{
    bt_node_t* rv = NULL;
    if(    ( NULL != list )
        && ( NULL != node )
        && ( NULL != list->root ) )
    {
        bool isFound = false;
        rv = __get_next( list, list->root, node->data, next, &isFound );
    }
    return rv;
}

bt_node_t* bt_get_head( bt_list_t *list )
{
    bt_node_t* rv = NULL;
    if(    ( NULL != list )
        && ( NULL != list->root ) ) {
        rv = __get_head(list->root);
    }
    return rv;
}

bt_node_t* bt_get_tail( bt_list_t *list )
{
    bt_node_t* rv = NULL;
    if(    ( NULL != list )
        && ( NULL != list->root ) ) {
        rv = __get_tail(list->root);
    }
    return rv;
}

void bt_iterate( bt_list_t *list,
                 bt_ir_t (*iterator)(bt_node_t *node,
                     void *user_data),
                 void (*deleter)(bt_node_t *node,
                         void *user_data),
                 void *user_data )
{
    if(    ( NULL != list )
        && ( NULL != iterator )
        && ( NULL != list->root ) )
    {
        __iterate( list->root, iterator, deleter, user_data );
    }
}

void bt_delete_list( bt_list_t *list,
                     void (*deleter)(bt_node_t *node, void *user_data),
                     void *user_data )
{
    if(    ( NULL != list )
        && ( NULL != list->root )
        && ( NULL != deleter ) )
    {
        __delete_list( list->root, deleter, user_data );
        list->root = NULL;
    }
}

void bt_remove( bt_list_t *list, void *data,
        void (*deleter)(bt_node_t *node, void *user_data),
        void *user_data )
{
    if(    ( NULL != list )
        && ( NULL != data )
        && ( NULL != deleter ) ) {
        __remove(data, &list->root, list->comparer, deleter, user_data);
    }
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static int __height( bt_node_t * node ) {
    if( NULL == node ) {
        return -1;
    } else {
        return node->height;
    }
}

static void __correct_height( bt_node_t **tree_root )
{
    int diff = __height((*tree_root)->left) -
               __height((*tree_root)->right);
    if( 2 == diff ) {
        /* The left node is out of wack */
        if( __height( (*tree_root)->left->left ) > __height( (*tree_root)->left->right ) ) {
            __single_rotate_left(tree_root);
        } else {
            __double_rotate_left(tree_root);
        }
    } else if ( -2 == diff ) {
        /* The right node is out of wack */
        if( __height( (*tree_root)->right->right ) > __height( (*tree_root)->right->left ) ) {
            __single_rotate_right(tree_root);
        } else {
            __double_rotate_right(tree_root);
        }
    }
    (*tree_root)->height = max( __height((*tree_root)->left),
                                __height((*tree_root)->right)) + 1;
}

static bool __insert( bt_node_t *node_to_add, bt_node_t **tree_root, bt_compare comparer )
{
    if( NULL == *tree_root ) {
        (*tree_root) = node_to_add;
        node_to_add->height = 0;
    } else {
        int8_t compare_rslt = comparer(node_to_add->data, (*tree_root)->data);
        if( 0 > compare_rslt ) {
            /* Add node is less than this node */
            if( !__insert(node_to_add, &(*tree_root)->left, comparer) ) {
                return false;
            }
        } else if( 0 < compare_rslt ) {
            /* Add node is greater than this node */
            if( !__insert(node_to_add, &(*tree_root)->right, comparer) ) {
                return false;
            }
        } else { /* 0 == compare_rslt */
            return false;
        }
        __correct_height(tree_root);
    }
    return true;
}


/**
 * Moves the left child to be the root,  the root becomes the
 * right child, and all other references are updated
 *
 * @note Because of the way the AVL works, this function SHALL
 *       only be called if the orig_root has a left child
 */
static void __single_rotate_left( bt_node_t **node )
{
    bt_node_t *orig_root = *node;
    bt_node_t *new_root = orig_root->left;
    orig_root->left = new_root->right;
    new_root->right = orig_root;

    orig_root->height = max(__height(orig_root->left),
                     __height(orig_root->right)) + 1;
    new_root->height = max(__height(new_root->left),
                     orig_root->height) + 1;

    *node = new_root;
}

/**
 * Moves the right child to be the root,  the root becomes the
 * left child, and all other references are updated
 *
 * @note Because of the way the AVL works, this function SHALL
 *       only be called if the orig_root has a right child
 */
static void __single_rotate_right( bt_node_t **node )
{
    bt_node_t *orig_root = *node;
    bt_node_t *new_root = orig_root->right;
    orig_root->right = new_root->left;
    new_root->left = orig_root;

    orig_root->height = max(__height(orig_root->left),
                     __height(orig_root->right)) + 1;
    new_root->height = max(__height(new_root->left),
                     orig_root->height) + 1;

    *node = new_root;
}

static void __double_rotate_left( bt_node_t **node )
{
    __single_rotate_right(&(*node)->left);
    __single_rotate_left(node);
}

static void __double_rotate_right( bt_node_t **node )
{
    __single_rotate_left(&(*node)->right);
    __single_rotate_right(node);
}

static bt_ir_t __iterate( bt_node_t *node,
                 bt_ir_t (*iterator)(bt_node_t *node,
                     void *user_data),
                 void (*deleter)(bt_node_t *node,
                         void *user_data),
                 void *user_data )
{
    bt_ir_t rv = BT_IR__CONTINUE;
    if( NULL != node->left ) {
        rv = __iterate(node->left, iterator, deleter, user_data);
    }
    if( BT_IR__STOP != rv ) {
        rv = iterator(node, user_data);
    }
    if(    ( BT_IR__STOP != rv )
        && ( NULL != node->right ) ) {
        rv = __iterate(node->right, iterator, deleter, user_data);
    }
    return rv;
}

static void __delete_list( bt_node_t *node,
        void (*deleter)(bt_node_t *node, void *user_data),
        void *user_data )
{
    if( NULL != node->left ) {
        __delete_list(node->left, deleter, user_data);
        node->left = NULL;
    }
    if( NULL != node->right ) {
        __delete_list(node->right, deleter, user_data);
        node->right = NULL;
    }
    deleter(node, user_data);
}

static bt_node_t* __get_next( bt_list_t *list, bt_node_t *node,
        void *data, bt_get_t next, bool *isFound )
{
    int8_t compare_rslt = list->comparer( data, node->data );
    bt_node_t *rv = NULL;
    if( 0 > compare_rslt ) {
        if( NULL == node->left ) {
        /* Node we are looking for is less than this node */
            *isFound = true;
        } else {
            rv = __get_next(list, node->left, data, next, isFound);
            if(    (BT_GET__NEXT == next)
                && (false == *isFound) ) {
                rv = node;
                *isFound = true;
            }
        }
    } else if( 0 < compare_rslt ) {
        /* Node we are looking for is greater than this node */
        if( NULL == node->right ) {
            *isFound = true;
        } else {
            rv = __get_next(list, node->right, data, next, isFound);
            if(    (BT_GET__PREVIOUS == next)
                && (false == *isFound) ) {
                rv = node;
                *isFound = true;
            }
        }
    } else {
        /* This is the node we are looking for.
         * Which means we now need to go right, then left*
         * until we hit a node which is null to get the next node
         */
        if( BT_GET__NEXT == next ) {
            if( NULL != node->right ) {
                *isFound = true;
                rv = node->right;
                while( NULL != rv->left ) {
                    rv = rv->left;
                }
            }
        } else {
            /* BT_GET__PREVIOUS */
            if( NULL != node->left ) {
                *isFound = true;
                rv = node->left;
                while( NULL != rv->right ) {
                    rv = rv->right;
                }
            }
        }
    }
    return rv;
}

static bt_node_t* __get_head( bt_node_t *node )
{
    while(1) {
        if( NULL != node->left ) {
            node = node->left;
        } else {
            break;
        }
    }
    return node;
}

static bt_node_t* __get_tail( bt_node_t *node )
{
    while(1) {
        if( NULL != node->right ) {
            node = node->right;
        } else {
            break;
        }
    }
    return node;
}

static bool __remove( void *data, bt_node_t **tree_root,
        bt_compare comparer,
        void (*deleter)(bt_node_t *node, void *user_data),
        void *user_data)
{
    int8_t result;
    bool rv = true;
    if( NULL == *tree_root ) {
        return false;
    }

    result = comparer( data, (*tree_root)->data );
    if( 0 == result ) {
        bt_node_t *tmp = *tree_root;
        if(    (NULL == tmp->left)
            && (NULL == tmp->right) ) {
            *tree_root = NULL;
        } else if( NULL == tmp->right ) {
            *tree_root = tmp->left;
        } else if( NULL == tmp->left ) {
            *tree_root = tmp->right;
        } else {
            /* The left and right nodes are both in use.
             * During this removal call, this is the only
             * time this case will be hit
             */
            bt_node_t *new_head = __get_head(tmp->right);
            __remove( new_head->data, &tmp->right, comparer, NULL, user_data );
            new_head->left = tmp->left;
            new_head->right = tmp->right;
            *tree_root = new_head;
        }
        if( NULL != deleter ) {
            deleter( tmp, user_data );
        }
    } else if( -1 == result ) {
        rv = __remove( data, &(*tree_root)->left, comparer, deleter, user_data );
    } else { /* 1 == result */
        rv = __remove( data, &(*tree_root)->right, comparer, deleter, user_data );
    }
    if(    (rv)
        && (NULL != *tree_root) )
    {
        __correct_height(tree_root);
    }
    return rv;
}
