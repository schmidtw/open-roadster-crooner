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

#define NEXT_SONG_DEBUG 0

#define _D1(...)

#if (NEXT_SONG_DEBUG > 0)
#include <stdio.h>
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif

song_node_t * find_random_song_from_artist( generic_node_t * an, uint32_t first_song_index, uint32_t last_song_index );
song_node_t * find_random_song_from_album( generic_node_t * an, uint32_t random_song_index );
song_node_t * find_random_song_from_songs( song_node_t * sn, uint32_t random_song_index );
uint32_t random_number_in_range( uint32_t start, uint32_t stop );

/* See database.h for information */
db_status_t next_song( song_node_t ** current_song,
                       const db_traverse_t operation,
                       const db_level_t level )
{
    generic_node_t *group;
    generic_node_t *artist;
    generic_node_t  *album;
    db_status_t rv = DS_FAILURE;
    
    if(    ( false == rdn.initialized )
        || ( NULL == current_song )
        || ( NULL == rdn.groups.head )
        || ( NULL == rdn.groups.tail ) )
    {
        return DS_FAILURE;
    }
    
    if( NULL == *current_song ) {
        group = (generic_node_t *)rdn.groups.head->data;
        if( NULL == group->children.head ) {
            return DS_FAILURE;
        }
        artist = (generic_node_t *)group->children.head->data;
        if( NULL == artist->children.head ) {
            return DS_FAILURE;
        }
        album = (generic_node_t *)artist->children.head->data;
        if( NULL == album->children.head ) {
            return DS_FAILURE;
        }
        *current_song = (song_node_t *)album->children.head->data;
        if( DT_NEXT == operation ) {
            return DS_SUCCESS;
        }
    }
    
    album = (*current_song)->d.parent;
    artist = album->parent;
    group = artist->parent;
    
    switch( operation ) {
        case DT_NEXT:
            switch( level ) {
                case DL_SONG:
                    if( NULL == (*current_song)->d.node.next ) {
                        *current_song = (song_node_t *)album->children.head->data;
                        return DS_END_OF_LIST;
                    }
                    *current_song = (song_node_t *)(*current_song)->d.node.next->data;
                    return DS_SUCCESS;
                case DL_ALBUM:
                    if( NULL == album->node.next ) {
                        album = (generic_node_t *)artist->children.head->data;
                        rv = DS_END_OF_LIST;
                    } else {
                        album = (generic_node_t *)album->node.next->data;
                        rv = DS_SUCCESS;
                    }
                    *current_song = (song_node_t *)album->children.head->data;
                    break;
                case DL_ARTIST:
                    if( NULL == artist->node.next ) {
                        artist = (generic_node_t *)group->children.head->data;
                        rv = DS_END_OF_LIST;
                    } else {
                        artist = (generic_node_t *)artist->node.next->data;
                        rv = DS_SUCCESS;
                    }
                    album = (generic_node_t *)artist->children.head->data;
                    *current_song = (song_node_t *)album->children.head->data;
                    break;
                default: /* DL_GROUP */
                    if( NULL == group->node.next ) {
                        group = (generic_node_t *)rdn.groups.head->data;
                        rv = DS_END_OF_LIST;
                    } else {
                        group = (generic_node_t *)group->node.next->data;
                        rv = DS_SUCCESS;
                    }
                    artist = (generic_node_t *)group->children.head->data;
                    album = (generic_node_t *)artist->children.head->data;
                    *current_song = (song_node_t *)album->children.head->data;
                    break;
            }
            break;
        case DT_PREVIOUS:
            switch( level ) {
                case DL_SONG:
                    if( NULL == (*current_song)->d.node.prev ) {
                        *current_song = (song_node_t *)album->children.tail->data;
                        return DS_END_OF_LIST;
                    }
                    *current_song = (song_node_t *)(*current_song)->d.node.prev->data;
                    return DS_SUCCESS;
                case DL_ALBUM:
                    if( NULL == album->node.prev ) {
                        album = (generic_node_t *)artist->children.tail->data;
                        rv = DS_END_OF_LIST;
                    } else {
                        album = (generic_node_t *)album->node.prev->data;
                        rv = DS_SUCCESS;
                    }
                    *current_song = (song_node_t *)album->children.head->data;
                    break;
                case DL_ARTIST:
                    if( NULL == artist->node.prev ) {
                        artist = (generic_node_t *)group->children.tail->data;
                        rv = DS_END_OF_LIST;
                    } else {
                        artist = (generic_node_t *)artist->node.prev->data;
                        rv = DS_SUCCESS;
                    }
                    album = (generic_node_t *)artist->children.tail->data;
                    *current_song = (song_node_t *)album->children.head->data;
                    break;
                default: /* DL_GROUP */
                    if( NULL == group->node.prev ) {
                        group = (generic_node_t *)rdn.groups.tail->data;
                        rv = DS_END_OF_LIST;
                    } else {
                        group = (generic_node_t *)group->node.prev->data;
                        rv = DS_SUCCESS;
                    }
                    artist = (generic_node_t *)group->children.tail->data;
                    album = (generic_node_t *)artist->children.tail->data;
                    *current_song = (song_node_t *)album->children.head->data;
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
                    /* The group list is 1 larger than the random range 
                     * function is expecting.
                     */
                    random_number = random_number_in_range(0, (rdn.size_list-1));
                    group = (generic_node_t *) rdn.groups.head->data;
                    
                    for(ii = 0; ii < random_number; ii++) {
                        if( NULL == group->node.next ) {
                            return DS_FAILURE;
                        }
                        group = (generic_node_t *)group->node.next->data;
                    }
                    /* Break left out on purpose */
                case DL_ARTIST:
                {
                    song_node_t * sn;
                    if( NULL == group->children.head ) {
                        return DS_FAILURE;
                    }
                    sn = find_random_song_from_artist((generic_node_t *)group->children.head->data,
                                                      group->d.list.index_songs_start,
                                                      group->d.list.index_songs_stop);
                    if( NULL == sn ) {
                        return DS_FAILURE;
                    }
                    *current_song = sn;
                    break;
                }
                case DL_ALBUM:
                {
                    song_node_t * sn;
                    sn = find_random_song_from_artist(artist, artist->d.list.index_songs_start, artist->d.list.index_songs_stop);
                    if( NULL == sn ) {
                        return DS_FAILURE;
                    }
                    *current_song = sn;
                    break;
                }
                default: /* DL_SONG */
                {
                    song_node_t * sn;
                    sn = find_random_song_from_artist(artist, album->d.list.index_songs_start, album->d.list.index_songs_stop);
                    if( NULL == sn ) {
                        return DS_FAILURE;
                    }
                    *current_song = sn;
                    break;
                }
            }
    }
    return rv;
}

song_node_t * find_random_song_from_artist( generic_node_t * an, uint32_t first_song_index, uint32_t last_song_index )
{
    generic_node_t * artist = an;
    uint32_t random_song_index;
    
    random_song_index = random_number_in_range(first_song_index, last_song_index);
    
    _D1( "Random Number = %d\n", random_song_index );
    while( 1 ) {
        if( NULL == artist ) {
            return NULL;
        }
        if(    ( random_song_index >= artist->d.list.index_songs_start )
            && ( random_song_index <= artist->d.list.index_songs_stop ) ) {
            /* We have found the artist of interest */
            return find_random_song_from_album((generic_node_t *)artist->children.head->data, random_song_index);
        }
        artist = (generic_node_t *) artist->node.next->data;
    }
}

song_node_t * find_random_song_from_album( generic_node_t * an, uint32_t random_song_index )
{
    generic_node_t * album = an;
    
    while( 1 ) {
        if( NULL == album ) {
            return NULL;
        }
        if(    ( random_song_index >= album->d.list.index_songs_start )
            && ( random_song_index <= album->d.list.index_songs_stop ) ) {
            /* We have found the album of interest */
            return find_random_song_from_songs((song_node_t *)album->children.head->data, random_song_index);
        }
        album = (generic_node_t *) album->node.next->data;
    }
}

song_node_t * find_random_song_from_songs( song_node_t * sn, uint32_t random_song_index )
{
    song_node_t * song = sn;
    
    while( 1 ) {
        if( NULL == song ) {
            return NULL;
        }
        if( random_song_index == song->index_songs_value ) {
            /* Found it :) */
            return song;
        }
        song = (song_node_t *) song->d.node.next->data;
    }
}

/**
 * @param start the lowest possible number that can be returned
 * @param stop  the highest possible number that can be returned
 * 
 * @note if an error occurs, start will be returned.
 * 
 * @return a random number in the range of [start, stop]
 */
uint32_t random_number_in_range( uint32_t start, uint32_t stop )
{
    uint32_t range = stop - start + 1;
    uint32_t rv;
    if( stop < start ) {
        return start;
    }
    
    rv = rand()/((float)INT32_MAX) * range;
    rv += start;
    return rv;
}
