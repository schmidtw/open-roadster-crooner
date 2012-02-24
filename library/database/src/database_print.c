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
    
    printf("==== Database ==========================\n");
    if( NULL != rdn.root )
    {
        printf(
#ifdef INT32_STANDARD_INT_SIZE
               " root (%u-%u)\n",
#else
               " root (%lu-%lu)\n",
#endif
               rdn.root->list.index_songs_start,
               rdn.root->list.index_songs_stop);
        bt_iterate( &rdn.root->list.children, artist_print, NULL, (void*)DISPLAY_OFFSET);
    }
}

bt_ir_t artist_print(bt_node_t *node, void *user_data)
{
    generic_node_t *artist = (generic_node_t*)node->data;
    int spaces = (int) user_data;
    
    printf(
#ifdef INT32_STANDARD_INT_SIZE
           "%*.*s %u-(%u-%u) %-*.*s\n",
#else
           "%*.*s %lu-(%lu-%lu) %-*.*s\n",
#endif
           spaces, spaces, " ",
           artist->index,
           artist->list.index_songs_start,
           artist->list.index_songs_stop,
           MAX_DISPLAY_ARTIST_LEN, MAX_DISPLAY_ARTIST_LEN, artist->name.artist );
    bt_iterate(&artist->list.children, album_print, NULL, (void*)(spaces + MAX_DISPLAY_ARTIST_LEN));
    return BT_IR__CONTINUE;
}

bt_ir_t album_print(bt_node_t *node, void *user_data)
{
    generic_node_t *album = (generic_node_t*)node->data;
    int spaces = (int) user_data;
    
    printf(
#ifdef INT32_STANDARD_INT_SIZE
           "%*.*s %u-(%u-%u) %-*.*s\n",
#else
           "%*.*s %lu-(%lu-%lu) %-*.*s\n",
#endif
           spaces, spaces, " ",
           album->index,
           album->list.index_songs_start,
           album->list.index_songs_stop,
           MAX_DISPLAY_ALBUM_LEN, MAX_DISPLAY_ALBUM_LEN, album->name.album );
    bt_iterate(&album->list.children, song_print, NULL, (void*)(spaces + MAX_DISPLAY_ALBUM_LEN));
    return BT_IR__CONTINUE;
}

bt_ir_t song_print(bt_node_t *node, void *user_data)
{
    song_node_t *song = (song_node_t*) node->data;
    int spaces = (int) user_data;
    printf(
#ifdef INT32_STANDARD_INT_SIZE
           "%*.*s %*.*u %u) %-*.*s  [% 3.3f:% 3.3f|% 3.3f:% 3.3f] -- %-*.*s\n",
#else
           "%*.*s %*.*lu %lu) %-*.*s  [% 3.3f:% 3.3f|% 3.3f:% 3.3f] -- %-*.*s\n",
#endif
           spaces, spaces, " ",
           MAX_TRACK_DIGITS, MAX_TRACK_DIGITS, get_song_number(song, DL_SONG),
           song->d.index,
           MAX_DISPLAY_SONG_LEN, MAX_DISPLAY_SONG_LEN, song->d.name.song,
           song->gain.album_gain, song->gain.album_peak,
           song->gain.track_gain, song->gain.track_peak,
           MAX_FILE_NAME, MAX_FILE_NAME, song->file_location );
    return BT_IR__CONTINUE;
}
