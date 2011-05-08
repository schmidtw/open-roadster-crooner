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
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "handle_display_update.h"


/* See handle_display_update.h  for documentation */
void handle_display_update( struct display_globals * ref )
{
    bool is_scrolling_message = false;
    size_t nchars_disp = 0;
    char * text = ref->text_state.text_info.text;
    ref->text_state.next_draw_time = ref->scroll_speed;

    switch( ref->text_state.state ) {
        case SOD_NOT_DISPLAYING:
            break;
        case SOD_BEGINNING_OF_TEXT:
            nchars_disp = ref->text_print_fn( text );
            ref->text_state.display_offset = 0;
            is_scrolling_message = true;
            break;
        case SOD_END_OF_TEXT:
            ref->text_state.display_offset = ref->text_print_fn( text );
            ref->text_state.state = SOD_BEGINNING_OF_TEXT;
            ref->text_state.next_draw_time = ref->pause_at_beginning_of_text;
            break;
        case SOD_MIDDLE_OF_TEXT:
            nchars_disp = ref->text_print_fn( &(text[ref->text_state.display_offset]) );
            is_scrolling_message = true;
            break;
        case SOD_NO_SCROLLING_NEEDED:
        default:
            /* Because some user interactions dismiss the text,
             * this part of the loop will redraw the text so
             * the user can see the display again
             */
            ref->text_print_fn( text );
            break;
    }
    if( true == is_scrolling_message ) {
        if( ref->text_state.length == (ref->text_state.display_offset + nchars_disp) ) {
            ref->text_state.state = SOD_END_OF_TEXT;
            ref->text_state.next_draw_time = ref->pause_at_end_of_text;
        } else {
            ref->text_state.state = SOD_MIDDLE_OF_TEXT;
            if( ref->num_characters_to_shift < nchars_disp ) {
                ref->text_state.display_offset += ref->num_characters_to_shift;
            } else {
                ref->text_state.display_offset += nchars_disp;
            }
        }
    }
}
