/*
 * Copyright (c) 2011  Weston Schmidt
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

#ifndef __DEBUG_HELPERS_H__
#define __DEBUG_HELPERS_H__

#include "commands.h"

/**
 *  Used to print (via printf) the cid structure.
 *
 *  @param cid the structure to print
 */
void print_cid( mc_cid_t *cid );

/**
 *  Used to print (via printf) the csd structure.
 *
 *  @param csd the structure to print
 */
void print_csd( mc_csd_t *csd );

/**
 *  Used to print (via printf) the card status structure.
 *
 *  @param status the structure to print
 */
void print_card_status( mc_cs_t *status );
#endif
