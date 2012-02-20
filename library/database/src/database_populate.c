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

#define DEBUG_DUMP_LIST 0

bool iterate_to_dir_entry( const char * dir_name );
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
        root = get_new_generic_node(GNT_ROOT, "root");
        if( NULL != root ) {
            rdn.root = (generic_node_t *)(root->data);
            if( __put_songs_into_root(RootDirectory) ) {
                index_root(&(rdn.root->node));
#if (0 != DEBUG_DUMP_LIST)
                database_print();
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
    char last_dir[MAX_SHORT_FILENAME_PATH_W_NULL];
    char full_path[MAX_SHORT_FILENAME_PATH_W_NULL];
    
    if( NULL == RootDirectory ) {
        return false;
    }
    
    strcpy( full_path, RootDirectory );
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
            if( 0 == strcmp( full_path, RootDirectory ) ) {
                /* This is the base directory which was passed in.  We
                 * don't want to search folders below this.
                 */
                return true;
            }
            if( false == get_last_dir_name( last_dir, full_path ) ) {
                return false;
            }
            /* If we have a last_dir, then we should
             * go up a directory and continue searching for files
             * after this directory name
             */
            /* Open the parent directory */
            if(    ( FRV_RETURN_GOOD != open_directory( full_path ) )
                || ( false == iterate_to_dir_entry( last_dir ) ) ) {
                return false;
            }
        } else if( FRV_RETURN_GOOD == rv ) {
            append_to_path(full_path, file_info.short_filename );
            if( true == file_info.is_dir ) {
                /* this is a directory, search for the group.  Once we have
                 * the group, call a subroutine which will place all files
                 * in this directory into the correct group.
                 */

                /* open the found directory for searching */
                if( FRV_RETURN_GOOD != open_directory( full_path ) ) {
                    return false;
                }
            } else { /* This is a file */
                media_status_t rv;
                media_metadata_t metadata;
                media_play_fn_t play_fn;
                char junk_filename[MAX_SHORT_FILENAME_W_NULL];
                /* Place this file into the miscellaneous group */
                rv = mi_get_information( full_path, &metadata, &play_fn );
                if( MI_RETURN_OK == rv ) {
                    if( 0 < metadata.disc_number ) {
                        metadata.track_number += 1000 * (metadata.disc_number - 1);
                    }
                    add_song_to_root( rdn.root, &metadata,
                            play_fn, full_path );
                }
                if( false == get_last_dir_name( junk_filename, full_path ) ) {
                    return false;
                }
            }
        } else {
            /* We ran into some sort of error, bail. */
            return false;
        }
    }
}

bool iterate_to_dir_entry( const char * dir_name )
{
    file_info_t file_info;
    /* Open directory elements until we get to the element which
     * we are trying to find
     */
    while( FRV_RETURN_GOOD == get_next_element_in_directory( &file_info ) ) {
        if( true == file_info.is_dir ) {
            if( 0 == strcmp(file_info.short_filename, dir_name) ) {
                return true;
            }
        }
    }
    return false;
}

