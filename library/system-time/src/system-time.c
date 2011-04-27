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
#include <stdio.h>

#include <stdint.h>
#include <sys/reent.h>
#include <sys/time.h>

#include <bsp/boards/boards.h>
#include <bsp/cpu.h>
#include <bsp/pm.h>

#include <freertos/os.h>

#include <newlib/reent-file-glue.h>

#include "system-time.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define SYS_TIME_TASK_STACK_SIZE 50

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static semaphore_handle_t __lock;
static uint64_t __time_in_ns;
static uint32_t __last_cpu_time;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __sys_time_task( void *params );
static uint64_t __clock_time_to_ns( const uint32_t clock_time );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
systime_status_t system_time_init( const uint32_t priority )
{
    bool status;

     __lock = os_semaphore_create_binary();
    __time_in_ns = 0;
    __last_cpu_time = 0;

    if( NULL == __lock ) {
        goto failure;
    }

    status = os_task_create( __sys_time_task, "SysTime", SYS_TIME_TASK_STACK_SIZE,
                             NULL, priority, NULL );

    if( true != status ) {
        goto failure;
    }

    return SYSTIME_RETURN_OK;

failure:
    return SYSTIME_RESOURCE_ERROR;
}

/*----------------------------------------------------------------------------*/
/*                               Newlib <=> Glue                              */
/*----------------------------------------------------------------------------*/
int _gettimeofday( struct timeval *tv, struct timezone *tz )
{
    uint64_t offset;
    uint32_t now, delta;

    if( (NULL == tv) || (NULL == tz) ) {
        return -1;
    }

    /* Always in GMT timezone */
    tz->tz_minuteswest = 0;
    tz->tz_dsttime = 0;

    os_semaphore_take( __lock, WAIT_FOREVER );
    now = cpu_get_sys_count();
    if( __last_cpu_time < now ) {
        delta = now - __last_cpu_time;
    } else {
        delta = 0xffffffffu - __last_cpu_time + now + 1;
    }
    offset = __time_in_ns;
    os_semaphore_give( __lock );

    offset += __clock_time_to_ns( delta );
    offset /= 1000;
    tv->tv_sec = (uint32_t) (offset / 1000000ULL);
    tv->tv_usec = (uint32_t) (offset % 1000000ULL);

    return 0;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/**
 *  The system time keeper.
 */
static void __sys_time_task( void *params )
{
    uint32_t now, delta;

    while( 1 ) {
        os_task_delay_ms( 1000 );

        os_semaphore_take( __lock, WAIT_FOREVER );
        now = cpu_get_sys_count();
        if( __last_cpu_time < now ) {
            delta = now - __last_cpu_time;
        } else {
            delta = 0xffffffffu - __last_cpu_time + now + 1;
        }
        __last_cpu_time = now;
        __time_in_ns += __clock_time_to_ns( delta );
        os_semaphore_give( __lock );
    }
}

/**
 *  Used to convert the clock time to ns.
 *
 *  @param clock_time the time to convert to ns
 *
 *  @return the time in ns
 */
static uint64_t __clock_time_to_ns( const uint32_t clock_time )
{
    uint64_t clock;
    uint64_t tmp;

    tmp = clock_time;
    tmp *= 1000000000ULL;

    clock = pm_get_frequency( PM__CPU );

    tmp /= clock;

    return tmp;
}
