#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include <binary-tree-avl/binary-tree-avl.h>
#include <media-interface/media-interface.h>

#define MAX_ROOT_NAME            20
#define MAX_ROOT_NAME_W_NULL     (MAX_ROOT_NAME + 1)
#define MAX_ARTIST_NAME          MEDIA_ARTIST_LENGTH
#define MAX_ARTIST_NAME_W_NULL   (MAX_ALBUM_TITLE + 1)
#define MAX_ALBUM_TITLE          MEDIA_ALBUM_LENGTH
#define MAX_ALBUM_TITLE_W_NULL   (MAX_ALBUM_TITLE + 1)
#define MAX_SONG_TITLE           MEDIA_TITLE_LENGTH
#define MAX_SONG_TITLE_W_NULL    (MAX_SONG_TITLE + 1)

#define MAX_SHORT_FILENAME                 12
#define MAX_SHORT_FILENAME_W_NULL          (MAX_SHORT_FILENAME + 1)
#define MAX_SHORT_FILENAME_PATH            260
#define MAX_SHORT_FILENAME_PATH_W_NULL     (MAX_SHORT_FILENAME_PATH + 1)

typedef struct {
    bt_list_t children;  // Could be artists, album, song
    uint32_t index_songs_start; // The index start of the songs which are children of this list
    uint32_t index_songs_stop;   // The index stop (last) of the songs which are children of this list
} index_song_node_t;

typedef enum {
    GNT_GENERIC_CREATE_NODE = -20,
    GNT_SONG_CREATE_NODE = -10,
    GNT_SONG_SEARCH_NODE = -9,
    GNT_ROOT = 0,
    GNT_ARTIST = 1,
    GNT_ALBUM = 2,
    GNT_SONG = 3
} generic_node_types_t;

typedef struct generic_node {
    generic_node_types_t type;
    union {
        char root[MAX_ROOT_NAME_W_NULL];
        char artist[MAX_ARTIST_NAME_W_NULL];
        char album[MAX_ALBUM_TITLE_W_NULL];
        char song[MAX_SONG_TITLE_W_NULL];
    } name;
    bt_node_t node;
    union {
        uint32_t song_index; // Used when a song_node_t
        index_song_node_t list; // Used when group/artist/album node
    } i;
    // Parent -- Could be group, artist, album
    struct generic_node * parent;
} generic_node_t;

typedef struct {
    generic_node_t d;
    char file_location[MAX_SHORT_FILENAME_PATH_W_NULL];
    media_gain_t gain;

    uint16_t track_number;
    media_command_fn_t command_fn;
    media_play_fn_t play_fn;
} song_node_t;

typedef enum {
    DT_NEXT,
    DT_PREVIOUS,
    DT_RANDOM
} db_traverse_t;

typedef enum {
    DL_SONG,
    DL_ALBUM,
    DL_ARTIST
} db_level_t;

typedef enum {
    DS_SUCCESS,
    DS_END_OF_LIST,
    DS_FAILURE
} db_status_t;

/**
 * Sets up the database function pointers for use.
 *
 * @param mi the media interface to associate with the database
 * 
 * @return true if the database initialized properly.  False if the passed
 *              in strings are too long or if an error occurs while
 *              initializing
 */
bool init_database( media_interface_t *mi );

/**
 * Accesses the next/previous/random song at a specified level.
 * 
 * @param current_song The song which we want to use as our reference point.
 *        If *current_song points to NULL, the first song will be found.
 * @param operation What song should be retrieved from the database
 * @param level The level we want to get the next song.
 * 
 * @note: next_song( NULL, DT_PREVIOUS, DL_SONG ) will result in --
 *  root->Artist[0]->Album[0]->Song <--- Song returned
 *                           ->Song
 *                           ->Song
 *                           ->Song
 *                 ->Album[1]->Song
 *                           
 * @note: next_song( *ptr, DT_PREVIOUS, DL_ALBUM ) will result in --
 *  root->Artist[0]->Album[0]->Song <--- Song passed in
 *                           ->Song
 *                 ->Album[1]->Song
 *                           ->Song
 *                 ->Album[2]->Song <--- Song returned
 *                           ->Song
 * 
 * @note  If the song passed in is the 2nd song in an album, the operation
 *        is DT_NEXT, and the level is DL_ALBUM, the new song will be the
 *        first song in the next album.
 *        
 * root->Artist[0]->Album[0]->Song
 *                          ->Song <--- Song passed in
 *                          ->Song
 *                ->Album[1]->Song <--- Song returned
 *                          ->Song
 *                                    
 * @return DS_SUCCESS when a new song is found and the current_song pointer
 *         is updated.  DS_END_OF_LIST will place the first song of the
 *         specified level in the current_song pointer.  DS_FAILURE otherwise
 */
db_status_t next_song( song_node_t ** current_song,
                       const db_traverse_t operation,
                       const db_level_t level );

/**
 * Using this function will keep a playlist queue such that
 * a DT_PREVIOUS will result in the previous song being played.
 * If no previous song is available, the @next_song rules will be
 * followed with a supplied DT_RANDOM.
 *
 * @param operation if not DT_PREVIOUS, a DT_RANDOM will be used
 *        for the @next_song call
 *
 * @note Songs collected via next_song interface will not be placed
 *       into the play queue
 */
db_status_t queued_next_song( song_node_t ** current_song,
                             const db_traverse_t operation,
                             const db_level_t level );

typedef db_status_t (*next_song_fct)( song_node_t **, const db_traverse_t, const db_level_t );

/**
 * Cleans up all the Group/Artist/Album/Song nodes.
 */
void database_purge( void );

/**
 * The structure of the pools are:
 * root -> Artist[0-N] -> Albums[0-N] -> Song[1]
 * 
 * root->Artist[0]->Album[0]->Song
 *                          ->Song
 *                          ->Song
 *                ->Album[1]->Song
 *                          ->Song
 *     ->Artist[1]->Album[0]->Song
 *                          ->Song
 *     ->Artist[0]->Album[0]->Song
 *     .
 *     .
 *     .
 *     ->Artist[0]->Album[0]->Song
 * 
 * Searches all directories for supported file types which can be played
 * and places the metadata of the files into the database.
 * 
 * ** WARNING ** this call will take a long time.
 * 
 * @param RootDirectory the identifier which is the location of the root
 *        filesystem.  NULL terminated string.
 *        
 * @return true if the database was properly created and setup.  False
 *         otherwise.
 */
bool populate_database( const char * RootDirectory );

#endif /* __DATABASE_H__ */
