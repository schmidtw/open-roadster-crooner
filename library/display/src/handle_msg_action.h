#ifndef HANDLE_MSG_ACTION_H_
#define HANDLE_MSG_ACTION_H_

#include <stdbool.h>
#include "freertos/os.h"
#include "display_internal.h"

/**
 * @param ticks_to_wait_for_message pointer to variable which will be
 *        updated with the number of ticks to wait before the next message
 *        request will be made
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
 * 
 * @return TRUE if the text has been displayed to the screen and no more text
 *         operations should be done before waiting
 *         ticks_to_wait_before_looping.
 *         FALSE the gld.text_info.state has been updated and the normal text
 *         manipulation should be done immediately
 */
bool handle_msg_action( struct display_message * msg,
                        uint32_t * ticks_to_wait_for_message,
                        uint32_t * ticks_to_wait_before_looping,
                        char * text,
                        struct display_globals * ref );

#endif /* HANDLE_MSG_ACTION_H_ */
