/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdbool.h>
#include <stdint.h>
#include <media-interface/media-interface.h>
#include <stdlib.h>
#include <binary-tree-avl/binary-tree-avl.h>

#include "database.h"
#include "internal_database.h"
#include "indexer.h"
#include "add_song.h"
#include "database_print.h"
#include "file_os_wrapper.h"
#include "mi_interface.h"
#include "generic.h"
#include "file_helper.h"

#define DEBUG_DUMP_LIST 1
#define PRINT_DB_INDEX_TIME 0

#if ( 0 != PRINT_DB_INDEX_TIME )
typedef uint32_t portTickType;
extern portTickType xTaskGetTickCount(void);
#endif

static bool __put_songs_into_root( const char * RootDirectory );


/**
 * The structure of the pools are:
 * Root[1] -> Artist[0-N] -> Albums[0-N] -> Song[1]
 * 
 * Root->Artist[0]->Album[0]->Song
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
bool populate_database( const char * RootDirectory )
{
    bt_node_t * root;
    
    database_purge();
    
    if( NULL != RootDirectory ) {
#if ( 0 != PRINT_DB_INDEX_TIME )
        portTickType begin = xTaskGetTickCount();
        portTickType end;
#endif
        root = get_new_generic_node(GNT_ROOT, "root");
        if( NULL != root ) {
            rdn.root = (generic_node_t *)(root->data);
            if( __put_songs_into_root(RootDirectory) ) {
                index_root(&(rdn.root->node));
#if (0 != DEBUG_DUMP_LIST)
                database_print();
#endif
#if ( 0 != PRINT_DB_INDEX_TIME )
                end = xTaskGetTickCount();
                printf("DB Time took: %u\n", end - begin);
#endif
                return true;
            }
        }
    }
    database_purge();
    return false;
}

static bool __put_songs_into_root( const char * RootDirectory )
{
    char full_path[MAX_SHORT_FILENAME_PATH_W_NULL];
    size_t full_path_size;
    long last_dir_next_read_loc[260];
    int16_t last_dir_index = -1;
    
    /* Song structures */
    media_metadata_t metadata;
    media_play_fn_t play_fn;

    if( NULL == RootDirectory ) {
        return false;
    }
    
    strcpy( full_path, RootDirectory );
    full_path_size = strlen(full_path);
    if( FRV_RETURN_GOOD != open_directory( full_path ) ) {
        return false;
    }
    while( 1 ) {
        file_info_t file_info;
        /* Open the next element in this directory */
        file_return_value rv = get_next_element_in_directory( &file_info );
        if( FRV_END_OF_ENTRIES == rv ) {
            /* We don't have any more files in this directory.
             */
            if( -1 == last_dir_index ) {
                /* This is the base directory which was passed in.  We
                 * don't want to search folders below this.
                 */
                return true;
            }
            /* Go up a directory and continue searching for files
             */
            /* Open the parent directory */
            if(    ( false == remove_last_dir_name( full_path, &full_path_size ) )
                || ( FRV_RETURN_GOOD != open_directory( full_path ) )
                || ( FRV_RETURN_GOOD !=
                        seek_to_index_in_directory(last_dir_next_read_loc[last_dir_index--]) ) )
            {
                return false;
            }
        } else if( FRV_RETURN_GOOD == rv ) {
            append_to_path(full_path, &full_path_size, file_info.short_filename );
            if( true == file_info.is_dir ) {
                /* Open the found directory for searching.
                 * But first mark where we are in this directory
                 */
                if(    ( FRV_RETURN_GOOD !=
                             get_index_in_directory(&last_dir_next_read_loc[++last_dir_index]) )
                    || ( FRV_RETURN_GOOD != open_directory( full_path ) ) )
                {
                    return false;
                }

            } else { /* This is a file */
                if( MI_RETURN_OK ==
                        mi_get_information(full_path, &metadata, &play_fn) ) {
                    if( 0 < metadata.disc_number ) {
                        metadata.track_number += 1000 * (metadata.disc_number - 1);
                    }
                    add_song_to_root( rdn.root, &metadata,
                            play_fn, full_path );
                }
                if( false == remove_last_dir_name( full_path, &full_path_size ) ) {
                    return false;
                }
            }
        } else {
            /* We ran into some sort of error, bail. */
            return false;
        }
    }
}
