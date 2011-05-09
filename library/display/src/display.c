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
#include <stdio.h>
#include <sys/time.h>

#include "freertos/os.h"

#include "display.h"
#include "display_internal.h"
#include "handle_display_update.h"
#include "handle_msg_action.h"

#if (0 < DEBUG)
#define _D1(...) printf(__VA_ARGS__)
#else
#define _D1(...)
#endif

#define DISPLAY_QUEUE_LENGTH     5
#define DISPLAY_TASK_DEPTH       2000
#define DISPLAY_TASK_PRIORITY    1
#define DISPLAY_MSG_POST_DELAY   10

#define GRAB_MUTEX()       os_semaphore_take( gld.os.mutex_handle, WAIT_FOREVER);
#define RELEASE_MUTEX()    os_semaphore_give( gld.os.mutex_handle );

static struct display_globals gld;

/**
 * Internal function prototypes
 */
void display_main( void * parameters );
uint32_t __get_time_delta( struct timeval * v1, struct timeval * v2 );

/**
 * Public function prototypes
 */
/* See display.h for more info */
DRV_t display_init(text_print_fct text_print_fn,
        uint32_t scroll_speed,
        uint32_t pause_at_beginning_of_text,
        uint32_t pause_at_end_of_text,
        size_t num_characters_to_shift,
        bool repeat_text)
{
    bzero(&gld, sizeof(struct display_globals));
    if(NULL != text_print_fn) {
        _D1("%s:%d -- Scroll Speed                %ld\n", __FILE__, __LINE__, scroll_speed);
        _D1("%s:%d -- Pause at Beginning of Text  %ld\n", __FILE__, __LINE__, pause_at_beginning_of_text);
        _D1("%s:%d -- Pause at End of Text        %ld\n", __FILE__, __LINE__, pause_at_end_of_text);
        _D1("%s:%d -- Number of Chars to shift    %ld\n", __FILE__, __LINE__, num_characters_to_shift);
        _D1("%s:%d -- Text should %s\n", __FILE__, __LINE__, repeat_text?"Repeat":"Not Repeat");
        gld.text_print_fn = text_print_fn;
        gld.scroll_speed = scroll_speed;
        gld.pause_at_beginning_of_text = pause_at_beginning_of_text;
        gld.pause_at_end_of_text = pause_at_end_of_text;
        if( 0 == num_characters_to_shift ) {
            gld.num_characters_to_shift = SIZE_MAX;
        } else {
            gld.num_characters_to_shift = num_characters_to_shift;
        }
        gld.repeat = repeat_text;
    
        gld.os.queue_handle = os_queue_create(DISPLAY_QUEUE_LENGTH, sizeof(struct display_message));
        if( 0 != gld.os.queue_handle ) {
            gld.os.mutex_handle = os_semaphore_create_binary();
            if( NULL != gld.os.mutex_handle ) {
                if( true == os_task_create( display_main, "DISPLAY", DISPLAY_TASK_DEPTH,
                                NULL, DISPLAY_TASK_PRIORITY, &(gld.os.task_handle) ) )
                {
                    gld.valid = true;
                    _D1("%s:%d -- %s -- success\n", __FILE__, __LINE__, "init()");
                    return DRV_SUCCESS;
                }
                os_queue_delete( gld.os.queue_handle );
            }
        }
    }
    _D1("%s:%d -- %s -- failure", __FILE__, __LINE__, "init()");
    return DRV_NOT_INITIALIZED;
}

/* See display.h for more info */
DRV_t display_stop_text( void )
{
    return display_start_text( NULL );
}

/* See display.h for more info */
DRV_t display_start_text( const char *text_to_display )
{
    struct display_message msg;
    
    if( false == gld.valid ) {
        _D1("%s:%d -- %s -- not initialized\n", __FILE__, __LINE__, "start_text()");
        return DRV_NOT_INITIALIZED;
    }
    
    if( NULL == text_to_display ) {
        msg.action = DA_STOP;
    } else {
        msg.action = DA_START;
    }

    GRAB_MUTEX();
    gld.text_info.state = SOD_NOT_DISPLAYING;
    msg.identifier = ++gld.identifier;
    RELEASE_MUTEX();
    msg.text = text_to_display;


    while( true != os_queue_send_to_back( gld.os.queue_handle, &msg, DISPLAY_MSG_POST_DELAY ) ) {
        /* The queue is full, we should continue to try to post this message */
        /* TODO : revisit this idea.  We probably don't want to loop forever,
         * but don't want to fail silently.
         */
        ;
    }
    _D1("%s:%d -- %s -- success - `%s`\n", __FILE__, __LINE__, "start_text()", text_to_display);
    return DRV_SUCCESS;
}

/* See display.h for more info */
void display_main( void * parameters )
{
    struct timeval tv;
    struct timezone tz;
    struct display_message msg;
    bool recieved_message;
    
    _D1("%s:%d -- display_main() thread started\n", __FILE__, __LINE__);

    bzero( &tv, sizeof(struct timeval) );
    gettimeofday( &tv, &tz );

    while(1) {
        struct timeval now;
        recieved_message = os_queue_receive( gld.os.queue_handle, &msg, gld.scroll_speed );
        
        if( 0 != gettimeofday(&now, &tz) ) {
            /* We didn't get a valid time, so use the time we have in tv */
            memcpy(&now, &tv, sizeof(struct timeval));
        }
        _D1("%s:%d -- now(%d) vs tv(%d)\n", __FILE__, __LINE__, now.tv_sec, tv.tv_sec);

        if( true == recieved_message ) {
            _D1("%s:%d -- msg: action = %d -- identifier = %d\n",
                    __FILE__, __LINE__, msg.action, msg.identifier);
            GRAB_MUTEX();
            _D1("%s:%d -- id match: %s\n", __FILE__, __LINE__,
                    ( gld.identifier == msg.identifier )?"true":"false");
            if( gld.identifier == msg.identifier ) {
                handle_msg_action( &msg, &gld );
            }
            RELEASE_MUTEX();
        } else {
            uint32_t delta = __get_time_delta(&now, &tv);

            _D1("%s:%d --no msg received -- delta %ld -- next draw time %ld\n", __FILE__, __LINE__, delta, gld.next_draw_time);
            if( delta >= gld.next_draw_time ) {
                handle_display_update( &gld );
            } else {
                gld.next_draw_time -= delta;
            }
        }
        memcpy(&tv, &now, sizeof(struct timeval));
    }
}

/* See display.h for more info */
void display_destroy( void )
{
    if( true == gld.valid ) {
        gld.valid = false;
        os_task_delete( gld.os.task_handle );
        os_queue_delete( gld.os.queue_handle );
    }
}

/**
 * Gets the delta between v1 and v2.
 *
 * @return delta in milliseconds
 */
uint32_t __get_time_delta( struct timeval * v1, struct timeval * v2 )
{
    uint32_t delta;
    struct timeval *larger = v1;
    struct timeval *smaller = v2;
    if( (v1->tv_sec == v2->tv_sec)?
            (v1->tv_usec < v2->tv_usec):
            (v1->tv_sec  < v2->tv_sec) ) {
        larger = v2;
        smaller = v1;
    }
    delta = (larger->tv_sec - smaller->tv_sec) * 1000 + (larger->tv_usec - smaller->tv_usec) / 1000;
    return delta;
}
