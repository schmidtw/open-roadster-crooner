#ifndef __GENERIC_H__
#define __GENERIC_H__

#include <linked-list/linked-list.h>
#include "database.h"

/**
 * The generic_compare_fn_t() function compares the two elements.
 * It returns an integer less than, equal to, or greater than
 * zero if element1 is found, respectively, to be less than, to match,
 * or be greater than element2.
 */
typedef int8_t (*generic_compare_fn_t)( const void * element1, const generic_node_t * element2 );

/**
 * The generic_create_fn_t() function creates a new generic_node_t
 * from the element.
 *
 * @param type the type the node is created for.
 *
 * @return new ll_node_t on SUCCESS.  NULL otherwise
 */
typedef ll_node_t * (*generic_create_fn_t)( const generic_node_types_t type, const void * element );

/**
 * Places an artist, album, song into the parent linked list.  Artists,
 * Albums, Songs will be sorted alphabetically.
 *
 * @param generic pointer to the group, artist, album which the element would
 *        be added to.
 * @param compare_fct function which is called to see if the existing element
 *        found in list is the same as the passed in element
 * @param new_node_fct function which is called if no element exists in the
 *        list
 * @param element data which is passed into the compare_fct and new_node_fct
 * @param created_node will be updated to true if a node was created, or
 *        false if the node was not created.  This is only valid if the
 *        album_node_t returned is not NULL.
 *
 * @retrun pointer to the album_node_t which is represented by the album
 *         parameter.  Or NULL if there was an error.
 */
generic_node_t * find_or_create_generic( generic_node_t * generic,
        generic_compare_fn_t compare_fct,
        generic_create_fn_t new_node_fct,
        void * element,
        bool * created_node );

/**
 * Create a new element of the specified type.
 *
 * @return ll_node_t on success.  NULL on failure.
 */
ll_node_t * get_new_generic_node( const generic_node_types_t type, const void * element );

/**
 * Compares the two elements as strings
 */
int8_t generic_compare( const void * element1, const generic_node_t * element2 );

/**
 * Compares to elements for songs
 */
int8_t song_compare( const void * element1, const generic_node_t * element2 );

/**
 * Deletion function which should be referenced when using
 * the linked listed deleter function.
 */
void delete_generic(ll_node_t *node, volatile void *user_data);

#endif /* __GENERIC_H__ */
