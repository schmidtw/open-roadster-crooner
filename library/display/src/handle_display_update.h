#ifndef HANDLE_DISPLAY_UPDATE_H_
#define HANDLE_DISPLAY_UPDATE_H_

#include "freertos/portmacro.h"
#include "display_internal.h"

/**
 * @param ticks_to_wait_before_looping pointer to variable which will be
 *        updated with the number of ticks to wait before trying to get the
 *        next message
 * @param text Must not be NULL.  Pointer to the text array.  The text string
 *        MUST be NULL terminated.
 * @param ref pointer to the structure which has all the information for
 *        printing, state of the display state machine, etc.
 *        
 * @note  By passing ref parameter in, it simplifies unit testing and allows
 *        this function to live in a different file.
 */
void handle_display_update( portTickType * ticks_to_wait_before_looping,
                            char * text,
                            struct display_globals * ref );


#endif /* HANDLE_DISPLAY_UPDATE_H_ */
