#ifndef __ARTIST_H__
#define __ARTIST_H__

#include <linked-list/linked-list.h>
#include "database.h"

/**
 * Places an artist into the group linked list.  Artists will
 * be sorted alphabetically.
 * 
 * @param group pointer to the group which the artist should be found in
 *        or added to
 * @param artist NULL terminated string of the artist name
 * 
 * @retrun pointer to the artist_node_t which is represented by the artist
 *         parameter.  Or NULL if there was an error.
 */
artist_node_t * find_or_create_artist( group_node_t * group,
        const char * artist );

/**
 * Deletion function which should be referenced when using
 * the linked listed deleter function.
 */
void delete_artist(ll_node_t *node, volatile void *user_data);

#endif /* __ARTIST_H__ */
