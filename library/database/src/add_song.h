#ifndef __ADD_SONG_H__
#define __ADD_SONG_H__

#include "database.h"
#include <media-interface/media-interface.h>

/**
 * Add a song to a group.  The song will be placed in the proper artist/album,
 * or create the node artist/album.
 * 
 * @note If the song already exists then a failure will occur.
 * @note The songs will be placed into the list ordered by the track number.
 *       Multiple track numbers will result in the the songs being orderer
 *       alphabetically.
 * 
 * @param group pointer to the group which the song should be added
 * @param metadata pointer to the data related to the song
 *       (artist/album/song names, track number)
 * @param play_fn function which is to be used when initially playing this song
 * @param file_location NULL terminated string representing the location of
 *        this song
 *        
 * @return pointer to the song on Success.  On failure NULL is returned.
 */
song_node_t * add_song_to_root( generic_node_t * group,
        media_metadata_t * metadata,
        media_play_fn_t play_fn,
        char * file_location );

#endif /* __ADD_SONG_H__ */
