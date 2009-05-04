#ifndef __ALBUM_H__
#define __ALBUM_H__

#include <linked-list/linked-list.h>
#include "database.h"

/**
 * Places an album into the artist linked list.  Albums will
 * be sorted alphabetically.
 * 
 * @param artist pointer to the artist which the album should be found in
 *        or added to
 * @param album NULL terminated string of the album name
 * 
 * @retrun pointer to the album_node_t which is represented by the album
 *         parameter.  Or NULL if there was an error.
 */
album_node_t * find_or_create_album( artist_node_t * artist,
        const char * album );

/**
 * Deletion function which should be referenced when using
 * the linked listed deleter function.
 */
void delete_album(ll_node_t *node, volatile void *user_data);

#endif /* __ALBUM_H__ */
