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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <ibus-radio-protocol/ibus-radio-protocol.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <memcard/memcard.h>

#include "radio-interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define RI_TASK_STACK_SIZE  (configMINIMAL_STACK_SIZE + 1400)
#define RI_TASK_PRIORITY    (tskIDLE_PRIORITY+1)
#define RI_POLL_TIMEOUT     (TASK_DELAY_S(15))  /* 15 seconds */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    irp_state_t device_status;
    bool magazine_present;
    uint8_t discs_present;
    uint8_t current_disc;
    uint8_t current_track;
} ri_state_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static xSemaphoreHandle __poll_cmd;
static ri_state_t __state;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void __poll_task( void *params );
static void __msg_task( void *params );
static void __send_state( void );
static void __test( const mc_card_status_t status );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See radio-interface.h for details */
bool ri_init( void )
{
    irp_init();

    vSemaphoreCreateBinary( __poll_cmd );
    xSemaphoreTake( __poll_cmd, 0 );

    __state.device_status = IRP_STATE__STOPPED;
    __state.magazine_present = false;
    __state.discs_present = 0;
    __state.current_disc = 0;
    __state.current_track = 0;

    xTaskCreate( __poll_task, (signed portCHAR *) "POLL",
                 RI_TASK_STACK_SIZE, NULL, RI_TASK_PRIORITY, NULL );
    xTaskCreate( __msg_task, (signed portCHAR *) "iMsg",
                 RI_TASK_STACK_SIZE, NULL, RI_TASK_PRIORITY, NULL );
    return true;
}

void ri_checking_for_discs( void )
{
    irp_going_to_check_disc( 1, 0 );
}

void ri_checking_complete( const uint8_t found_map )
{
    int32_t i;
    int32_t lowest_disc;

    lowest_disc = 7;
    for( i = 1; i <= 6; i++ ) {
        bool disc_present;
        uint8_t active_map;

        disc_present = ((1 << (i - 1)) == (found_map & ((1 << (i - 1)))));
        active_map = (found_map & (0x3f >> (6 - i)));

        printf( "%ld - preset: %s active_map: 0x%02x\n", i,
                ((true == disc_present) ? "true" : "false"),
                active_map );

        if( (true == disc_present) && (i < lowest_disc) ) {
            lowest_disc = i;
        }

        irp_completed_disc_check( i, disc_present, active_map );
        if( i < 6 ) {
            irp_going_to_check_disc( (i+1), active_map );
        }
    }

    __state.discs_present = found_map;
    if( 0x00 == found_map ) {
        __state.device_status = IRP_STATE__STOPPED;
        __state.current_disc = 0;
        __state.current_track = 0;
    } else {
        __state.current_disc = (uint8_t) lowest_disc;
        __state.current_track = 1;
    }

    __send_state();
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void __poll_task( void *params )
{
    irp_send_announce();

    while( 1 ) {
        if( pdTRUE == xSemaphoreTake(__poll_cmd, RI_POLL_TIMEOUT) ) {
            printf( "Poll Request\n" );
            irp_send_poll_response();
        } else {
            printf( "ibus timed out\n" );
            irp_send_announce();
        }
    }
}

static void __msg_task( void *params )
{
    mc_register( &__test );

    while( 1 ) {
        irp_rx_msg_t msg;

        if( IRP_RETURN_OK == irp_get_message(&msg) ) {
            if( IRP_CMD__POLL == msg.command ) {
                xSemaphoreGive( __poll_cmd );
            } else {
                printf( "Got a different command. 0x%04x\n", msg.command );
                __send_state();
            }
        }
    }
}

static void __send_state( void )
{
    irp_send_normal_status( __state.device_status,
                            __state.magazine_present,
                            __state.discs_present,
                            __state.current_disc,
                            __state.current_track );
}

static void __test( const mc_card_status_t status )
{
    switch( status ) {
        case MC_CARD__INSERTED:
            printf( "New card status: MC_CARD__INSERTED\n" );
            break;
        case MC_CARD__MOUNTED:
            __state.magazine_present = true;
            __send_state();
            printf( "New card status: MC_CARD__MOUNTED\n" );
            break;
        case MC_CARD__UNUSABLE:
            printf( "New card status: MC_CARD__UNUSABLE\n" );
            break;
        case MC_CARD__REMOVED:
            __state.magazine_present = false;
            __state.discs_present = 0;
            __state.current_disc = 0;
            __state.current_track = 0;
            __send_state();
            printf( "New card status: MC_CARD__REMOVED\n" );
            break;
    }
}
