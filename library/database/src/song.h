#ifndef __SONG_H__
#define __SONG_H__

#include <linked-list/linked-list.h>
#include "database.h"

/**
 * Places a song into the album linked list.  Songs will
 * be sorted by the track number and then alphabetically.
 * 
 * @param album pointer to the album which the song should be found in
 *        or added to
 * @param song NULL terminated string of the song name
 * @param track_number this tracks number
 * 
 * @retrun pointer to the song_node_t which is represented by the song
 *         parameter.  Or NULL if there was an error.
 */
song_node_t * find_or_create_song( album_node_t * group,
        const char * song, const uint8_t track_number );

void delete_song(ll_node_t *node, volatile void *user_data);

#endif /* __SONG_H__ */
