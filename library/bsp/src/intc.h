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
#ifndef __INTC_H__
#define __INTC_H__

#include <stdbool.h>
#include <stdint.h>

#include <avr32/io.h>

#include "bsp_errors.h"

typedef enum {
    ISR__SYSBLOCK_COMPARE       = 0x0000,
    ISR__EXTERNAL_INTERRUPT_0   = 0x0100,
    ISR__EXTERNAL_INTERRUPT_1   = 0x0101,
    ISR__EXTERNAL_INTERRUPT_2   = 0x0102,
    ISR__EXTERNAL_INTERRUPT_3   = 0x0103,
    ISR__EXTERNAL_INTERRUPT_4   = 0x0104,
    ISR__EXTERNAL_INTERRUPT_5   = 0x0105,
    ISR__EXTERNAL_INTERRUPT_6   = 0x0106,
    ISR__EXTERNAL_INTERRUPT_7   = 0x0107,
    ISR__REAL_TIME_CLOCK        = 0x0108,
    ISR__POWER_MANAGER          = 0x0109,
    ISR__FREQUENCY_METER        = 0x010a,
    ISR__GPIO_0                 = 0x0200,
    ISR__GPIO_1                 = 0x0201,
    ISR__GPIO_2                 = 0x0202,
    ISR__GPIO_3                 = 0x0203,
    ISR__GPIO_4                 = 0x0204,
    ISR__GPIO_5                 = 0x0205,
    ISR__GPIO_6                 = 0x0206,
    ISR__GPIO_7                 = 0x0207,
    ISR__GPIO_8                 = 0x0208,
    ISR__GPIO_9                 = 0x0209,
    ISR__GPIO_10                = 0x020a,
    ISR__GPIO_11                = 0x020b,
    ISR__GPIO_12                = 0x020c,
    ISR__GPIO_13                = 0x020d,
    ISR__PDCA_0                 = 0x0300,
    ISR__PDCA_1                 = 0x0301,
    ISR__PDCA_2                 = 0x0302,
    ISR__PDCA_3                 = 0x0303,
    ISR__PDCA_4                 = 0x0304,
    ISR__PDCA_5                 = 0x0305,
    ISR__PDCA_6                 = 0x0306,
    ISR__PDCA_7                 = 0x0307,
    ISR__PDCA_8                 = 0x0308,
    ISR__PDCA_9                 = 0x0309,
    ISR__PDCA_10                = 0x030a,
    ISR__PDCA_11                = 0x030b,
    ISR__PDCA_12                = 0x030c,
    ISR__PDCA_13                = 0x030d,
    ISR__PDCA_14                = 0x030e,
    ISR__FLASH_CONTROLLER       = 0x0400,
    ISR__USART_0                = 0x0500,
    ISR__USART_1                = 0x0600,
    ISR__USART_2                = 0x0700,
    ISR__USART_3                = 0x0800,
    ISR__SPI_0                  = 0x0900,
    ISR__SPI_1                  = 0x0a00,
    ISR__TWI                    = 0x0b00,
    ISR__PWM                    = 0x0c00,
    ISR__SSC                    = 0x0d00,
    ISR__TC_0                   = 0x0e00,
    ISR__TC_1                   = 0x0e01,
    ISR__TC_2                   = 0x0e02,
    ISR__ADC                    = 0x0f00,
    ISR__ETHERNET_MAC           = 0x1000,
    ISR__USB                    = 0x1100,
    ISR__SDRAM                  = 0x1200,
    ISR__DAC                    = 0x1300
} intc_isr_t;

typedef enum {
    ISR_LEVEL__0    = 0,    /** Lowest priority */
    ISR_LEVEL__1    = 1,
    ISR_LEVEL__2    = 2,
    ISR_LEVEL__3    = 3     /** Highest priority */
} intc_level_t;

typedef void (*intc_handler_t)( void );

/**
 *  Used to disable global interrupts.
 */
void disable_global_interrupts( void );

/**
 *  Used to enable global interrupts.
 */
void enable_global_interrupts( void );

/**
 *  Used to determine if global interrupts are enabled.
 *
 *  @return true of enabled, false otherwise
 */
bool are_global_interrupts_enabled( void );

/**
 *  Used to set the global interrupts to off & return the
 *  previous state.
 *
 *  @return true if the interrupts were previously enabled,
 *          false othewise
 */
bool interrupts_save_and_disable( void );

/**
 *  Used to set the global interrupts to the previous state.
 *
 *  @param state the previous state of the interrupts to restore
 */
void interrupts_restore( const bool state );

/**
 *  Used to disable global exceptions.
 */
void disable_global_exceptions( void );

/**
 *  Used to enable global exceptions.
 */
void enable_global_exceptions( void );

/**
 *  Used to determine if global exceptions are enabled.
 *
 *  @return true of enabled, false otherwise
 */
bool are_global_exceptions_enabled( void );

/**
 *  Used to register an interrupt handler.
 *
 *  @note Lower priority interrupts may be interrupted by higher
 *        priority interrupts.
 *
 *  @param handler the function to call (must be of
 *                 __attribute__((__interrupt__)) type)
 *  @param group the group of the interrupt to map the handler to
 *  @param line the line of the interrupt to map the handler to
 *  @param level the interrupt level to map to
 *
 *  @return Status
 *      @retval BSP_RETURN_OK
 *      @retval BSP_ERROR_PARAMETER
 */
bsp_status_t intc_register_isr( const intc_handler_t handler,
                                const intc_isr_t isr,
                                const intc_level_t level );

/**
 *  Used to output a string while in an ISR.
 *
 *  @note DO NOT use any of the printf routines for output in
 *        an ISR because the reentrancy is not correct.
 *
 *  @param out the string to output
 */
void intc_isr_puts( const char *out );

#endif
