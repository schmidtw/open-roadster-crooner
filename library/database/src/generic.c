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
#include <linked-list/linked-list.h>
#include "database.h"
#include "w_malloc.h"
#include "generic.h"
#include "song.h"

generic_node_t * find_or_create_generic( generic_node_t * generic,
        generic_compare_fn_t compare_fct,
        void * element,
        bool * created_node )
{
    generic_node_t * generic_n;
    ll_node_t * node_before_new_node = NULL;
    ll_node_t * node;
    int result;

    if(    ( NULL == element )
        || ( NULL == generic )
        || ( NULL == compare_fct )
        || ( NULL == created_node ) ) {
        return NULL;
    }
    *created_node = false;

    if(NULL == generic->children.head) {
        *created_node = true;
    } else {
        node = generic->children.head;
    }

    while( false == *created_node ) {
        generic_n = (generic_node_t *)node->data;
        result = compare_fct(element, (void*)generic_n);
        if( 0 == result ) {
            return generic_n;
        } else if ( 0 > result ) {
            /* This is a new album which is to be created
             */
            *created_node = true;
        } else {
            node_before_new_node = node;
            /* The album name we are looking for is after this node,
             * if it exists.
             */
            if( NULL == node->next ) {
                *created_node = true;
            } else {
                node = node->next;
            }
        }
    }
    /* The type of the newly created node will be be one greater
     * than the parent.
     */
    node = get_new_generic_node( (generic->type+1) , element );
    if( NULL == node ) {
        return NULL;
    }
    ll_insert_after(&generic->children, node, node_before_new_node);
    generic->d.list.size++;
    generic_n = (generic_node_t *)node->data;
    generic_n->parent = generic;

    return generic_n;
}

int8_t generic_compare( const void * element1, const generic_node_t * element2 )
{
    return ( strcasecmp((char*)element1, element2->name.album) );
}

int8_t song_compare( const void * element1, const generic_node_t * element2 )
{
    song_create_t * sc1 = (song_create_t*)element1;
    int32_t result = sc1->metadata->track_number - ((song_node_t*)element2)->track_number;
    if( 0 == result ) {
        result = strcasecmp(sc1->metadata->title, element2->name.song);
        if( 0 == result ) {
            if(    ( sc1->metadata->album_gain != element2->d.gain.album_gain )
                || ( sc1->metadata->album_peak != element2->d.gain.album_peak )
                || ( sc1->metadata->track_gain != element2->d.gain.track_gain )
                || ( sc1->metadata->track_peak != element2->d.gain.track_peak ) ) {
                result = 1;
            }
        }
    }
    if( 0 > result ) {
        return -1;
    } else if( 0 < result ) {
        return 1;
    }
    return 0;
}

ll_node_t * get_new_generic_node( const generic_node_types_t type, const void * element )
{
    generic_node_t * generic_n;
    size_t name_size;
    char * name_loc;

    if( NULL == element ) {
        return NULL;
    }

    if( GNT_SONG == type ) {
        generic_n = (generic_node_t *) w_malloc( sizeof(song_node_t) );
    } else {
        generic_n = (generic_node_t *) w_malloc( sizeof(generic_node_t) );
    }
    if( generic_n == NULL ) {
        return NULL;
    }

    ll_init_node( &(generic_n->node), generic_n );

    ll_init_list(&(generic_n->children));
    generic_n->type = type;

    {
        bool do_name_copy = true;
        switch(type) {
            case GNT_ALBUM:
                name_size = MAX_ALBUM_TITLE;
                name_loc = generic_n->name.album;
                break;
            case GNT_ARTIST:
                name_size = MAX_ARTIST_NAME;
                name_loc = generic_n->name.artist;
                break;
            case GNT_ROOT:
                name_size = MAX_ROOT_NAME;
                name_loc = generic_n->name.root;
                break;
            case GNT_SONG:
            default:
            {
                do_name_copy = false;
                song_create_t * meta = (song_create_t*) element;
                song_node_t * sn = (song_node_t*)generic_n;
                if(    ( NULL == meta->metadata )
                    || ( NULL == meta->file_location )
                    || ( NULL == meta->metadata->title )
                    || ( NULL == meta->play_fn ) )
                {
                    delete_generic(&(generic_n->node), NULL);
                    return NULL;
                }
                strncpy( generic_n->name.song, meta->metadata->title, MAX_SONG_TITLE );
                generic_n->name.song[MAX_SONG_TITLE] = '\0';

                sn->track_number = meta->metadata->track_number;
                sn->d.d.gain.album_gain = meta->metadata->album_gain;
                sn->d.d.gain.album_peak = meta->metadata->album_peak;
                sn->d.d.gain.track_gain = meta->metadata->track_gain;
                sn->d.d.gain.track_peak = meta->metadata->track_peak;
                sn->play_fn = meta->play_fn;
                strcpy(sn->file_location, meta->file_location);
                break;
            }
        }
        if( do_name_copy ) {
            strncpy( name_loc, (char*)element, sizeof(char) * name_size);
            name_loc[name_size] = '\0';
        }
    }

    return &(generic_n->node);
}

void delete_generic(ll_node_t *node, volatile void *user_data)
{
    generic_node_t *generic_n;

    generic_n = (generic_node_t *)node->data;
    if( GNT_SONG != generic_n->type ) {
        ll_delete_list(&(generic_n->children), delete_generic, NULL);
    }
    if( NULL != generic_n->parent ) {
        generic_n->parent->d.list.size--;
    }
    free(generic_n);
}
