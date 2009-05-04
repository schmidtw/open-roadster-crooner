/* Copyright (C) 2006-2008, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdbool.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/sysregs.h>

#include <bsp/boards/boards.h>
#include <bsp/usart.h>
#include <bsp/intc.h>
#include <bsp/pm.h>

#define SYSCALL_DEBUG

static void init_debug_usart( void )
{
    static const gpio_map_t debug_usart_map[] =
    {
        { .pin = DEBUG_RX_PIN, .function = DEBUG_RX_FUNCTION },
        { .pin = DEBUG_TX_PIN, .function = DEBUG_TX_FUNCTION }
    };

    static const usart_options_t debug_usart_options = {
        .baudrate     = DEBUG_BAUD_RATE,
        .data_bits    = USART_DATA_BITS__8,
        .parity       = USART_PARITY__NONE,
        .stop_bits    = USART_STOPBITS__1,
        .mode         = USART_MODE__NORMAL,
        .tx_only      = false,
        .hw_handshake = false,
        .map          = debug_usart_map,
        .map_size     = sizeof(debug_usart_map) / sizeof(gpio_map_t)
    };

    usart_init_rs232( DEBUG_USART, &debug_usart_options );
}

int _init_startup( void )
{
    extern void _evba;
    bsp_status_t osc0, pll, select;

    osc0 = pll = select = BSP_RETURN_OK;

    __builtin_mtsr( AVR32_EVBA, (int) &_evba );

	enable_global_exceptions();

    osc0 = pm_enable_osc( PM__OSC0, FOSC0, OSC0_STARTUP );

#if (1 == CPU_PLL_ENABLED)
    pll = pm_enable_pll( PM__PLL0, PM__OSC0, CPU_PLL_MULTIPLIER, CPU_PLL_DIVIDER,
                   CPU_PLL_DIVIDE_OUTPUT_BY_2, CPU_PLL_STARTUP );
    select = pm_select_clock( PM__PLL0, INITIAL_CPU_SPEED, INITIAL_PBA_SPEED, INITIAL_PBB_SPEED );
#else
    select = pm_select_clock( PM__OSC0, FOSC0, FOSC0, FOSC0 );
#endif

    init_debug_usart();

#ifdef SYSCALL_DEBUG
    printf( "Startup complete.\n" );
#endif
    return 0;
}

__attribute__((weak))
int _file_write( int file, char *ptr, int len )
{
#ifdef SYSCALL_DEBUG
    fprintf( stderr, "Error: Unimplemented syscall: %s\n", __func__ );
#endif
    return -1;
}

__attribute__((weak))
int _file_read( int file, char *ptr, int len )
{
#ifdef SYSCALL_DEBUG
    fprintf( stderr, "Error: Unimplemented syscall: %s\n", __func__ );
#endif
    return 0;
}

void _exit( int code )
{
#ifdef SYSCALL_DEBUG
    /* Signal exit */
    fprintf( stderr, "\004%d", code );
#endif

    /* flush all pending writes */
    fflush( stdout );
    fflush( stderr );

    while( 1 ) { ; }
}

int _read( int file, char *ptr, int len )
{
    if( 0 == file ) {
        int i, c;

        for( i = 0; i < len; i++ ) {
            if( BSP_RETURN_OK == usart_read_char(DEBUG_USART, &c) ) {
                ptr[i] = (char) i;
            } else {
                return i;
            }
        }

        return len;
    }

    if( (1 == file) || (2 == file) ) {
        return -1;
    }

    return _file_read( file, ptr, len );
}

/**
 * Low-level write command.
 * When newlib buffer is full or fflush is called, this will output
 * data to correct location.
 * 1 and 2 is stdout and stderr which goes to usart
 */
int _write( int file, char *ptr, int len )
{
    if( (1 == file) || (2 == file) ) {
        int i;

        for( i = 0; i < len; i++ ) {
            while( false == usart_tx_ready(DEBUG_USART) ) { ; }

            usart_write_char( DEBUG_USART, ptr[i] );
        }

        return len;
    }

    if( 0 == file ) {
        return -1;
    }

    return _file_write( file, ptr, len );
}
