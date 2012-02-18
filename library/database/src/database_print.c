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
#include <stdio.h>
#include <linked-list/linked-list.h>
#include "database.h"
#include "internal_database.h"
#include "database_print.h"

#define DISPLAY_OFFSET          2
#define MAX_DISPLAY_ARTIST_LEN  20
#define MAX_DISPLAY_ALBUM_LEN   20
#define MAX_DISPLAY_SONG_LEN    30
#define MAX_TRACK_DIGITS        5
#define MAX_FILE_NAME           35

#ifdef UNIT_TEST
#define INT32_STANDARD_INT_SIZE
#endif

void database_print( void )
{
    generic_node_t * gn;
    
    printf("==== Database ==========================\n");
    if(    ( NULL != rdn.root )
        && ( NULL != rdn.root->children.head ) )
    {
        printf(
#ifdef INT32_STANDARD_INT_SIZE
               " root (%u-%u)\n",
#else
               " root (%lu-%lu)\n",
#endif
               rdn.root->d.list.index_songs_start,
               rdn.root->d.list.index_songs_stop);
        gn = (generic_node_t *)rdn.root->children.head->data;
        while( NULL != gn ) {
            artist_print( gn, DISPLAY_OFFSET );
            if( NULL != gn->node.next ) {
                gn = (generic_node_t *)gn->node.next->data;
                printf("------------------------------\n");
            } else {
                break;
            }
        }
    }
}

void artist_print( generic_node_t * artist, int spaces )
{
    generic_node_t *al_n;
    
    printf(
#ifdef INT32_STANDARD_INT_SIZE
           "%*.*s %u-(%u-%u) %-*.*s\n",
#else
           "%*.*s %lu-(%lu-%lu) %-*.*s\n",
#endif
           spaces, spaces, " ",
           artist->d.list.index,
           artist->d.list.index_songs_start,
           artist->d.list.index_songs_stop,
           MAX_DISPLAY_ARTIST_LEN, MAX_DISPLAY_ARTIST_LEN, artist->name.artist );
    if( NULL == artist->children.head ) {
        printf("%*.*s Why are the albums NULL?\n", spaces, spaces, " ");
        return;
    }
    al_n = (generic_node_t *)artist->children.head->data;
    while( NULL != al_n ) {
        album_print( al_n, (spaces + MAX_DISPLAY_ARTIST_LEN) );
        if( NULL != al_n->node.next ) {
            al_n = (generic_node_t *)al_n->node.next->data;
        } else {
            break;
        }
    }
}

void album_print( generic_node_t * album, int spaces )
{
    generic_node_t *so_n;
    
    printf(
#ifdef INT32_STANDARD_INT_SIZE
           "%*.*s %u-(%u-%u) %-*.*s\n",
#else
           "%*.*s %lu-(%lu-%lu) %-*.*s\n",
#endif
           spaces, spaces, " ",
           album->d.list.index,
           album->d.list.index_songs_start,
           album->d.list.index_songs_stop,
           MAX_DISPLAY_ALBUM_LEN, MAX_DISPLAY_ALBUM_LEN, album->name.album );
    if( NULL == album->children.head ) {
        printf("%*.*s Why are the songs NULL?\n", spaces, spaces, " ");
        return;
    }
    so_n = (generic_node_t *)album->children.head->data;
    while( NULL != so_n ) {
        song_print( so_n, (spaces + MAX_DISPLAY_ARTIST_LEN) );
        if( NULL != so_n->node.next ) {
            so_n = (generic_node_t *)so_n->node.next->data;
        } else {
            break;
        }
    }
}

void song_print( generic_node_t * song, int spaces )
{
    printf(
#ifdef INT32_STANDARD_INT_SIZE
           "%*.*s %*.*u %u) %-*.*s  [% 3.3f:% 3.3f|% 3.3f:% 3.3f] -- %-*.*s\n",
#else
           "%*.*s %*.*u %lu) %-*.*s  [% 3.3f:% 3.3f|% 3.3f:% 3.3f] -- %-*.*s\n",
#endif
           spaces, spaces, " ",
           MAX_TRACK_DIGITS, MAX_TRACK_DIGITS, ((song_node_t*)song)->track_number,
           ((song_node_t*)song)->index_songs_value,
           MAX_DISPLAY_SONG_LEN, MAX_DISPLAY_SONG_LEN, song->name.song,
           song->d.gain.album_gain, song->d.gain.album_peak,
           song->d.gain.track_gain, song->d.gain.track_peak,
           MAX_FILE_NAME, MAX_FILE_NAME, ((song_node_t*)song)->file_location );
}
