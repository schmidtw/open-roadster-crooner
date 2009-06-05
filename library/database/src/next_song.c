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

/* See database.h for information */
db_status_t next_song( song_node_t ** current_song,
                       const db_traverse_t operation,
                       const db_level_t level )
{
    group_node_t *group;
    artist_node_t *artist;
    album_node_t  *album;
    db_status_t rv = DS_FAILURE;
    
    if(    ( false == rdn.initialized )
        || ( NULL == current_song ) )
    {
        return DS_FAILURE;
    }
    
    if( NULL == *current_song ) {
        if(    ( NULL == rdn.groups.head )
            || ( NULL == rdn.groups.tail ) )
        {
            return DS_FAILURE;
        }
        switch( operation ) {
            case DT_NEXT:
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
                return DS_SUCCESS;
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
            case DT_RANDOM:
                break;
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
            /* DT_RANDOM */
            switch( level ) {
                case DL_GROUP:
                case DL_ARTIST:
                case DL_ALBUM:
                default: /* DL_SONG */
                    ;
            }
    }
    
    
    return rv;
}
