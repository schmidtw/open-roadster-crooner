#ifndef  __DISPLAY_H__
#define __DISPLAY_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "freertos/os.h"

typedef enum {
    DRV_NOT_INITIALIZED,
    DRV_STRING_TO_LONG,
    DRV_DISPLAY_FAILURE,
    DRV_SUCCESS
} DRV_t;

/**
 * The function which will actually display the message
 * to the screen.  The buffer won't be accessed after
 * the return of this function.  A local copy of this
 * buffer is kept.
 * 
 * @param string Pointer to the string which is to be
 *               displayed.
 * @return The number of characters from the string which
 *         are being displayed to the screen.
 */
typedef size_t (*text_print_fct)(char *string);

/**
 * Sets up the text displaying to the radio.
 * 
 * @param text_print_fn see above
 * @param scroll_speed The delay before the text if shifted left to show
 *        longer titles.
 * @param pause_at_beginning_of_text The delay when the first part of the
 *        text is shown.
 * @param pause_at_end_of_text The delay when the last part of the text
 *        is shown.
 * @param redraw_no_scrolling The delay before the text should be redrawn
 *        if the entire message fits on the display
 * @param num_characters_to_shift When the text is to be shifted, the
 *        number of characters which we want to shift off the display is
 *        specified by this value.  A value of 0 (Zero) is not valid and
 *        and will be interpreted as shift as many characters as possible
 *        without skipping characters.
 * @param repeat_text True if the string should be displayed over and over
 *        False if the string should only be displayed once.
 * 
 * @return DRV_SUCCESS when the initialization succeeds.  Else check the error
 *         code.
 */
DRV_t display_init(text_print_fct text_print_fn,
        uint32_t scroll_speed,
        uint32_t pause_at_beginning_of_text,
        uint32_t pause_at_end_of_text,
        uint32_t redraw_no_scrolling,
        size_t num_characters_to_shift,
        bool repeat_text);

/**
 * Stops the text from being displayed to the radio
 * 
 * @return DRV_SUCCESS on success, else check the error code.
 */
DRV_t display_stop_text( void );

/**
 * Displays the text pointed to by the parameters.  The text display
 * behavior is dictated by the display_init.
 * 
 * @param text_to_display must be '/0' terminated.  After calling this
 * function the data pointed to by text_to_display will no longer be
 * referenced.  The string will copied into a buffer to push to the
 * radio.
 * 
 * @return DRV_SUCCESS on success, else check the error code.
 */
DRV_t display_start_text( const char *text_to_display );

/**
 * Destroy the display library.  All queues/mutexes/tasks will be destroyed
 */
void display_destroy( void );

#endif /* __DISPLAY_H__ */
