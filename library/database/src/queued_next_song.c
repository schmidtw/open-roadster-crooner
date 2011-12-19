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
#include "database.h"
#include "queued_next_song.h"
#include <linked-list/linked-list.h>
#include <stdlib.h>
#include <string.h>

#define MAX_QUEUED_LINKED_LIST 100

ll_list_t queued_song_list;
size_t linked_list_size;

typedef struct  {
    ll_node_t node;
    song_node_t * song_info;
} queued_node_t;

bool queued_song_init()
{
    ll_init_list( &queued_song_list );
    linked_list_size = 0;
    return true;
}

void queued_song_deleter(ll_node_t *node, volatile void *user_data)
{
    free(node);
}

void queued_song_clear()
{
    ll_delete_list(&queued_song_list, queued_song_deleter, NULL);
    linked_list_size = 0;
}

db_status_t queued_next_song( song_node_t ** current_song,
                             const db_traverse_t operation,
                             const db_level_t level )
{
    db_status_t rv;
    queued_node_t *node;
    if( DT_PREVIOUS == operation ) {
        /* Because we put the last played song on the queue, if we
         * want to go to the previous song, we need to remove the
         * last song, and the next song will be our song of interest
         */
        node = (queued_node_t*)ll_remove_head( &queued_song_list );
        if( NULL != node ) {
            linked_list_size--;
            free(node);
            node = (queued_node_t*)ll_remove_head( &queued_song_list );
            if( NULL != node ) {
                (*current_song) = node->song_info;
                ll_prepend(&queued_song_list, &(node->node));
                return DS_SUCCESS;
            }
        }
    }

    /* Failed to get the previous song from the list */
    rv = next_song(current_song, DT_RANDOM, level);
    if(    ( DS_SUCCESS == rv )
        || ( DS_END_OF_LIST == rv ) ) {
        node = malloc( sizeof(queued_node_t) );
        node->node.data = (void *) node;
        if( NULL != node ) {
            linked_list_size++;
            node->song_info = *current_song;
            ll_prepend(&queued_song_list, &(node->node));
        }
    }
    if( MAX_QUEUED_LINKED_LIST < linked_list_size ) {
        ll_node_t * tail = queued_song_list.tail;
        if( NULL != tail ) {
            ll_remove(&queued_song_list, tail);
            free(tail);
            linked_list_size--;
        }
    }
    return rv;
}

