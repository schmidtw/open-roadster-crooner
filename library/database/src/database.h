#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include <linked-list/linked-list.h>
#include <media-interface/media-interface.h>

#define MAX_GROUP_NAME           24
#define MAX_GROUP_NAME_W_NULL    (MAX_GROUP_NAME + 1)
#define MAX_ARTIST_NAME          127
#define MAX_ARTIST_NAME_W_NULL   (MAX_ALBUM_TITLE + 1)
#define MAX_ALBUM_TITLE          127
#define MAX_ALBUM_TITLE_W_NULL   (MAX_ALBUM_TITLE + 1)
#define MAX_SONG_TITLE           127
#define MAX_SONG_TITLE_W_NULL    (MAX_SONG_TITLE + 1)

#define MAX_SHORT_FILENAME                 12
#define MAX_SHORT_FILENAME_W_NULL          (MAX_SHORT_FILENAME + 1)
#define MAX_SHORT_FILENAME_PATH            260
#define MAX_SHORT_FILENAME_PATH_W_NULL     (MAX_SHORT_FILENAME_PATH + 1)

typedef struct {
    char name[MAX_GROUP_NAME_W_NULL];
    ll_node_t node;
    ll_list_t artists;
    uint32_t size_list;
} group_node_t;

typedef struct {
    char name[MAX_ARTIST_NAME_W_NULL];
    ll_node_t node;
    ll_list_t albums;
    uint32_t size_list;
    group_node_t * group;
} artist_node_t;

typedef struct {
    char name[MAX_ALBUM_TITLE_W_NULL];
    ll_node_t node;
    ll_list_t songs;
    uint32_t size_list;
    artist_node_t * artist;
} album_node_t;

typedef struct {
    char title[MAX_SONG_TITLE_W_NULL];
    ll_node_t node;
    char file_location[MAX_SHORT_FILENAME_PATH_W_NULL];
    album_node_t * album;
    uint16_t track_number;
    double album_gain;
    double album_peak;
    double track_gain;
    double track_peak;
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
    DL_ARTIST,
    DL_GROUP
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
 *        Group[0]->Artist[0]->Album[0]->Song <--- Song returned
 *                                     ->Song
 *                                     ->Song
 *                                     ->Song
 *                           ->Album[1]->Song
 *                           
 * @note: next_song( *ptr, DT_PREVIOUS, DL_ALBUM ) will result in --
 *        Group[0]->Artist[0]->Album[0]->Song <--- Song passed in
 *                                     ->Song
 *                           ->Album[1]->Song
 *                                     ->Song
 *                           ->Album[2]->Song <--- Song returned
 *                                     ->Song
 * 
 * @note  If the song passed in is the 2nd song in an album, the operation
 *        is DT_NEXT, and the level is DL_ALBUM, the new song will be the
 *        first song in the next album.
 *        
 * Root->Group[0]->Artist[0]->Album[0]->Song
 *                                    ->Song <--- Song passed in
 *                                    ->Song
 *                          ->Album[1]->Song <--- Song returned
 *                                    ->Song
 *                                    
 * @return DS_SUCCESS when a new song is found and the current_song pointer
 *         is updated.  DS_END_OF_LIST will place the first song of the
 *         specified level in the current_song pointer.  DS_FAILURE otherwise
 */
db_status_t next_song( song_node_t ** current_song,
                       const db_traverse_t operation,
                       const db_level_t level );

/**
 * Cleans up all the Group/Artist/Album/Song nodes.
 */
void database_purge( void );

/**
 * The structure of the pools are:
 * Root[1] -> Groups[6] -> Artist[0-N] -> Albums[0-N] -> Song[1]
 * 
 * Root->Group[0]->Artist[0]->Album[0]->Song
 *                                    ->Song
 *                                    ->Song
 *                          ->Album[1]->Song
 *                                    ->Song
 *               ->Artist[1]->Album[0]->Song
 *                                    ->Song
 *     ->Group[1]->Artist[0]->Album[0]->Song
 *     .
 *     .
 *     .
 *     ->Group[5]->Artist[0]->Album[0]->Song
 * 
 * Searches all directories for supported file types which can be played
 * and places the metadata of the files into the database.
 * 
 * ** WARNING ** this call will take a long time.
 * 
 * @param directory NULL terminated string which is the name of a root folder.
 *              If a root directory matches the string, then all music files
 *              in that folder and sub-folders will be placed in the
 *              corresponding group.  A NULL directory will allow that group
 *              to be assigned UNKOWN root folders.
 * @param num_directories The number of char * pointers in the directory
 *              parameter.
 * @param RootDirectory the identifier which is the location of the root
 *        filesystem.  NULL terminated string.
 * @param fs pointer to the Filesystem which has been opened.
 *        
 * @return true if the database was properly created and setup.  False
 *         otherwise.
 */
bool populate_database( const char ** directory,
                        const uint8_t num_directories,
                        const char * RootDirectory );

#endif /* __DATABASE_H__ */
