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
#include <stdlib.h>
#include "database.h"
#include "internal_database.h"

/* See database.h for information */
db_status_t next_song( volatile song_node_t ** current_song,
                       const db_traverse_t operation,
                       const db_level_t level )
{
    group_node_t *group;
    artist_node_t *artist;
    album_node_t  *album;
    db_status_t rv = DS_FAILURE;
    
    if(    ( false == rdn.initialized )
        || ( NULL == current_song )
        || ( NULL == rdn.groups.head )
        || ( NULL == rdn.groups.tail ) )
    {
        return DS_FAILURE;
    }
    
    if( NULL == *current_song ) {
        switch( operation ) {
            case DT_NEXT:
            case DT_RANDOM:
                group = (group_node_t *)rdn.groups.head->data;
                if( NULL == group->artists.head ) {
                    return DS_FAILURE;
                }
                artist = (artist_node_t *)group->artists.head->data;
                if( NULL == artist->albums.head ) {
                    return DS_FAILURE;
                }
                album = (album_node_t *)artist->albums.head->data;
                if( NULL == album->songs.head ) {
                    return DS_FAILURE;
                }
                *current_song = (song_node_t *)album->songs.head->data;
                if( DT_NEXT == operation ) {
                    return DS_SUCCESS;
                }
                /* else DT_RANDOM */
                break;
            case DT_PREVIOUS:
                group = (group_node_t *)rdn.groups.tail->data;
                if( NULL == group->artists.tail ) {
                    return DS_FAILURE;
                }
                artist = (artist_node_t *)group->artists.tail->data;
                if( NULL == artist->albums.tail ) {
                    return DS_FAILURE;
                }
                album = (album_node_t *)artist->albums.tail->data;
                if( NULL == album->songs.tail ) {
                    return DS_FAILURE;
                }
                *current_song = (song_node_t *)album->songs.tail->data;
                return DS_SUCCESS;
        }
    }
    
    album = (*current_song)->album;
    artist = album->artist;
    group = artist->group;
    
    switch( operation ) {
        case DT_NEXT:
            switch( level ) {
                case DL_SONG:
                    if( NULL == (*current_song)->node.next ) {
                        *current_song = (song_node_t *)album->songs.head->data;
                        return DS_END_OF_LIST;
                    }
                    *current_song = (song_node_t *)(*current_song)->node.next->data;
                    return DS_SUCCESS;
                case DL_ALBUM:
                    if( NULL == album->node.next ) {
                        album = (album_node_t *)artist->albums.head->data;
                        rv = DS_END_OF_LIST;
                    } else {
                        album = (album_node_t *)album->node.next->data;
                        rv = DS_SUCCESS;
                    }
                    *current_song = (song_node_t *)album->songs.head->data;
                    break;
                case DL_ARTIST:
                    if( NULL == artist->node.next ) {
                        artist = (artist_node_t *)group->artists.head->data;
                        rv = DS_END_OF_LIST;
                    } else {
                        artist = (artist_node_t *)artist->node.next->data;
                        rv = DS_SUCCESS;
                    }
                    album = (album_node_t *)artist->albums.head->data;
                    *current_song = (song_node_t *)album->songs.head->data;
                    break;
                default: /* DL_GROUP */
                    if( NULL == group->node.next ) {
                        group = (group_node_t *)rdn.groups.head->data;
                        rv = DS_END_OF_LIST;
                    } else {
                        group = (group_node_t *)group->node.next->data;
                        rv = DS_SUCCESS;
                    }
                    artist = (artist_node_t *)group->artists.head->data;
                    album = (album_node_t *)artist->albums.head->data;
                    *current_song = (song_node_t *)album->songs.head->data;
                    break;
            }
            break;
        case DT_PREVIOUS:
            switch( level ) {
                case DL_SONG:
                    if( NULL == (*current_song)->node.prev ) {
                        *current_song = (song_node_t *)album->songs.tail->data;
                        return DS_END_OF_LIST;
                    }
                    *current_song = (song_node_t *)(*current_song)->node.prev->data;
                    return DS_SUCCESS;
                case DL_ALBUM:
                    if( NULL == album->node.prev ) {
                        album = (album_node_t *)artist->albums.tail->data;
                        rv = DS_END_OF_LIST;
                    } else {
                        album = (album_node_t *)album->node.prev->data;
                        rv = DS_SUCCESS;
                    }
                    *current_song = (song_node_t *)album->songs.tail->data;
                    break;
                case DL_ARTIST:
                    if( NULL == artist->node.prev ) {
                        artist = (artist_node_t *)group->artists.tail->data;
                        rv = DS_END_OF_LIST;
                    } else {
                        artist = (artist_node_t *)artist->node.prev->data;
                        rv = DS_SUCCESS;
                    }
                    album = (album_node_t *)artist->albums.tail->data;
                    *current_song = (song_node_t *)album->songs.tail->data;
                    break;
                default: /* DL_GROUP */
                    if( NULL == group->node.next ) {
                        group = (group_node_t *)rdn.groups.tail->data;
                        rv = DS_END_OF_LIST;
                    } else {
                        group = (group_node_t *)group->node.prev->data;
                        rv = DS_SUCCESS;
                    }
                    artist = (artist_node_t *)group->artists.tail->data;
                    album = (album_node_t *)artist->albums.tail->data;
                    *current_song = (song_node_t *)album->songs.tail->data;
                    break;
            }
            break;
        default:
            rv = DS_SUCCESS;
            /* DT_RANDOM */
            switch( level ) {
                int random_number;
                int ii;
                case DL_GROUP:
                    random_number = rand() % rdn.size_list;
                    group = (group_node_t *) rdn.groups.head->data;
                    for(ii = 0; ii < random_number; ii++) {
                        if( NULL == group->node.next ) {
                            return DS_FAILURE;
                        }
                        group = (group_node_t *)group->node.next->data;
                    }
                    /* Break left out on purpose */
                case DL_ARTIST:
                    random_number = rand() % group->size_list;
                    if( NULL == group->artists.head ) {
                        return DS_FAILURE;
                    }
                    artist = (artist_node_t *) group->artists.head->data;
                    for(ii = 0; ii < random_number; ii++) {
                        if( NULL == artist->node.next ) {
                            return DS_FAILURE;
                        }
                        artist = (artist_node_t *) artist->node.next->data;
                    }
                    /* Break left out on purpose */
                case DL_ALBUM:
                    random_number = rand() % artist->size_list;
                    if( NULL == artist->albums.head ) {
                        return DS_FAILURE;
                    }
                    album = (album_node_t *)artist->albums.head->data;
                    for(ii = 0; ii < random_number; ii++) {
                        if( NULL == album->node.next ) {
                            return DS_FAILURE;
                        }
                        album = (album_node_t *)album->node.next->data;
                    }
                    /* Break left out on purpose */
                default: /* DL_SONG */
                    random_number = rand() % album->size_list;
                    if( NULL == album->songs.head ) {
                        return DS_FAILURE;
                    }
                    *current_song = (song_node_t *)album->songs.head->data;
                    for(ii = 0; ii < random_number; ii++) {
                        if( NULL == (*current_song)->node.next ) {
                            return DS_FAILURE;
                        }
                        *current_song = (song_node_t *)(*current_song)->node.next->data;
                    }
            }
    }
    
    
    return rv;
}
