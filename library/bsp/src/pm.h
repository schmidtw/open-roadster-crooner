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
#ifndef __PM_H__
#define __PM_H__

#include <stdbool.h>
#include <stdint.h>

#include "bsp_errors.h"

typedef enum {
    PM__OSC0,
    PM__OSC1,
    PM__PLL0,
    PM__PLL1,
    PM__OSC32,
    PM__SLOW,
    PM__RCOSC
} pm_clocks_t;

typedef enum {
    PM__0_cycles     = 0,
    PM__64_cycles    = 1,
    PM__128_cycles   = 2,
    PM__2048_cycles  = 3,
    PM__4096_cycles  = 4,
    PM__8192_cycles  = 5,
    PM__16384_cycles = 6
} pm_startup_t;

typedef enum {
    PM__CPU,
    PM__PBA,
    PM__PBB,
} pm_bus_t;

/**
 *  Used to enable a physical crystal oscillator.
 *
 *  @param clock the crystal oscillator to enable
 *  @param frequency the frequency of the crystal oscillator in Hz
 *  @param startup the delay needed to start the crystal
 *
 *  @return Status
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_ERROR_PARAMETER Invalid argument(s) passed.
 */
bsp_status_t pm_enable_osc( const pm_clocks_t clock,
                            const uint32_t frequency,
                            const pm_startup_t startup );
/**
 *  Used to enable a PLL oscillator.
 *
 *  @note If div == 0, use this equation:
 *      Fpll = (2 * (mult + 1) * Fosc) / (divide_by_two ? 2 : 1)
 *
 *  @note If div > 0, use this equation:
 *      Fpll = ((mult + 1) / div) * Fosc) / (divide_by_two ? 2 : 1)
 *
 *  @param clock the PLL oscillator to enable
 *  @param osc the crystal oscillator to base the PLL on
 *  @param mult mult in the equations above
 *  @param div div in the equations above
 *  @param divide_by_two div in the equations above
 *  @param count how long to wait for the PLL to lock
 *
 *  @return Status
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_ERROR_PARAMETER Invalid argument(s) passed.
 */
bsp_status_t pm_enable_pll( const pm_clocks_t clock,
                            const pm_clocks_t osc,
                            const uint32_t mult,
                            const uint32_t div,
                            const bool divide_by_two,
                            const uint32_t count );

/**
 *  Used to select the main clock & assign the dividers for all
 *  the buses.
 *
 *  @param main the clock to use [PM__SLOW|PM__OSC0|PM__PLL0]
 *  @param cpu the frequency in Hz for the cpu module
 *  @param pba the frequency in Hz for the pba bus
 *  @param pbb the frequency in Hz for the pbb bus
 *
 *  @return Status
 *      @retval BSP_RETURN_OK       Success.
 *      @retval BSP_ERROR_PARAMETER Invalid argument(s) passed.
 */
bsp_status_t pm_select_clock( const pm_clocks_t main,
                              const uint32_t cpu,
                              const uint32_t pba,
                              const uint32_t pbb );

/**
 *  Used to get the frequency of a bus (or the cpu).
 *
 *  @param bus the bus or cpu
 *
 *  @return the frequency of the clock, or 0 on error.
 */
uint32_t pm_get_frequency( const pm_bus_t bus );

/**
 *  Used to get the frequency of the oscillators or PLLs.
 *
 *  @param clock the clock frequency to return
 *
 *  @return the frequency of the clock, or 0 on error (or inactive).
 */
uint32_t pm_get_clock_frequency( const pm_clocks_t clock );

#endif
