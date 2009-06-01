#ifndef __RADIO_INTERFACE_H__
#define __RADIO_INTERFACE_H__

#include <stdbool.h>
#include <stdint.h>

bool ri_init( void );
void ri_checking_for_discs( void );
void ri_checking_complete( const uint8_t found_map );

#endif
