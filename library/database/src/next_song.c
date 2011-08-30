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

generic_node_t * find_random_song_from_generic( generic_node_t * generic, uint32_t first_song_index, uint32_t last_song_index );
uint32_t random_number_in_range( uint32_t start, uint32_t stop );

/* See database.h for information */
db_status_t next_song( song_node_t ** current_song,
                       const db_traverse_t operation,
                       const db_level_t level )
{
    generic_node_t *generic_n;
    db_status_t rv = DS_FAILURE;
    
    if(    ( false == rdn.initialized )
        || ( NULL == current_song )
        || ( 0 == rdn.root->d.list.size ) )
    {
        return DS_FAILURE;
    }
    
    if( NULL == *current_song ) {
        generic_n = (generic_node_t *)rdn.root->children.head->data;
        while( GNT_SONG != generic_n->type ) {
            if( NULL == generic_n->children.head ) {
                return DS_FAILURE;
            }
            generic_n = (generic_node_t *)generic_n->children.head->data;
        }
        *current_song = (song_node_t *)generic_n;
        if( DT_NEXT == operation ) {
            return DS_SUCCESS;
        }
    }
    generic_n = (generic_node_t*)*current_song;
    switch( level ) {
        case DL_ARTIST:
            generic_n = (generic_node_t*)generic_n->parent;
        case DL_ALBUM:
            generic_n = (generic_node_t*)generic_n->parent;
            /* break left out on purpose */
        default:
            /* DL_SONG */
            break;
    }

    switch( operation ) {
        case DT_NEXT:
            if( NULL == generic_n->node.next ) {
                generic_n = (generic_node_t*)generic_n->parent->children.head->data;
                rv = DS_END_OF_LIST;
            } else {
                generic_n = (generic_node_t*)generic_n->node.next->data;
                rv = DS_SUCCESS;
            }
            break;
        case DT_PREVIOUS:
            if( NULL == generic_n->node.prev ) {
                generic_n = (generic_node_t*)generic_n->parent->children.tail->data;
                rv = DS_END_OF_LIST;
            } else {
                generic_n = (generic_node_t*)generic_n->node.prev->data;
                rv = DS_SUCCESS;
            }
            break;
        default:
            /* DT_RANDOM */
            rv = DS_SUCCESS;
            generic_n = (generic_node_t*)generic_n->parent;
            if( NULL == generic_n->children.head ) {
                rv = DS_FAILURE;
            } else {
                generic_n = find_random_song_from_generic(
                        (generic_node_t *)generic_n,
                        generic_n->d.list.index_songs_start,
                        generic_n->d.list.index_songs_stop);
            }
            break;
    }
    if( NULL != generic_n ) {
        switch( generic_n->type ) {
            case GNT_ROOT:
                generic_n = (generic_node_t*)generic_n->children.head->data;
            case GNT_ARTIST:
                generic_n = (generic_node_t*)generic_n->children.head->data;
            case GNT_ALBUM:
                generic_n = (generic_node_t*)generic_n->children.head->data;
            case GNT_SONG:
                *current_song = (song_node_t*)generic_n;
        }
    } else {
        *current_song = NULL;
        rv = DS_FAILURE;
    }
    return rv;
}

generic_node_t * find_random_song_from_generic( generic_node_t * generic, uint32_t first_song_index, uint32_t last_song_index )
{
    generic_node_t * generic_n = generic;
    uint32_t random_song_index = random_number_in_range(first_song_index, last_song_index);

    while( NULL != generic_n ) {
        if( GNT_SONG == generic_n->type ) {
            if( random_song_index == ((song_node_t*)generic_n)->index_songs_value ) {
                return generic_n;
            }
        } else {
            if(    ( random_song_index >= generic_n->d.list.index_songs_start )
                && ( random_song_index <= generic_n->d.list.index_songs_stop ) ) {
                /* We have found the generic_n of interest, lets drill down into the child */
                generic_n = (generic_node_t*)generic_n->children.head->data;
                continue;
            }
        }
        generic_n = generic_n->node.next->data;
    }
    return NULL;
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
