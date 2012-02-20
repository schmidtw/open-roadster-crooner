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
#include "next_song.h"

#define NEXT_SONG_DEBUG 0

#define _D1(...)

#if (NEXT_SONG_DEBUG > 0)
#include <stdio.h>
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif


typedef struct {
    generic_node_types_t type;
    uint32_t index;
} __song_index_t;


int8_t __compare_indexed_general_song_search( __song_index_t* si, generic_node_t *node );
int8_t __compare_indexed_general( generic_node_t *node1, generic_node_t *node2 );
generic_node_t * find_random_song_from_generic( generic_node_t * generic, uint32_t first_song_index, uint32_t last_song_index );
uint32_t random_number_in_range( uint32_t start, uint32_t stop );

static generic_node_t *__ns_get_head( bt_list_t *list ) {
    bt_node_t * node = bt_get_head(list);
    if( NULL != node ) {
        return node->data;
    }
    return NULL;
}

static generic_node_t *__ns_get_tail( bt_list_t *list ) {
    bt_node_t * node = bt_get_tail(list);
    if( NULL != node ) {
        return node->data;
    }
    return NULL;
}

static db_status_t __get_next_item_from_list( generic_node_t ** node, bt_get_t next ) {
    bt_node_t *next_node = bt_get(&(*node)->parent->children, &(*node)->node, next);

    if( NULL != next_node ) {
        (*node) = (generic_node_t*)next_node->data;
        return DS_SUCCESS;
    }
    return DS_END_OF_LIST;
}

static generic_node_t * __ns_find( bt_list_t *list, void * data ) {
    bt_node_t * node = bt_find(list, data);

    if( NULL != node ) {
        return (generic_node_t*)node->data;
    }
    return NULL;
}

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
        generic_n = __ns_get_head(&rdn.root->children);
        while(1) {
            if( NULL == generic_n ) {
                return DS_FAILURE;
            }
            if( GNT_SONG == generic_n->type ) {
                break;
            }
            generic_n = __ns_get_head(&generic_n->children);
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
            /* no break */
        case DL_ALBUM:
            generic_n = (generic_node_t*)generic_n->parent;
            /* no break */
        default:
            /* DL_SONG */
            break;
    }

    switch( operation ) {
        case DT_NEXT:
            rv = __get_next_item_from_list( &generic_n, BT_GET__NEXT );
            if( DS_END_OF_LIST == rv ) {
                generic_n = __ns_get_head( &generic_n->parent->children );
            }
            break;
        case DT_PREVIOUS:
            rv = __get_next_item_from_list( &generic_n, BT_GET__PREVIOUS );
            if( DS_END_OF_LIST == rv ) {
                generic_n = __ns_get_tail( &generic_n->parent->children );
            }
            break;
        default:
            /* DT_RANDOM */
            rv = DS_SUCCESS;
            generic_n = (generic_node_t*)generic_n->parent;
            if( NULL == generic_n->children.root ) {
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
                generic_n = __ns_get_head( &generic_n->children );
                /* no break */
            case GNT_ARTIST:
                generic_n = __ns_get_head( &generic_n->children );
                /* no break */
            case GNT_ALBUM:
                generic_n = __ns_get_head( &generic_n->children );
                /* no break */
            case GNT_SONG:
                *current_song = (song_node_t*)generic_n;
                /* no break */
        }
    } else {
        *current_song = NULL;
        rv = DS_FAILURE;
    }
    return rv;
}

generic_node_t * find_random_song_from_generic( generic_node_t * generic,
        uint32_t first_song_index, uint32_t last_song_index )
{
    generic_node_t * generic_n = generic;
    uint32_t random_song_index =
            random_number_in_range(first_song_index, last_song_index);
    __song_index_t song_index;
    song_index.type = GNT_SONG_SEARCH_NODE;
    song_index.index = random_song_index;

    while(    ( NULL != generic_n )
           && ( GNT_SONG != generic_n->type ) )
    {
        generic_n = __ns_find( &generic_n->children, &song_index );
    }
    return generic_n;
}

int8_t compare_indexed_song( void * data1, void * data2 ) {
    uint32_t index1, index2;

    if( GNT_SONG_SEARCH_NODE == *(generic_node_types_t*)data1 ) {
        index1 = ((__song_index_t*)data1)->index;
    } else {
        index1 = ((song_node_t*)data1)->index_songs_value;
    }

    if( GNT_SONG_SEARCH_NODE == *(generic_node_types_t*)data2 ) {
        index2 = ((__song_index_t*)data2)->index;
    } else {
        index2 = ((song_node_t*)data2)->index_songs_value;
    }

    if( index1 < index2 ) {
        return -1;
    } else if ( index1 == index2 ) {
        return 0;
    } else {
        return 1;
    }
}

int8_t __compare_indexed_general_song_search( __song_index_t* si, generic_node_t *node )
{
    if( si->index < node->d.list.index_songs_start ) {
        return -1;
    } else if( si->index > node->d.list.index_songs_stop ) {
        return 1;
    }
    return 0;
}

int8_t __compare_indexed_general( generic_node_t *node1, generic_node_t *node2 )
{
    uint32_t range_1 = node1->d.list.index_songs_start;
    uint32_t range_2 = node2->d.list.index_songs_start;

    if( range_1 < range_2 ) {
        return -1;
    } else if( range_1 > range_2 ) {
        return 1;
    }
    return 0;
}

int8_t compare_indexed_general( void * data1, void * data2 ) {

    if( GNT_SONG_SEARCH_NODE == ((generic_node_t*)data1)->type ) {
        return __compare_indexed_general_song_search( (__song_index_t*)data1, (generic_node_t*)data2 );
    } else if( GNT_SONG_SEARCH_NODE == ((generic_node_t*)data2)->type ) {
        return -(__compare_indexed_general_song_search( (__song_index_t*)data2, (generic_node_t*)data1 ));
    }
    return __compare_indexed_general( (generic_node_t*)data1, (generic_node_t*)data2 );
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
