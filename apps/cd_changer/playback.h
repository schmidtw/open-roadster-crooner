#ifndef __PLAYBACK_H__
#define __PLAYBACK_H__

#include <stdint.h>

typedef enum {
    PB_CMD__album_next,
    PB_CMD__album_prev,
    PB_CMD__song_next,
    PB_CMD__song_prev,
    PB_CMD__play,
    PB_CMD__pause,
    PB_CMD__stop,
    PB_CMD__change_disc
} pb_command_t;

/**
 *  Used to initialize the playback system.
 *
 *  @returns 0 on success, -1 on error
 */
int32_t playback_init( void );

/**
 *  Used to command the playback system.
 *
 *  @param command the command to apply to the system
 *  @param disc the disc to change to if PB_CMD__change_disc is the command,
 *              ignored otherwise
 */
void playback_command( const pb_command_t command, const uint8_t disc );

#endif
