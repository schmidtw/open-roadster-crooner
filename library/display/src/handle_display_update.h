#ifndef HANDLE_DISPLAY_UPDATE_H_
#define HANDLE_DISPLAY_UPDATE_H_

#include "freertos/os.h"
#include "display_internal.h"

/**
 * @param ref pointer to the structure which has all the information for
 *        printing, state of the display state machine, etc.
 *        
 * @note  By passing ref parameter in, it simplifies unit testing and allows
 *        this function to live in a different file.
 */
void handle_display_update( struct display_globals * ref );


#endif /* HANDLE_DISPLAY_UPDATE_H_ */
