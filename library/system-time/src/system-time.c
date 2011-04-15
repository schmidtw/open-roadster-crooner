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

#include <bsp/cpu.h>
#include <bsp/pm.h>

#include <freertos/task.h>
#include <freertos/semphr.h>

#include <newlib/reent-file-glue.h>

#include "system-time.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define SYS_TIME_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE)

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static xSemaphoreHandle __lock;
static uint64_t __time_in_ns;

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
    portBASE_TYPE status;

    vSemaphoreCreateBinary( __lock );
    __time_in_ns = 0;//1302854109000000000ULL;

    if( NULL == __lock ) {
        goto failure;
    }

    status = xTaskCreate( __sys_time_task, ( signed portCHAR *) "TIME",
                          SYS_TIME_TASK_STACK_SIZE, NULL, priority, NULL );

    if( pdPASS != status ) {
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
    uint32_t now;

    if( (NULL == tv) || (NULL == tz) ) {
        return -1;
    }

    /* Always in GMT timezone */
    tz->tz_minuteswest = 0;
    tz->tz_dsttime = 0;

    xSemaphoreTake( __lock, portMAX_DELAY );
    now = cpu_get_sys_count();
    offset = __time_in_ns;
    xSemaphoreGive( __lock );

    offset += __clock_time_to_ns( now );
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
    uint32_t now;

    while( 1 ) {
        vTaskDelay( 1000 / portTICK_RATE_MS );

        xSemaphoreTake( __lock, portMAX_DELAY );
        now = cpu_get_sys_count();
        cpu_set_sys_count( 0 );
        __time_in_ns += __clock_time_to_ns( now );
        xSemaphoreGive( __lock );
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
