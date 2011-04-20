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
    memset(&gld, '\0', sizeof(struct display_globals));
    if(NULL == text_print_fn) {
        goto failure0;
    }
    
    gld.text_print_fn = text_print_fn;
    gld.scroll_speed = scroll_speed;
    gld.pause_at_beginning_of_text = pause_at_beginning_of_text;
    gld.pause_at_end_of_text = pause_at_end_of_text;
    if( 0 == num_characters_to_shift ) {
        gld.num_characters_to_shift = 0xFFFFFFFF;
    } else {
        gld.num_characters_to_shift = num_characters_to_shift;
    }
    gld.repeat = repeat_text;
    
    gld.os.queue_handle = os_queue_create(DISPLAY_QUEUE_LENGTH, sizeof(struct display_message));
    if( 0 == gld.os.queue_handle ) {
        /* An error occurred */
        goto failure1;
    }
    gld.os.mutex_handle = os_semaphore_create_binary();
    if( NULL == gld.os.mutex_handle ) {
        goto failure2;
    }
    
    if( true != os_task_create( display_main, "DISPLAY", DISPLAY_TASK_DEPTH,
                                NULL, DISPLAY_TASK_PRIORITY, &(gld.os.task_handle) ) )
    {
        goto failure3;
    }
    
    gld.valid = true;
    _D1("%s:%d -- %s -- success\n", "display.c", __LINE__, "init()");
    return DRV_SUCCESS;
failure3:
failure2:
    os_queue_delete( gld.os.queue_handle );
failure1:
failure0:
    _D1("%s:%d -- %s -- failure", "display.c", __LINE__, "init()");
    return DRV_NOT_INITIALIZED;
}

/* See display.h for more info */
DRV_t display_stop_text( void )
{
    struct display_message msg;
    
    if( false == gld.valid ) {
        _D1("%s:%d -- %s -- not initialized\n", "display.c", __LINE__, "stop_text()");
        return DRV_NOT_INITIALIZED;
    }
    msg.action = DA_STOP;
    GRAB_MUTEX();
    msg.identifier = ++gld.os.identifier;
    RELEASE_MUTEX();
    while( true != os_queue_send_to_back( gld.os.queue_handle, &msg, DISPLAY_MSG_POST_DELAY ) ) {
        /* The queue is full, we should continue to try to post this message */
        /* TODO : revisit this idea.  We probably don't want to loop forever,
         * but don't want to fail silently.
         */
        ;
    }
    _D1("%s:%d -- %s -- success\n", "display.c", __LINE__, "stop_text()");
    return DRV_SUCCESS;
}

/* See display.h for more info */
DRV_t display_start_text( const char *text_to_display )
{
    struct display_message msg;
    
    if( false == gld.valid ) {
        _D1("%s:%d -- %s -- not initialized\n", "display.c", __LINE__, "start_text()");
        return DRV_NOT_INITIALIZED;
    }
    display_stop_text();
    
    if( MAX_DISPLAY_LENGTH < strlen(text_to_display) ) {
        _D1("%s:%d -- %s -- string too long\n", "display.c", __LINE__, "start_text()");
        return DRV_STRING_TO_LONG;
    }
    msg.action = DA_START;
    GRAB_MUTEX();
    strcpy( gld.text_info.text, text_to_display );
    msg.identifier = ++gld.os.identifier;
    RELEASE_MUTEX();
    while( true != os_queue_send_to_back( gld.os.queue_handle, &msg, DISPLAY_MSG_POST_DELAY ) ) {
        /* The queue is full, we should continue to try to post this message */
        /* TODO : revisit this idea.  We probably don't want to loop forever,
         * but don't want to fail silently.
         */
        ;
    }
    _D1("%s:%d -- %s -- success - `%s`\n", "display.c", __LINE__, "start_text()", text_to_display);
    return DRV_SUCCESS;
}

/* See display.h for more info */
void display_main( void * parameters )
{
    uint32_t ticks_to_wait_for_message = WAIT_FOREVER;
    uint32_t ticks_to_wait_before_looping = NO_WAIT;
    char local_text[MAX_DISPLAY_LENGTH];
    struct display_message msg;
    bool recieved_message;
    bool is_message_stale;
    
    while(1) {
        is_message_stale = false;
        recieved_message = os_queue_receive( gld.os.queue_handle, &msg, ticks_to_wait_for_message );
        
        if( true == recieved_message ) {
            _D1("msg:\n\taction = %d\n\tidentifier = %d\n", msg.action, msg.identifier);
            GRAB_MUTEX();
            is_message_stale = (gld.os.identifier==msg.identifier?false:true);
            if(    ( false == is_message_stale )
                && ( DA_START == msg.action ))
            {
                strcpy( local_text, gld.text_info.text );
            }
            RELEASE_MUTEX();
        }
        if( true == is_message_stale ) {
            continue;
        }
        if( true == recieved_message ) {
            if( true == handle_msg_action( &msg,
                            &ticks_to_wait_for_message,
                            &ticks_to_wait_before_looping,
                            local_text,
                            &gld) )
            {
                continue;
            }
        }
        handle_display_update( &ticks_to_wait_before_looping, local_text, &gld );    
        os_task_delay_ticks( ticks_to_wait_before_looping );
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
