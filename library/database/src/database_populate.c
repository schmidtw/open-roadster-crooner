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
#include "database.h"
#include "internal_database.h"
#include "group.h"
#include "add_song.h"
#include "database_print.h"
#include "file_os_wrapper.h"
#include "mi_interface.h"

#include <media-interface/media-interface.h>

bool setup_group_name( const char * name );
group_node_t * find_group_node( const char * dir_name );
void place_songs_into_group( group_node_t * gn, char * dir_name );
bool get_last_dir_name( char * dest, char * src );
void append_to_path( char * dest, const char * src );
bool iterate_to_dir_entry( const char * dir_name );

#define FILENAME_LENGTH 11
#define FILENAME_LENGTH_WITH_NULL (FILENAME_LENGTH + 1)
#define MAX_ABS_FILE_NAME_W_NULL 257

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
 *        
 * @return true if the database was properly created and setup.  False
 *         otherwise.
 */
bool populate_database( const char ** directory,
                        const uint8_t num_directories,
                        const char * RootDirectory )
{
    uint8_t ii;
    
    database_purge();
    
    ll_init_list( &(rdn.groups) );
    for(ii=0;ii<num_directories;ii++) {
        if( NULL != directory[ii] ) {
            if( false == setup_group_name(directory[ii]) ) {
                goto failure;
            }
            rdn.size_list++;
        }
    }
    /* Create an extra group which will be the tail of the list
     * and will receive all of the unsorted media files
     */
    if( false == setup_group_name("") ) {
        goto failure;
    }
    rdn.size_list++;
    {
        char base_dir[MAX_ABS_FILE_NAME_W_NULL];
        file_info_t file_info;
        size_t num_chars_base_dir;
        
        strcpy( base_dir, RootDirectory );
        num_chars_base_dir = strlen( base_dir );
        if( FRV_RETURN_GOOD != open_directory( base_dir ) ) {
            goto failure;
        }
        
        while( FRV_RETURN_GOOD == get_next_element_in_directory( &file_info ) ) {
            /* append the file/dir name to the base dir.  This absolute path
             * is critical in opening this file/dir for metadata or searching
             * for more files.
             */
            append_to_path( base_dir, file_info.short_filename );
            if( true == file_info.is_dir ) {
                /* this is a directory, search for the group.  Once we have
                 * the group, call a subroutine which will place all files
                 * in this directory into the correct group.
                 */
                place_songs_into_group( find_group_node(file_info.short_filename),
                                        base_dir );
                /* Reopen the root directory */
                base_dir[num_chars_base_dir] = '\0';
                if( FRV_RETURN_GOOD != open_directory( base_dir ) ) {
                    goto failure;
                }
                iterate_to_dir_entry( file_info.short_filename );
            } else { /* This is a file */
                media_status_t rv;
                media_metadata_t *metadata;
                media_command_fn_t *command_fn;
                media_play_fn_t *play_fn;
                /* Place this file into the miscellaneous group */
                rv = mi_get_information( base_dir, metadata,
                        command_fn, play_fn );
                if( MI_RETURN_OK == rv ) {
                    add_song_to_group( (group_node_t *)rdn.groups.tail->data,
                            metadata->artist, metadata->album,
                            metadata->title, metadata->track_number,
                            command_fn, play_fn, base_dir );
                }
            }
        }
        goto failure;
    }
    return true;
failure:
    database_print();
    database_purge();
    return false;
}

bool setup_group_name( const char * name )
{
    ll_node_t *n;
    n = get_new_group_and_node( name );
    if( NULL == n ) {
        return false;
    }
    ll_append(&rdn.groups, n);
    return true;
}

group_node_t * find_group_node( const char * dir_name )
{
    group_node_t * gn;
    if( NULL == dir_name ) {
        return NULL;
    }
    if( NULL != rdn.groups.head ) {
        gn = (group_node_t *)rdn.groups.head->data;
    }
    while( NULL == gn ) {
        if( 0 == strcmp( dir_name, gn->name ) ) {
            break;
        }
        if( NULL == gn->node.next ) {
            gn = NULL;
        } else {
            gn = (group_node_t *)gn->node.next->data;
        }
    }
    
    if( NULL == gn ) {
        if( NULL != rdn.groups.tail ) {
            return ((group_node_t *)rdn.groups.tail->data);
        }
    }
    return gn;
}

void place_songs_into_group( group_node_t * gn, char * dir_name )
{
    char last_dir[FILENAME_LENGTH_WITH_NULL];
    char full_path[MAX_ABS_FILE_NAME_W_NULL];
    
    if( NULL == gn ) {
        return;
    }
//    snprintf(dir_name, "/%s/", dlist->currentEntry.FileName);
    full_path[0] = '\0';
    strcpy( full_path, dir_name );
    /* Open the full_path directory for populating */
    if( FRV_RETURN_GOOD != open_directory( full_path ) ) {
        return;
    }
    while( 1 ) {
        file_info_t file_info;
        /* Open the next element in this directory */
        file_return_value rv = get_next_element_in_directory( &file_info ); 
        
        if( FRV_END_OF_ENTRIES == rv ) {
            /* We don't have any more files in this directory.
             */
            if( 0 == strcmp( full_path, dir_name ) ) {
                /* This is the base directory which was passed in.  We
                 * don't want to search folders below this.
                 */
                return;
            }
            if( false == get_last_dir_name( last_dir, full_path ) ) {
                return;
            }
            /* If we have a last_dir, then we should
             * go up a directory and continue searching for files
             * after this directory name
             */
            /* Open the parent directory */
            if( FRV_RETURN_GOOD != open_directory( full_path ) ) {
                return;
            }
            if( false == iterate_to_dir_entry( last_dir ) ) {
                return;
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
                    return;
                }
            } else { /* This is a file */
                media_status_t rv;
                media_metadata_t *metadata;
                media_command_fn_t *command_fn;
                media_play_fn_t *play_fn;
                char junk_filename[MAX_SHORT_FILE_NAME_W_NULL];
                /* Place this file into the miscellaneous group */
                rv = mi_get_information( full_path, metadata,
                        command_fn, play_fn );
                if( MI_RETURN_OK == rv ) {
                    add_song_to_group( gn, metadata->artist, metadata->album,
                            metadata->title, metadata->track_number,
                            command_fn, play_fn, full_path );
                }
                get_last_dir_name( junk_filename, full_path );
            }
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

/**
 * Will remove the last directory from the src and place the
 * string onto the dest ptr.  The dest buff must be able to
 * support short name filesize of 8 characters plus the
 * terminating character.
 * 
 * ex: src * = /blue/black/green
 * result:
 *    dest * = green
 *     src * = /blue/black
 */
bool get_last_dir_name( char * dest, char * src )
{
    char *i_src;
    char *i_dst;
    char *last_slash;
    
    if(    (NULL == dest)
        || (NULL == src) ) {
        return false;
    }
    
    last_slash = NULL;
    i_src = src;
    i_dst = dest;
    while( '\0' != *i_src ) {
        if( '/' == *i_src ) {
            last_slash = i_src;
        }
        i_src++;
    }
    i_src = last_slash;
    *last_slash = '\0';
    i_src++;
    while( '\0' != *i_src ) {
        *i_dst++ = *i_src++;
    }
    *i_dst = '\0';
    if( last_slash+1 == i_src ) {
        return false;
    }
    return true;
}

/**
 * Will place the src string plus a '/' onto
 * the end of the dest string.
 * 
 * ex: dest * = /blue/black
 *      src * = green
 * result:
 *     dest * = /blue/black/green
 *     
 * @param dest a buffer large enough to support max_characters+1 more
 *             characters
 * @param src buffer which contains the new string
 */
void append_to_path( char * dest, const char * src )
{
    char *i_dst;
    bool add_trailing_slash = true;
    if(    (NULL == dest)
        || (NULL == src) ) {
        return;
    }
    
    i_dst = dest;
    while( '\0' != *i_dst ) {
        add_trailing_slash = ( '/' != *i_dst );
        i_dst++;
    }
    if( true == add_trailing_slash ) {
        *i_dst++ = '/';
    }
    strcpy( i_dst, src );
}
