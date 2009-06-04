#ifndef __ARTIST_H__
#define __ARTIST_H__

#include <linked-list/linked-list.h>
#include <stdbool.h>
#include "database.h"

/**
 * Places an artist into the group linked list.  Artists will
 * be sorted alphabetically.
 * 
 * @param group pointer to the group which the artist should be found in
 *        or added to
 * @param artist NULL terminated string of the artist name
 * @param created_node will be updated to true if a node was created, or
 *        false if the node was not created.  This is only valid if the
 *        artist_node_t returned is not NULL.
 * 
 * @retrun pointer to the artist_node_t which is represented by the artist
 *         parameter.  Or NULL if there was an error.
 */
artist_node_t * find_or_create_artist( group_node_t * group,
        const char * artist,
        bool * created_node );

/**
 * Deletion function which should be referenced when using
 * the linked listed deleter function.
 */
void delete_artist(ll_node_t *node, volatile void *user_data);

#endif /* __ARTIST_H__ */
