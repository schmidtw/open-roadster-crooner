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

#ifndef __TIMING_PARAMETERS_H__
#define __TIMING_PARAMETERS_H__

/*
 *     Constant |  Min  |  Max  |
 *  ------------+-------+-------+
 *          Ncs |   0   |   -   |
 *          Ncr |   0   |   8   |
 *          Nac |   1   |   *   |
 *          Nwr |   1   |   -   |
 *          Nec |   0   |   -   |
 *          Nds |   0   |   -   |
 *          Nbr |   0   |   1   |
 *
 *  f = clock frequency
 *
 *  * = [10 * ((TAAC * f) + (100 * NSAC)) / 8] for MMC
 *  * = min[ ((TAAC * f) + (100 *NSAC)) / 8 or (100ms * f) / 8)] for SD
 *  * = [(100ms * f) / 8] for SD20
 */
#define MC_Ncs  1
#define MC_Ncr  10
/* Nac is card dependent. */
#define MC_Nwr  1
#define MC_Nec  1
#define MC_Nds  1

#endif
