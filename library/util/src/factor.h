/*
 *  Copyright (c) 2009  Weston Schmidt
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  In other words, you are welcome to use, share and improve this program.
 *  You are forbidden to forbid anyone else to use, share and improve
 *  what you give them.   Help stamp out software-hoarding!
 */
#ifndef __FACTOR_H__
#define __FACTOR_H__

#include <stdint.h>

/**
 *  Used to factor a number into a power of two & the remainder.
 *
 *  @param in the number to factor
 *  @param power_of_two the largest power of two that can go into the number
 *  @param remainder the remainder of the number
 *  
 *  ex: in = 100100 (binary) 36
 *     power_of_two => 2
 *     remainder    => 1001
 *  ex: in 10 (binary) 2
 *     power_of_two => 1
 *     remainder    => 1
 */
void factor_out_two( const uint32_t in,
                      uint32_t *power_of_two,
                      uint32_t *remainder );
#endif
