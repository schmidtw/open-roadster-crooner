#ifndef __GENERIC_H__
#define __GENERIC_H__

#include <binary-tree-avl/binary-tree-avl.h>
#include "database.h"

typedef struct {
    generic_node_types_t type;
    char *string;
} generic_holder_t;

/**
 * The generic_compare_fn_t() function compares the two elements.
 * It returns an integer less than, equal to, or greater than
 * zero if element1 is found, respectively, to be less than, to match,
 * or be greater than element2.
 */
typedef int8_t (*generic_compare_fn_t)( const void * element1, const generic_node_t * element2 );

/**
 * Places an artist, album, song into the parent list.  Artists and
 * Albums will be sorted alphabetically.  Songs will be sorted by track.
 *
 * @param generic pointer to the group, artist, album which the element would
 *        be added to.
 * @param compare_fct function which is called to see if the existing element
 *        found in list is the same as the passed in element
 * @param element data which is passed into the compare_fct and new_node_fct
 * @param created_node will be updated to true if a node was created, or
 *        false if the node was not created.  This is only valid if the
 *        album_node_t returned is not NULL.
 *
 * @return pointer to the generic_node_t which is represented by the generic
 *         parameter.  Or NULL if there was an error.
 */
generic_node_t * find_or_create_generic( generic_node_t * generic,
        void * element,
        bool * created_node );

/**
 * Create a new element of the specified type.
 *
 * @note be careful changing the behavior of this function.  The element being passed
 *       in is very closely tied to the type
 *
 * @TODO: this function needs to be refactored
 *
 *
 * @param type The node type to create
 * @param element pointer to the data which is to be copied into the
 *        allocated block
 * @return bt_node_t on success.  NULL on failure.
 */
bt_node_t * get_new_generic_node( const generic_node_types_t type, const void * element );

/**
 * Compares the two elements as strings
 */
int8_t generic_compare( const void * element1, const void * element2 );

/**
 * Compares to elements for songs
 */
int8_t song_compare( const void * element1, const void * element2 );

/**
 * Deletion function which should be referenced when using
 * the list deleter function.
 */
void delete_generic(bt_node_t *node, volatile void *user_data);

#endif /* __GENERIC_H__ */
