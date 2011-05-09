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
#include <stddef.h>
#include <string.h>

#include "freertos/os.h"
#include "display_internal.h"

#if (0 < DEBUG)
#include <stdio.h>
#define _D1(...) printf(__VA_ARGS__)
#else
#define _D1(...)
#endif

/* See handle_msg_action.h for documentation */
void handle_msg_action( struct display_message * msg,
                        struct display_globals * ref )
{
    size_t nchars_disp;
    switch( msg->action ) {
        case DA_START:
        {
            char * text;
            ref->text_state.text_info.text = msg->text_info.text;
            text = ref->text_state.text_info.text;
            _D1("%s:%d -- handle_msg_action -- DA_START - '%s'\n", __FILE__, __LINE__, text);
            ref->text_state.length = strlen( text );
            nchars_disp = ref->text_print_fn( text );
            if( nchars_disp == ref->text_state.length ) {
                /* The entire text displays on the screen.  No need for
                 * scrolling.
                 */
                ref->text_state.state = SOD_NO_SCROLLING_NEEDED;
                /* Because some user interactions dismiss the text,
                 * the redraw period will be the same as the scroll
                 * speed
                 */
                ref->text_state.next_draw_time = ref->scroll_speed;
            } else {
                ref->text_state.state = SOD_MIDDLE_OF_TEXT;
                if( ref->num_characters_to_shift < nchars_disp ) {
                    ref->text_state.display_offset += ref->num_characters_to_shift;
                } else {
                    ref->text_state.display_offset += nchars_disp;
                }
                ref->text_state.next_draw_time = ref->pause_at_beginning_of_text;
            }
            break;
        }
        case DA_STOP:
        default:
            /* DA_STOP */
            _D1("%s:%d -- handle_msg_action -- DA_STOP\n", __FILE__, __LINE__);
            ref->text_state.state = SOD_NOT_DISPLAYING;
    }
}
