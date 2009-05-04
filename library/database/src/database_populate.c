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

bool setup_group_name( const char * name );
group_node_t * find_group_node( const char * dir_name );
void place_songs_into_group( group_node_t * gn, char * dir_name, DirList * dlist, FileSystem *fs );
bool get_last_dir_name( char * dest, char * src );
void append_dir_name( char * dest, const char * src, size_t max_characters );
void local_memcpy( char * dest, const char * src, size_t max_characters );
bool iterate_to_dir_entry( const char * dir_name, DirList * list );

#define FILENAME_LENGTH 11
#define FILENAME_LENGTH_WITH_NULL (FILENAME_LENGTH + 1)

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
                        const char * RootDirectory,
                        FileSystem *fs )
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
        DirList dir_list;
        if( 0 != rdn.openDir(&dir_list, fs, "/") ) {
            goto failure;
        }
        while( 0 == rdn.openNext(&dir_list) ) {
            char filename[FILENAME_LENGTH_WITH_NULL];
            char dir_name[FILENAME_LENGTH_WITH_NULL];
            char extension[4];
            char full_filename[FILENAME_LENGTH_WITH_NULL+2];
            dir_name[0] = '\0';
            snprintf(filename, 9, "%s", dir_list.currentEntry.FileName);
            filename[9] = '\0';
            snprintf(extension, 4, "%s", &(dir_list.currentEntry.FileName[8]));
            extension[3] = '\0';
            
            if( ATTR_DIRECTORY & dir_list.currentEntry.Attribute ) {
                /* this is a directory, search for the group.  Once we have
                 * the group, call a subroutine which will place all files
                 * in this directory into the correct group.
                 */
                local_memcpy(dir_name, filename, FILENAME_LENGTH);
                place_songs_into_group( find_group_node(filename),
                                        filename,
                                        &dir_list,
                                        fs);
                if( 0 != rdn.openDir(&dir_list, fs, "/") ) {
                    goto failure;
                }
                iterate_to_dir_entry( dir_name, &dir_list );
            } else { /* This is a file */
                if( (ATTR_VOLUME_ID | ATTR_SYSTEM ) & dir_list.currentEntry.Attribute ) {
                    continue;
                }
                /* Place this file into the miscellaneous group */
                sprintf(full_filename, "%s.%s", filename, extension);
                add_song_to_group( (group_node_t *)rdn.groups.tail->data,
                                   "Fake Artist\0",
                                   "Root Directory\0",
                                   full_filename,
                                   2 );
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

void place_songs_into_group( group_node_t * gn, char * dir_name, DirList * list, FileSystem *fs )
{
    char last_dir[FILENAME_LENGTH_WITH_NULL];
    char full_path[257];
    
    if(    ( NULL == gn )
        || ( NULL == list ) ) {
        return;
    }
//    snprintf(dir_name, "/%s/", dlist->currentEntry.FileName);
    full_path[0] = '\0';
    
    append_dir_name( full_path, dir_name, FILENAME_LENGTH );
    if( 0 != rdn.openDir(list, fs, full_path) ) {
        return;
    }
    while( 1 ) {
        if( 0 != rdn.openNext(list) ) {
            /* We don't have any more files in this directory.
             */
            if( false == get_last_dir_name(last_dir, full_path) ) {
                return;
            }
            /* If we have a last_dir, then we should
             * go up a directory and continue searching for files
             * after this directory name
             */
            if( 0 != rdn.openDir(list, fs, full_path) ) {
                return;
            }
            if( false == iterate_to_dir_entry(last_dir, list) ) {
                return;
            }
        } else {
            char filename[FILENAME_LENGTH_WITH_NULL];
            memcpy( filename, list->currentEntry.FileName, FILENAME_LENGTH );
            filename[FILENAME_LENGTH] = '\0';
            if( ATTR_DIRECTORY & list->currentEntry.Attribute ) {
                /* this is a directory, search for the group.  Once we have
                 * the group, call a subroutine which will place all files
                 * in this directory into the correct group.
                 */
                append_dir_name(full_path, list->currentEntry.FileName, FILENAME_LENGTH);
                fflush(stdout);
                if( 0 != rdn.openDir(list, fs, full_path) ) {
                    return;
                }
            } else { /* This is a file */
                /* Place this file into the miscellaneous group */
                add_song_to_group( (group_node_t *)rdn.groups.tail->data,
                                   "Fake Artist\0",
                                   dir_name,
                                   filename,
                                   2 );
            }
        }
    }
}

bool iterate_to_dir_entry( const char * dir_name, DirList * list )
{
    char current_dir[FILENAME_LENGTH_WITH_NULL];
    
    while( 0 == rdn.openNext(list) ) {
        if( ATTR_DIRECTORY & list->currentEntry.Attribute ) {
            local_memcpy( current_dir, list->currentEntry.FileName, FILENAME_LENGTH);
            if( 0 == strcmp(current_dir, dir_name) ) {
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
 * Will place the src string (up to max_characters) plus a '/' onto
 * the end of the dest string.
 * 
 * ex: dest * = /blue/black
 *      src * = green
 * result:
 *     dest * = /blue/black/green 
 * 
 * @param max_characters number of characters to be read from the src
 * @param dest a buffer large enough to support max_characters+1 more
 *             characters
 * @param src buffer which contains the new string
 */
void append_dir_name( char * dest, const char * src, size_t max_characters )
{
    size_t ii;
    char *i_dst;
    if(    (NULL == dest)
        || (NULL == src)
        || (0 == max_characters) ) {
        return;
    }
    
    i_dst = dest;
    while( '\0' != *i_dst ) {
        i_dst++;
    }
    *i_dst++ = '/';
    local_memcpy( i_dst, src, max_characters );
}

void local_memcpy( char * dest, const char * src, size_t max_characters )
{
    size_t ii;
    char *i_src;
    char *i_dst;
    if(    (NULL == dest)
        || (NULL == src)
        || (0 == max_characters) ) {
        return;
    }
    i_src = src;
    i_dst = dest;
    for( ii = 0; ii < max_characters; ii++ ) {
        if(    ('\0' == *i_src)
            || (' ' == *i_src) ) {
            *i_dst = '\0';
            return;
        }
       *i_dst++ = *i_src++;
    }
    /* At this point we know that the last character was not a terminating
     * character.  So we must decrement i_dst because it points one position
     * past our limit, and then terminate this string.
     */
    i_dst--;
    *i_dst = '\0';
}
