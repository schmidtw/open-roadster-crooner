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
#include "album.h"
#include "song.h"
#include "w_malloc.h"

ll_node_t * get_new_album_and_node( const char * name );

album_node_t * find_or_create_album( artist_node_t * artist,
                                     const char * album )
{
    album_node_t * al_n;
    ll_node_t * node_before_new_node = NULL;
    ll_node_t * node;
    bool create_node = false;
    int result;
    
    if(    ( NULL == artist )
        || ( NULL == album ) ) {
        return NULL;
    }
    
    if(NULL == artist->albums.head) {
        create_node = true;
    } else {
        node = artist->albums.head;
    }
    
    while( false == create_node ) {
        al_n = (album_node_t *)node->data;
        result = strcmp(album, al_n->name);
        if( 0 == result ) {
            return al_n;
        } else if ( 0 > result ) {
            /* This is a new album which is to be created
             */
            create_node = true;
        } else {
            node_before_new_node = node;
            /* The album name we are looking for is after this node,
             * if it exists.
             */
            if( NULL == node->next ) {
                create_node = true;
            } else {
                node = node->next;
            }
        }
    }
    
    node = get_new_album_and_node(album);
    if( NULL == node ) {
        return NULL;
    }
    ll_insert_after(&artist->albums, node, node_before_new_node);
    artist->size_list++;
    al_n = (album_node_t *)node->data;
    al_n->artist = artist;
    
    return al_n;
}

ll_node_t * get_new_album_and_node( const char * name )
{
    album_node_t * an;
    
    if( NULL == name ) {
        return NULL;
    }
    
    an = (album_node_t *) w_malloc( sizeof(album_node_t) );
    if( an == NULL ) {
        return NULL;
    }
    
    ll_init_node( &(an->node), an );
    strncpy( an->name, name, sizeof(char) * MAX_ALBUM_TITLE);
    an->name[MAX_ALBUM_TITLE] = '\0';
    ll_init_list(&(an->songs));
    
    return &(an->node);
}

void delete_album(ll_node_t *node, volatile void *user_data)
{
    album_node_t *an;
    
    an = (album_node_t *)node->data;
    ll_delete_list(&an->songs, delete_song, NULL);
    an->artist->size_list--;
    free(an);
}
