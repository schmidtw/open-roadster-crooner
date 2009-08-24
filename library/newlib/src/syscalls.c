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
#include <errno.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/sysregs.h>
#include <sys/reent.h>

#include <bsp/boards/boards.h>
#include <bsp/usart.h>
#include <bsp/pdca.h>
#include <bsp/intc.h>
#include <bsp/pm.h>
#include <bsp/cpu.h>

#define SYSCALL_DEBUG
#define USE_INTERRUPT_DEBUG

static volatile bool __debug_sent;

#ifdef USE_INTERRUPT_DEBUG
__attribute__((__interrupt__))
static void __debug_tx_done( void );
#endif

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
        .hw_handshake = false,
        .map          = debug_usart_map,
        .map_size     = sizeof(debug_usart_map) / sizeof(gpio_map_t),
        .dir          = USART_DIRECTION__OUT,
        .new_char_fn  = NULL,
        .timeout_us   = 0,
        .periodic     = false,
        .cts_fn       = NULL
    };

#ifdef USE_INTERRUPT_DEBUG
    enable_global_interrupts();

    intc_register_isr( &__debug_tx_done,
                       PDCA_GET_ISR_NAME(PDCA_CHANNEL_ID_DEBUG_TX),
                       ISR_LEVEL__2 );

    pdca_channel_init( PDCA_CHANNEL_ID_DEBUG_TX, DEBUG_TX_PDCA_PID, 8 );
#endif

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
int _file_fstat_r( struct _reent *reent, int fd, struct stat *st )
{
    reent->_errno = EBADF;
#ifdef SYSCALL_DEBUG
    fprintf( stderr, "Error: Unimplemented syscall: %s\n", __func__ );
#endif
    return -1;
}

__attribute__((weak))
int _file_write_r( struct _reent *reent, int fd, void *buf, size_t len )
{
    reent->_errno = EBADF;
#ifdef SYSCALL_DEBUG
    fprintf( stderr, "Error: Unimplemented syscall: %s\n", __func__ );
#endif
    return -1;
}

__attribute__((weak))
int _file_read_r( struct _reent *reent, int fd, void *buf, size_t len )
{
    reent->_errno = EBADF;
#ifdef SYSCALL_DEBUG
    fprintf( stderr, "Error: Unimplemented syscall: %s\n", __func__ );
#endif
    return -1;
}

__attribute__((weak))
off_t _file_lseek_r( struct _reent *reent, int fd, off_t offset, int whence )
{
    reent->_errno = EBADF;
#ifdef SYSCALL_DEBUG
    fprintf( stderr, "Error: Unimplemented syscall: %s\n", __func__ );
#endif
    return -1;
}

__attribute__((weak))
int _file_close_r( struct _reent *reent, int fd )
{
    reent->_errno = EBADF;
#ifdef SYSCALL_DEBUG
    fprintf( stderr, "Error: Unimplemented syscall: %s\n", __func__ );
#endif
    return -1;
}

__attribute__((weak))
int _file_isatty_r( struct _reent *reent, int fd )
{
    reent->_errno = EBADF;
#ifdef SYSCALL_DEBUG
    fprintf( stderr, "Error: Unimplemented syscall: %s\n", __func__ );
#endif
    return 0;
}

__attribute__((weak))
int _open_r( struct _reent *reent, const char *name, int flags, int mode )
{
    reent->_errno = EBADF;
#ifdef SYSCALL_DEBUG
    fprintf( stderr, "Error: Unimplemented syscall: %s\n", __func__ );
#endif
    return -1;
}


__attribute__((weak))
void _debug_isr_tx_complete( void )
{
}

__attribute__((weak))
void _debug_block( void )
{
}

__attribute__((weak))
void _debug_lock( void )
{
}

__attribute__((weak))
void _debug_unlock( void )
{
}

void _exit_r( struct _reent *reent, int code )
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

int _fstat_r( struct _reent *reent, int fd, struct stat *st )
{
    if( (0 == fd) || (1 == fd) || (2 == fd) ) {
        st->st_mode = S_IFCHR;
        return 0;
    }

    return _file_fstat_r( reent, fd, st );
}

int _fstat( int fd, struct stat *st )
{
    return _fstat_r( _REENT, fd, st );
}

off_t _lseek_r( struct _reent *reent, int fd, off_t offset, int whence )
{
    if( (0 == fd) || (1 == fd) || (2 == fd) ) {
        reent->_errno = EBADF;
        return -1;
    }

    return _file_lseek_r( reent, fd, offset, whence );
}

off_t _lseek( int fd, off_t offset, int whence )
{
    return _lseek_r( _REENT, fd, offset, whence );
}

int _read_r( struct _reent *reent, int fd, void *buf, size_t len )
{
    if( (0 == fd) || (1 == fd) || (2 == fd) ) {
        reent->_errno = EBADF;
        return -1;
    }

    return _file_read_r( reent, fd, buf, len );
}

int _read( int fd, void *buf, size_t len )
{
    return _read_r( _REENT, fd, buf, len );
}

int _close_r( struct _reent *reent, int fd )
{
    if( (0 == fd) || (1 == fd) || (2 == fd) ) {
        reent->_errno = EBADF;
        return -1;
    }

    return _file_close_r( reent, fd );
}

int _close( int fd )
{
    return _close_r( _REENT, fd );
}

int _isatty_r( struct _reent *reent, int fd )
{
    if( (0 == fd) || (1 == fd) || (2 == fd) ) {
        return 1;
    }

    return _file_isatty_r( reent, fd );
}

int _isatty( int fd )
{
    return _isatty_r( _REENT, fd );
}

/**
 * Low-level write command.
 * When newlib buffer is full or fflush is called, this will output
 * data to correct location.
 * 1 and 2 is stdout and stderr which goes to usart
 */
int _write_r( struct _reent *reent, int fd, void *buf, size_t len )
{
    if( (1 == fd) || (2 == fd) ) {
#ifdef USE_INTERRUPT_DEBUG
        if( (CEM__APPLICATION == cpu_get_mode()) &&
            (true == are_global_interrupts_enabled()) )
        {
            _debug_lock();
            __debug_sent = false;

            pdca_queue_buffer( PDCA_CHANNEL_ID_DEBUG_TX, buf, len );
            pdca_isr_enable( PDCA_CHANNEL_ID_DEBUG_TX, PDCA_ISR__TRANSFER_COMPLETE );
            pdca_enable( PDCA_CHANNEL_ID_DEBUG_TX );

            _debug_block();

            while( false == __debug_sent ) { ; }

            _debug_unlock();
        } else
#endif
        {
            int i;
            for( i = 0; i < len; i++ ) {
                while( false == usart_tx_ready(DEBUG_USART) ) { ; }

                usart_write_char( DEBUG_USART, ((char *) buf)[i] );
            }
        }
        return len;
    }

    if( 0 == fd ) {
        reent->_errno = EBADF;
        return -1;
    }

    return _file_write_r( reent, fd, buf, len );
}

int _write( int fd, void *buf, size_t len )
{
    return _write_r( _REENT, fd, buf, len );
}

int _open( const char *name, int flags, int mode )
{
    return _open_r( _REENT, name, flags, mode );
}

#ifdef USE_INTERRUPT_DEBUG
__attribute__((__interrupt__))
static void __debug_tx_done( void )
{
    pdca_disable( PDCA_CHANNEL_ID_DEBUG_TX );
    pdca_isr_disable( PDCA_CHANNEL_ID_DEBUG_TX, PDCA_ISR__TRANSFER_COMPLETE );
    __debug_sent = true;
    _debug_isr_tx_complete();
}
#endif
