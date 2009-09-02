/*
 * Copyright (c) 2009  Weston Schmidt
 *
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
#ifndef __MEMCARD_PRIVATE_H__
#define __MEMCARD_PRIVATE_H__

#include <stdint.h>

typedef enum {
    MCT_UNKNOWN = 0x00,
    MCT_MMC     = 0x10,
    MCT_SD      = 0x20,
    MCT_SD_20   = 0x21,
    MCT_SDHC    = 0x22
} mc_card_type_t;

/**
 *  Used to get the current memory card type.
 *
 *  @return the current memory card type
 */
mc_card_type_t mc_get_type( void );

/**
 *  Used to get the current Nac read value.
 *
 *  @return the current Nac read value
 */
uint32_t mc_get_Nac_read( void );

/**
 *  Used to get the current magic insert number.
 *  This number increments each time a card is inserted
 *  or removed from the slot.  It will roll over.
 *
 *  @return the current magic insert number.
 */
uint32_t mc_get_magic_insert_number( void );
#endif
