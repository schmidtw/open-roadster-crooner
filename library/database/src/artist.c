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
#include "artist.h"
#include "album.h"
#include "w_malloc.h"

ll_node_t * get_new_artist_and_node( const char * name );

artist_node_t * find_or_create_artist( group_node_t * group,
                                       const char * artist,
                                       bool * created_node )
{
    artist_node_t * ar_n;
    ll_node_t * node_before_new_node = NULL;
    ll_node_t * node;
    int result;
    
    *created_node = false;
    
    if(    ( NULL == group )
        || ( NULL == artist ) ) {
        return NULL;
    }
    
    if(NULL == group->artists.head) {
        *created_node = true;
    } else {
        node = group->artists.head;
    }
    
    while( false == *created_node ) {
        ar_n = (artist_node_t *)node->data;
        result = strcmp(artist, ar_n->name);
        if( 0 == result ) {
            return ar_n;
        } else if ( 0 > result ) {
            /* This is a new artist which is to be created
             */
            *created_node = true;
        } else {
            node_before_new_node = node;
            /* The artist name we are looking for is after this node,
             * if it exists.
             */
            if( NULL == node->next ) {
                *created_node = true;
            } else {
                node = node->next;
            }
        }
    }
    
    node = get_new_artist_and_node( artist );
    if( NULL == node ) {
        return NULL;
    }
    ll_insert_after(&group->artists, node, node_before_new_node);
    group->size_list++;
    ar_n = (artist_node_t *)node->data;
    ar_n->group = group;
    
    return ar_n;
}

ll_node_t * get_new_artist_and_node( const char * name )
{
    artist_node_t * an;
    
    if( NULL == name ) {
        return NULL;
    }
    
    an = (artist_node_t *) w_malloc( sizeof(artist_node_t) );
    if( an == NULL ) {
        return NULL;
    }
    
    ll_init_node( &(an->node), an );
    strncpy( an->name, name, sizeof(char) * MAX_ARTIST_NAME);
    an->name[MAX_ARTIST_NAME] = '\0';
    ll_init_list(&(an->albums));
    
    return &(an->node);
}

void delete_artist(ll_node_t *node, volatile void *user_data)
{
    artist_node_t *an;
    
    an = (artist_node_t *)node->data;
    ll_delete_list(&an->albums, delete_album, NULL);
    an->group->size_list--;
    free(an);
}
