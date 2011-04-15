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

#ifndef __CPU_H__
#define __CPU_H__

typedef enum {
    CEM__APPLICATION            = 0,    /* Lowest priority */
    CEM__SUPERVISOR             = 1,
    CEM__INTERRUPT_LEVEL_0      = 2,
    CEM__INTERRUPT_LEVEL_1      = 3,
    CEM__INTERRUPT_LEVEL_2      = 4,
    CEM__INTERRUPT_LEVEL_3      = 5,
    CEM__EXCEPTION              = 6,
    CEM__NON_MASKABLE_INTERRUPT = 7     /* Highest priority */
} cpu_execution_mode_t;

/**
 *  Used to get the current execution mode of the cpu.
 *
 *  @return the execution mode of the cpu
 */
cpu_execution_mode_t cpu_get_mode( void );

/**
 *  Used to reboot the chip.
 */
void cpu_reboot( void );

/**
 *  Used to disable orphaned devices.
 */
void cpu_disable_orphans( void );

#define cpu_get_sys_count()     __builtin_mfsr( AVR32_COUNT )
#define cpu_set_sys_count(x)    { __builtin_mtsr( AVR32_COUNT, (x) ); }

#define cpu_get_sys_compare()   __builtin_mfsr( AVR32_COMPARE )
#define cpu_set_sys_compare(x)  { __builtin_mtsr( AVR32_COMPARE, (x) ); }

#endif
