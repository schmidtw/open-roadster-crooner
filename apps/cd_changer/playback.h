#ifndef __PLAYBACK_H__
#define __PLAYBACK_H__

void playback_init( void );

void playback_song_next( void );
void playback_song_prev( void );
void playback_disc( const uint8_t disc );
void playback_stop( void );
void playback_play( void );
#endif
