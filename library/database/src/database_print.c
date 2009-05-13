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

#define DISPLAY_OFFSET          5
#define MAX_DISPLAY_GROUP_LEN   20
#define MAX_DISPLAY_ARTIST_LEN  20
#define MAX_DISPLAY_ALBUM_LEN   20
#define MAX_DISPLAY_SONG_LEN    30
#define MAX_TRACK_DIGITS        3
#define MAX_FILE_NAME           35


void database_print( void )
{
    group_node_t * gn;
    
    if( NULL != rdn.groups.head ) {
        gn = (group_node_t *)rdn.groups.head->data;
        while( NULL != gn ) {
            group_print( gn, DISPLAY_OFFSET );
            if( NULL != gn->node.next ) {
                gn = (group_node_t *)gn->node.next->data;
                printf("------------------------------\n");
            } else {
                break;
            }
        }
    }
}

void group_print( group_node_t * group, int spaces )
{
    artist_node_t *ar_n;
    
    ar_n = (artist_node_t *)group->artists.head->data;
    printf("%*.*s Group: %-*.*s\n",
            spaces, spaces, " ",
            MAX_DISPLAY_GROUP_LEN, MAX_DISPLAY_GROUP_LEN, group->name );
    
    while( NULL != ar_n ) {
        artist_print( ar_n, (spaces + MAX_DISPLAY_GROUP_LEN + 8 ) );
        if( NULL != ar_n->node.next ) {
            ar_n = (artist_node_t *)ar_n->node.next->data;
        } else {
            break;
        }
    }
}

void artist_print( artist_node_t * artist, int spaces )
{
    album_node_t *al_n;
    
    al_n = (album_node_t *)artist->albums.head->data;
    
    printf("%*.*s %-*.*s\n",
            spaces, spaces, " ",
            MAX_DISPLAY_ARTIST_LEN, MAX_DISPLAY_ARTIST_LEN, artist->name );
    
    while( NULL != al_n ) {
        album_print( al_n, (spaces + MAX_DISPLAY_ARTIST_LEN) );
        if( NULL != al_n->node.next ) {
            al_n = (album_node_t *)al_n->node.next->data;
        } else {
            break;
        }
    }
}

void album_print( album_node_t * album, int spaces )
{
    song_node_t *so_n;
        
    so_n = (song_node_t *)album->songs.head->data;
    
    printf("%*.*s %-*.*s\n",
            spaces, spaces, " ",
            MAX_DISPLAY_ALBUM_LEN, MAX_DISPLAY_ALBUM_LEN, album->name );
    
    while( NULL != so_n ) {
        song_print( so_n, (spaces + MAX_DISPLAY_ARTIST_LEN) );
        if( NULL != so_n->node.next ) {
            so_n = (song_node_t *)so_n->node.next->data;
        } else {
            break;
        }
    }
}

void song_print( song_node_t * song, int spaces )
{
    printf("%*.*s %*.*d.) %-*.*s  -- %*.*s\n",
            spaces, spaces, " ",
            MAX_TRACK_DIGITS, MAX_TRACK_DIGITS, song->track_number,
            MAX_DISPLAY_SONG_LEN, MAX_DISPLAY_SONG_LEN, song->title,
            MAX_FILE_NAME, MAX_FILE_NAME, song->file_location );
}
