#ifndef HANDLE_MSG_ACTION_H_
#define HANDLE_MSG_ACTION_H_

#include <stdbool.h>
#include "freertos/os.h"
#include "display_internal.h"

/**
 * @param ticks_to_wait_for_message pointer to variable which will be
 *        updated with the number of ticks to wait before the next message
 *        request will be made
 * @param ref pointer to the structure which has all the information for
 *        printing, state of the display state machine, etc.
 *        
 * @note  By passing ref parameter in, it simplifies unit testing and allows
 *        this function to live in a different file.
 */
void handle_msg_action( struct display_message * msg,
                        struct display_globals * ref );

#endif /* HANDLE_MSG_ACTION_H_ */
