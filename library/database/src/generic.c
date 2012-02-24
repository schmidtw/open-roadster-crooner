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

#include <stdlib.h>
#include <string.h>
#include <binary-tree-avl/binary-tree-avl.h>
#include "database.h"
#include "w_malloc.h"
#include "generic.h"
#include "song.h"

#define COMPARE_TWO_VALUES( x, y ) \
    ((x)==(y)?0:               \
      ((x)>(y)?1:-1))

int8_t __generic_compare_to_generic_create( const generic_holder_t* holder, const generic_node_t *node );
int8_t __generic_compare( const generic_node_t* node1, const generic_node_t *node2 );
int8_t __song_compare_to_song_create( const song_create_t * sc, const song_node_t * sn );
int8_t __song_compare( const song_node_t * sn1, const song_node_t * sn2 );
int8_t __song_compare_gain( const media_gain_t *g1,
                            const media_gain_t *g2 );

generic_node_t * find_or_create_generic( generic_node_t * generic,
        void * element,
        bool * created_node )
{
    generic_node_t * generic_n;
    bt_node_t * node;

    if(    ( NULL == element )
        || ( NULL == generic )
        || ( NULL == created_node ) ) {
        return NULL;
    }
    *created_node = false;

    node = bt_find( &generic->list.children, element);
    if( NULL == node ) {
        *created_node = true;
        /* The type of the newly created node will be be one greater
         * than the parent.
         */
        if( GNT_GENERIC_CREATE_NODE == *(generic_node_types_t*)element ) {
            node = get_new_generic_node( (generic->type+1), (void*)((generic_holder_t*)element)->string );
        } else {
            node = get_new_generic_node( (generic->type+1), element );
        }
        if( NULL == node ) {
            return NULL;
        }
        bt_add(&generic->list.children, node);
    }
    generic_n = (generic_node_t *)node->data;
    generic_n->parent = generic;

    return generic_n;
}

int8_t generic_compare( const void * element1, const void * element2 )
{
    if( GNT_GENERIC_CREATE_NODE == *(generic_node_types_t*)element1 ) {
        return __generic_compare_to_generic_create( (generic_holder_t*) element1, (generic_node_t*)element2 );
    } else if( GNT_GENERIC_CREATE_NODE == *(generic_node_types_t*)element2 ) {
        return -(__generic_compare_to_generic_create( (generic_holder_t*) element2, (generic_node_t*)element1 ));
    }
    return __generic_compare( (generic_node_t*) element1, (generic_node_t*)element2 );
}

int8_t __generic_compare_to_generic_create( const generic_holder_t* holder, const generic_node_t *node )
{
    int rv = strcasecmp(holder->string, node->name.album);
    return COMPARE_TWO_VALUES(rv, 0);
}

int8_t __generic_compare( const generic_node_t* node1, const generic_node_t *node2 )
{
    int rv = strcasecmp(node1->name.album, node2->name.album);
    return COMPARE_TWO_VALUES(rv, 0);
}

int8_t song_compare( const void * element1, const void * element2 )
{
    if( GNT_SONG_CREATE_NODE == *(generic_node_types_t*)element1 ) {
        return __song_compare_to_song_create( (song_create_t*)element1, (song_node_t*)element2 );
    } else if( GNT_SONG_CREATE_NODE == *(generic_node_types_t*)element2 ) {
        return -(__song_compare_to_song_create( (song_create_t*)element2, (song_node_t*)element1 ));
    }
    return __song_compare((song_node_t*)element1, (song_node_t*)element2);
}

int8_t __song_compare_to_song_create( const song_create_t * sc, const song_node_t * sn )
{
    int8_t result = COMPARE_TWO_VALUES(sc->metadata->track_number, sn->track_number);
    if( 0 == result ) {
        result = strcasecmp(sc->metadata->title, sn->d.name.song);
        if( 0 == result ) {
            result = __song_compare_gain( &sc->metadata->gain, &sn->gain );
        }
    }
    return result;
}

int8_t __song_compare( const song_node_t * sn1, const song_node_t * sn2 )
{
    int8_t result = COMPARE_TWO_VALUES(sn1->track_number, sn2->track_number);
    if( 0 == result ) {
        result = strcasecmp(sn1->d.name.song, sn2->d.name.song);
        if( 0 == result ) {
            result = __song_compare_gain( &sn1->gain, &sn2->gain );
        }
    }
    return result;
}

int8_t __song_compare_gain( const media_gain_t *g1,
                            const media_gain_t *g2 )
{
    int compare_rslt = memcmp( g1, g2, sizeof(media_gain_t) );
    return COMPARE_TWO_VALUES( compare_rslt, 0 );
}

bool __init_generic_node( generic_node_t *node, const char *element ) {
    size_t name_size;
    char * name_loc;

    bt_init_list( &(node->list.children), generic_compare );
    /* Artist and album share the same size */
    name_size = MAX_ALBUM_TITLE;
    /* Root, Artist, and Album share the same name location */
    name_loc = node->name.album;
    switch(node->type) {
        case GNT_ALBUM:
            bt_set_compare(&(node->list.children), song_compare);
            break;
        case GNT_ROOT:
            name_size = MAX_ROOT_NAME;
            break;
        default:
            break;
    }
    strncpy( name_loc, element, sizeof(char) * name_size);
    name_loc[name_size] = '\0';
    return true;
}

bool __init_song_node( song_node_t *sn, const song_create_t *meta ) {
    if(    ( NULL == meta->metadata )
        || ( NULL == meta->file_location )
        || ( NULL == meta->metadata->title )
        || ( NULL == meta->play_fn ) )
    {
        return false;
    }
    strncpy( sn->d.name.song, meta->metadata->title, MAX_SONG_TITLE );
    sn->d.name.song[MAX_SONG_TITLE] = '\0';

    sn->track_number = meta->metadata->track_number;
    memcpy( &sn->gain, &meta->metadata->gain, sizeof(media_gain_t) );
    sn->play_fn = meta->play_fn;
    strcpy(sn->file_location, meta->file_location);
    return true;
}

typedef bool (*__init_generic_ptr)( generic_node_t*, const void *element );

bt_node_t * get_new_generic_node( const generic_node_types_t type, const void * element )
{
    generic_node_t * generic_n;
    __init_generic_ptr init_fct;

    if( NULL == element ) {
        return NULL;
    }

    if( GNT_SONG == type ) {
        generic_n = (generic_node_t *) w_malloc( sizeof(song_node_t) );
        init_fct = __init_song_node;
    } else {
        generic_n = (generic_node_t *) w_malloc( sizeof(generic_node_t) );
        init_fct = __init_generic_node;
    }
    if( generic_n == NULL ) {
        return NULL;
    }

    generic_n->type = type;
    bt_init_node( &(generic_n->node), generic_n );
    if( false == init_fct( generic_n, element ) ) {
        delete_generic( &(generic_n->node), NULL );
        return NULL;
    }
    return &(generic_n->node);
}

void delete_generic(bt_node_t *node, void *user_data)
{
    generic_node_t *generic_n;

    generic_n = (generic_node_t *)node->data;
    if( GNT_SONG != generic_n->type ) {
        bt_delete_list(&(generic_n->list.children), delete_generic, NULL);
    }
    free(generic_n);
}
