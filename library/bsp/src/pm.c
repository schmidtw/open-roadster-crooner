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

#include <stdbool.h>
#include <stdint.h>

#include <avr32/io.h>

#include "pm.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static uint32_t __clock_frequency[5];
static const int8_t __pm_clock_map[] = { 0, 1, 2, 3, 4, -1, -1 };

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
bsp_status_t __pm_enable_osc( const pm_clocks_t clock,
                              const uint32_t frequency,
                              const pm_startup_t startup );
bsp_status_t __pm_enable_pll( const pm_clocks_t clock,
                              const uint32_t frequency,
                              const pm_startup_t startup );
uint32_t __pm_get_pll_freq( const pm_clocks_t clock,
                            const uint32_t mult,
                            const uint32_t div,
                            const bool divide_by_two );
uint8_t __pm_get_cksel_value( const uint32_t divider );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See pm.h for details. */
bsp_status_t pm_enable_osc( const pm_clocks_t clock,
                            const uint32_t frequency,
                            const pm_startup_t startup )
{
    int mode;

    switch( startup ) {
        case PM__0_cycles:
        case PM__64_cycles:
        case PM__128_cycles:
        case PM__2048_cycles:
        case PM__4096_cycles:
        case PM__8192_cycles:
        case PM__16384_cycles:  break;
        default:                return BSP_ERROR_PARAMETER;
    }

    if( (PM__OSC0 != clock) && (PM__OSC0 != clock) ) {
        return BSP_ERROR_PARAMETER;
    }

    if( (frequency < 400000) || (16000000 < frequency) ) {
        return BSP_ERROR_PARAMETER;
    }

    if(      frequency < 900000  ) { mode = AVR32_PM_OSCCTRL0_MODE_CRYSTAL_G0; }
    else if( frequency < 3000000 ) { mode = AVR32_PM_OSCCTRL0_MODE_CRYSTAL_G1; }
    else if( frequency < 8000000 ) { mode = AVR32_PM_OSCCTRL0_MODE_CRYSTAL_G2; }
    else                           { mode = AVR32_PM_OSCCTRL0_MODE_CRYSTAL_G3; }

    if( 0 == PM__OSC0 ) {
        AVR32_PM.OSCCTRL0.mode = mode;
        AVR32_PM.OSCCTRL0.startup = (uint8_t) startup;
        AVR32_PM.MCCTRL.osc0en = 1;

        while( !(AVR32_PM_POSCSR_OSC0RDY_MASK & AVR32_PM.poscsr) ) { ; }
    } else {
        AVR32_PM.OSCCTRL1.mode = mode;
        AVR32_PM.OSCCTRL1.startup = (uint8_t) startup;
        AVR32_PM.MCCTRL.osc1en = 1;

        while( !(AVR32_PM_POSCSR_OSC1RDY_MASK & AVR32_PM.poscsr) ) { ; }
    }

    __clock_frequency[__pm_clock_map[clock]] = frequency;

    return BSP_RETURN_OK;
}

/* See pm.h for details. */
bsp_status_t pm_enable_pll( const pm_clocks_t clock,
                            const pm_clocks_t osc,
                            const uint32_t mult,
                            const uint32_t div,
                            const bool divide_by_two,
                            const uint32_t count )
{
    int range;
    uint32_t frequency;
    volatile avr32_pm_pll_t *pll;

    if( (PM__PLL0 != clock) && (PM__PLL1 != clock) ) {
        return BSP_ERROR_PARAMETER;
    }

    if( ((PM__OSC0 != osc) && (PM__OSC1 != osc)) ) {
        return BSP_ERROR_PARAMETER;
    }

    frequency = __pm_get_pll_freq( osc, mult, div, divide_by_two );

    if( (AVR32_PM_PLL_VCO_RANGE0_MIN_FREQ < frequency) &&
        (frequency < AVR32_PM_PLL_VCO_RANGE0_MAX_FREQ) )
    {
        range = 0;
    } else if( (AVR32_PM_PLL_VCO_RANGE1_MIN_FREQ < frequency) &&
               (frequency < AVR32_PM_PLL_VCO_RANGE1_MAX_FREQ) )
    {
        range = 1;
    } else {
        return BSP_ERROR_PARAMETER;
    }

    pll = &AVR32_PM.PLL[((PM__PLL0 == clock) ? 0 : 1)];

    /* Disable to prevent glitches. */
    pll->pllen = 0;

    pll->pllmul = mult;
    pll->plldiv = div;
    pll->pllosc = (PM__OSC0 == osc) ? 0 : 1;
    pll->pllcount = count;
    pll->pllopt = (divide_by_two << 1) | range;

    /* Re-enable */
    pll->pllen = 1;

    /* Start it up and wait for the lock. */
    if( PM__PLL0 == clock ) {
        while( !(AVR32_PM_POSCSR_LOCK0_MASK & AVR32_PM.poscsr) ) { ; }

        AVR32_PM.pll[0] |= AVR32_PM_PLL0_PLLBPL_MASK;
    } else {
        while( !(AVR32_PM_POSCSR_LOCK1_MASK & AVR32_PM.poscsr) ) { ; }

        AVR32_PM.pll[1] |= AVR32_PM_PLL1_PLLBPL_MASK;
    }

    __clock_frequency[__pm_clock_map[clock]] = frequency;

    return BSP_RETURN_OK;
}

/* See pm.h for details. */
bsp_status_t pm_select_clock( const pm_clocks_t main,
                              const uint32_t cpu,
                              const uint32_t pba,
                              const uint32_t pbb )
{
    int clock, wait_state;
    uint32_t cksel, cpu_factor, pba_factor, pbb_factor, osc_freq;
    uint8_t cpu_cksel, pbb_cksel, pba_cksel;

    switch( main ) {
        case PM__OSC0: clock = AVR32_PM_MCCTRL_MCSEL_OSC0; break;
        case PM__PLL0: clock = AVR32_PM_MCCTRL_MCSEL_PLL0; break;
        case PM__SLOW: clock = AVR32_PM_MCCTRL_MCSEL_SLOW; break;

        default:
            return BSP_ERROR_PARAMETER;
    }

    osc_freq = __clock_frequency[__pm_clock_map[main]];

    if( (0 == osc_freq) ||
        (AVR32_PM_CPU_MAX_FREQ < cpu) ||
        (AVR32_PM_HSB_MAX_FREQ < cpu) ||    /* These are the same */
        (AVR32_PM_PBA_MAX_FREQ < pba) ||
        (AVR32_PM_PBB_MAX_FREQ < pbb) )
    {
        return BSP_ERROR_PARAMETER;
    }

    cpu_factor = osc_freq / cpu;
    pba_factor = osc_freq / pba;
    pbb_factor = osc_freq / pbb;

    if( (osc_freq != (cpu * cpu_factor)) ||
        (osc_freq != (pba * pba_factor)) ||
        (osc_freq != (pbb * pbb_factor)) )
    {
        return BSP_ERROR_PARAMETER;
    }

    if( cpu <= AVR32_FLASHC_FWS_0_MAX_FREQ ) {
        wait_state = 0;
    } else {
        wait_state = 1;
    }

    cpu_cksel = __pm_get_cksel_value( cpu_factor );
    pba_cksel = __pm_get_cksel_value( pba_factor );
    pbb_cksel = __pm_get_cksel_value( pbb_factor );

    if( (0xff == cpu_cksel) ||
        (0xff == pba_cksel) ||
        (0xff == pbb_cksel) )
    {
        return BSP_ERROR_PARAMETER;
    }

    cksel = (pbb_cksel << 24) |
            (pba_cksel << 16) |
            (cpu_cksel <<  8) |
            (cpu_cksel);

    /* Make sure it is safe to write to mcsel. */
    while( !(AVR32_PM_POSCSR_OSC0RDY_MASK & AVR32_PM.poscsr) ) { ; }

    /* Go to a known safe mode */
    AVR32_PM.MCCTRL.mcsel = AVR32_PM_MCCTRL_MCSEL_SLOW;

    /* Make sure the flash wait state is properly set. */
    AVR32_FLASHC.FCR.fws = wait_state;

    /* Set the clock dividers */
    AVR32_PM.cksel = cksel;

    /* Make sure it is safe to write to mcsel. */
    while( !(AVR32_PM_POSCSR_OSC0RDY_MASK & AVR32_PM.poscsr) ) { ; }

    /* Set the clock to the new speed. */
    AVR32_PM.MCCTRL.mcsel = clock;

    return BSP_RETURN_OK;
}

/* See pm.h for details. */
uint32_t pm_get_frequency( const pm_bus_t bus )
{
    uint32_t base, divider;
    int clock;

    clock = AVR32_PM.MCCTRL.mcsel;
    if( AVR32_PM_MCCTRL_MCSEL_OSC0 == clock ) {
        base = __clock_frequency[__pm_clock_map[PM__OSC0]];
    } else if( AVR32_PM_MCCTRL_MCSEL_PLL0 == clock ) {
        base = __clock_frequency[__pm_clock_map[PM__PLL0]];
    } else {
        base = __clock_frequency[__pm_clock_map[PM__SLOW]];
    }

    divider = 0;
    switch( bus ) {
        case PM__CPU:
            if( 1 == AVR32_PM.CKSEL.cpudiv ) {
                divider = AVR32_PM.CKSEL.cpusel;
                divider++;
            }
            break;

        case PM__PBA:
            if( 1 == AVR32_PM.CKSEL.pbadiv ) {
                divider = AVR32_PM.CKSEL.pbasel;
                divider++;
            }
            break;

        case PM__PBB:
            if( 1 == AVR32_PM.CKSEL.pbbdiv ) {
                divider = AVR32_PM.CKSEL.pbbsel;
                divider++;
            }
            break;

        default:
            return 0;
    }
    divider = 1 << divider;

    return (base / divider);
}

/* See pm.h for details. */
uint32_t pm_get_clock_frequency( const pm_clocks_t clock )
{
    if( 0 <= __pm_clock_map[(uint32_t) clock] ) {
        return __clock_frequency[__pm_clock_map[(uint32_t) clock]];
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  Used to take the PLL parameters and calculate the PLL frequency.
 *
 *  @param clock the clock the PLL will use as its source
 *  @param mult the mult variable in the calculation
 *  @param div the div variable in the calculation
 *  @param divide_by_two if the Fvco should be divided by 2 to make Fpll
 *
 *  @return PLL frequency in Hz on success, 0 otherwise
 */
uint32_t __pm_get_pll_freq( const pm_clocks_t clock,
                            const uint32_t mult,
                            const uint32_t div,
                            const bool divide_by_two )
{
    uint32_t Fpll;

    if( __pm_clock_map[clock] < 0 ) {
        return 0;
    }

    if( 0 == div ) {
        Fpll = (2 * (mult + 1)) * __clock_frequency[__pm_clock_map[clock]];
    } else {
        Fpll = ((mult + 1) * __clock_frequency[__pm_clock_map[clock]]) / div;
    }

    if( true == divide_by_two ) {
        Fpll /= 2;
    }

    return Fpll;
}

/**
 *  Helper function that takes the native cksel value & convert it
 *  to a divisor.
 *
 *  @param divider the cksel format divider
 *
 *  @return the divider in the normal format
 */
uint8_t __pm_get_cksel_value( const uint32_t divider )
{
    switch( divider ) {
        case 1:     return 0x00;
        case 2:     return 0x80;
        case 4:     return 0x81;
        case 8:     return 0x82;
        case 16:    return 0x83;
        case 32:    return 0x84;
        case 64:    return 0x85;
        case 128:   return 0x86;
        case 256:   return 0x87;
    }

    return 0xff;    /* Error. */
}
