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

#include "freertos/portmacro.h"
#include "display_internal.h"


/* See handle_msg_action.h for documentation */
bool handle_msg_action( struct display_message * msg,
                        portTickType * ticks_to_wait_for_message,
                        portTickType * ticks_to_wait_before_looping,
                        char * text,
                        struct display_globals * ref )
{
    size_t nchars_disp;
    switch( msg->action ) {
        case DA_START:
            *ticks_to_wait_for_message = 0;
            ref->text_info.length = strlen( text );
            nchars_disp = ref->text_print_fn( text );
            if( nchars_disp == ref->text_info.length ) {
                /* The entire text displays on the screen.  No need for
                 * scrolling.
                 */
                ref->text_info.state = SOD_NO_SCROLLING_NEEDED;
                /* Because some user interactions dismiss the text,
                 * the redraw period will be the same as the scroll
                 * speed
                 */
                *ticks_to_wait_before_looping = ref->scroll_speed;
            } else {
                ref->text_info.state = SOD_MIDDLE_OF_TEXT;
                if( ref->num_characters_to_shift < nchars_disp ) {
                    ref->text_info.display_offset += ref->num_characters_to_shift;
                } else {
                    ref->text_info.display_offset += nchars_disp;
                }
                *ticks_to_wait_before_looping = ref->pause_at_beginning_of_text;
            }
            return true;
        case DA_STOP:
        default:
            /* DA_STOP */
            ref->text_info.state = SOD_NOT_DISPLAYING;
            *ticks_to_wait_for_message = portMAX_DELAY;
            *ticks_to_wait_before_looping = 0;
    }
    return false;
}
