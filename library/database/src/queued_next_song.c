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
#include <circular-buffer/circular-buffer.h>
#include <stdlib.h>
#include <string.h>

#define MAX_QUEUED_LINKED_LIST 100

void * queued_song_list = NULL;

typedef struct  {
    song_node_t * song_info;
} queued_node_t;

bool queued_song_init()
{
    if( NULL == queued_song_list ) {
        queued_song_list = cb_create_list(sizeof(queued_node_t), MAX_QUEUED_LINKED_LIST);
    } else {
        queued_song_clear();
    }
    return true;
}

void queued_song_clear()
{
    cb_clear_list(queued_song_list);
}

db_status_t queued_next_song( song_node_t ** current_song,
                             const db_traverse_t operation,
                             const db_level_t level )
{
    queued_node_t node;
    db_status_t rv;
    if( DT_PREVIOUS == operation ) {
        queued_node_t node;
        /* Because we put the last played song on the queue, if we
         * want to go to the previous song, we need to remove the
         * last song, and the next song will be our song of interest
         */
        if( cb_pop(queued_song_list, (void*)&node) ) {
            queued_node_t *tmp = (queued_node_t *)cb_peek_tail(queued_song_list);
            if( NULL != tmp ) {
                (*current_song) = tmp->song_info;
                return DS_SUCCESS;
            }
        }
    }

    /* Failed to get the previous song from the list */
    rv = next_song(current_song, DT_RANDOM, level);
    if(    ( DS_SUCCESS == rv )
        || ( DS_END_OF_LIST == rv ) ) {
        node.song_info = *current_song;
        cb_push(queued_song_list, &node);
    }
    return rv;
}

