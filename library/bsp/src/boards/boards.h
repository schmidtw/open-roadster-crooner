/*
 * Copyright (c) 2008  Weston Schmidt
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
#ifndef __BOARDS_H__
#define __BOARDS_H__

#define EVK1100         1
#define CROONER_1_0     2
#define CROONER_2_0     3

#if BOARD == EVK1100
#   include "evk1100.h"
#elif BOARD == CROONER_1_0
#   include "crooner-1.0.h"
#elif BOARD == CROONER_2_0
#   include "crooner-2.0.h"
#else
#   error No known board defined.
#endif

#endif
