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
#ifndef __MEMCARD_CONSTANTS_H__
#define __MEMCARD_CONSTANTS_H__

#include <stdint.h>

#include "timing-parameters.h"

#define MC_BLOCK_START          0xFE

#define MC_DATA_MASK                    0x1F
#define MC_DATA_ACCEPTED                0x05
#define MC_DATA_REJECTED_CRC            0x0B
#define MC_DATA_REJECTED_WRITE_ERROR    0x0D

#define MC_READ_SINGLE_BLOCK    17
#define MC_WRITE_SINGLE_BLOCK   24
#define MC_COMMAND_BUFFER_SIZE  (MC_Ncs + 6 + MC_Ncr)
#define MC_BLOCK_BUFFER_SIZE    (512 + 10)

extern const uint8_t memory_block_dummy_data[MC_BLOCK_BUFFER_SIZE];

#endif
