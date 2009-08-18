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
#include "w_malloc.h"

ll_node_t * get_new_song_and_node(
        const char * name,
        const uint16_t track_number,
        const double album_gain,
        const double album_peak,
        const double track_gain,
        const double track_peak,
        media_play_fn_t play_fn,
        const char * file_location );

song_node_t * find_or_create_song( album_node_t * album,
                                   const char * song,
                                   const uint16_t track_number,
                                   const double album_gain,
                                   const double album_peak,
                                   const double track_gain,
                                   const double track_peak,
                                   media_play_fn_t play_fn,
                                   const char * file_location )
{
    song_node_t * so_n;
    ll_node_t * node_before_new_node = NULL;
    ll_node_t * node;
    bool create_node = false;
    int result;
    
    if(    ( NULL == album )
        || ( NULL == song ) ) {
        return NULL;
    }
    
    if(NULL == album->songs.head ) {
        create_node = true;
    } else {
        node = album->songs.head;
    }
    
    while( false == create_node ) {
        so_n = (song_node_t *)node->data;
        if( track_number == so_n->track_number ) {
            result = strcmp(song, so_n->title);
            if( 0 == result ) {
                if( (so_n->album_gain != album_gain) ||
                    (so_n->album_peak != album_peak) ||
                    (so_n->album_gain != album_gain) ||
                    (so_n->album_peak != album_peak) )
                {
                    result = 1;
                }
            }

            if( 0 == result ) {
                return so_n;
            } else if ( 0 > result ) {
                /* This is a new song which is to be created
                 */
                create_node = true;
            } else {
                /* The song title we are looking for is after this node,
                 * if it exists.
                 */
                node_before_new_node = node;
                if( NULL == node->next ) {
                    create_node = true;
                } else {
                    node = node->next;
                }
            }
        } else if( track_number < so_n->track_number ) {
            /* This is a new song */
            create_node = true;
        } else {
            node_before_new_node = node;
            /* The song title we are looking for is after this node,
             * if it exists.
             */
            if( NULL == node->next ) {
                create_node = true;
            } else {
                node = node->next;
            }
        }
    }
    
    node = get_new_song_and_node(song, track_number, album_gain, album_peak,
                                 track_gain, track_peak, play_fn,
                                 file_location);
    if( NULL == node ) {
        return NULL;
    }
    ll_insert_after(&album->songs, node, node_before_new_node);
    album->size_list++;
    so_n = (song_node_t *)node->data;
    so_n->album = album;
    
    return so_n;
}

ll_node_t * get_new_song_and_node( const char * name,
        const uint16_t track_number, 
        const double album_gain,
        const double album_peak,
        const double track_gain,
        const double track_peak,
        media_play_fn_t play_fn, const char * file_location )
{
    song_node_t * sn;
    
    if(    ( NULL == name )
        || ( NULL == play_fn )
        || ( NULL == file_location ) )
    {
        return NULL;
    }
    
    sn = (song_node_t *) w_malloc( sizeof(song_node_t) );
    if( sn == NULL ) {
        return NULL;
    }
    
    ll_init_node( &(sn->node), sn );
    strncpy( sn->title, name, sizeof(char) * MAX_SONG_TITLE);
    sn->title[MAX_SONG_TITLE] = '\0';
    sn->track_number = track_number;
    sn->album_gain = album_gain;
    sn->album_peak = album_peak;
    sn->track_gain = track_gain;
    sn->track_peak = track_peak;
    sn->play_fn = play_fn;
    strcpy(sn->file_location, file_location);
    
    return &(sn->node);
}

void delete_song(ll_node_t *node, volatile void *user_data)
{
    song_node_t *sn;
    
    sn = (song_node_t *)node->data;
    sn->album->size_list--;
    free(sn);
}
