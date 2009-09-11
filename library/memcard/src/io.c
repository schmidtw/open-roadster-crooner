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

#include <stdint.h>

#include "config.h"
#include "io.h"
#include "timing-parameters.h"

#include <bsp/boards/boards.h>
#include <bsp/spi.h>
#include <bsp/gpio.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define __MC_CSR( index )   MC_SPI->CSR##index
#define MC_CSR( index )     __MC_CSR( index )

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See io.h for details. */
inline bsp_status_t io_send( const uint8_t data )
{
    return spi_write( MC_SPI, data );
}

/* See io.h for details. */
inline bsp_status_t io_send_dummy( void )
{
    return io_send( 0xff );
}

/* See io.h for details. */
inline bsp_status_t io_read( uint8_t *data )
{
    return spi_read8( MC_SPI, data );
}

/* See io.h for details. */
inline void io_select( void )
{
    spi_select( MC_SPI, MC_CS );
}

/* See io.h for details. */
inline void io_clean_select( void )
{
#if   (0 == MC_Ncs)
#elif (1 == MC_Ncs)
    io_send_dummy();
#else
    int i;

    for( i = 0; i < MC_Ncs; i++ ) {
        io_send_dummy();
    }
#endif
    io_select();
}

/* See io.h for details. */
inline void io_unselect( void )
{
    spi_unselect( MC_SPI );
}

/* See io.h for details. */
inline void io_clean_unselect( void )
{
#if   (0 == MC_Nec)
#elif (1 == MC_Nec)
    io_send_dummy();
#else
    int i;

    for( i = 0; i < MC_Nec; i++ ) {
        io_send_dummy();
    }
#endif

    io_unselect();
    io_send_dummy();
}

/* See io.h for details. */
inline bsp_status_t io_send_read( const uint8_t out, uint8_t *in )
{
    uint8_t tmp;

    if( BSP_RETURN_OK != io_send(out) ) {
        return BSP_ERROR_TIMEOUT;
    }

    if( BSP_RETURN_OK != io_read(&tmp) ) {
        return BSP_ERROR_TIMEOUT;
    }

    *in = tmp;

    return BSP_RETURN_OK;
}

/* See io.h for details. */
void io_disable( void )
{
    gpio_set_options( MC_SCK_PIN, GPIO_DIRECTION__INPUT, GPIO_PULL_UP__ENABLE,
                      GPIO_GLITCH_FILTER__DISABLE, GPIO_INTERRUPT__NONE, 1 );

    gpio_set_options( MC_MISO_PIN, GPIO_DIRECTION__INPUT, GPIO_PULL_UP__ENABLE,
                      GPIO_GLITCH_FILTER__DISABLE, GPIO_INTERRUPT__NONE, 1 );

    gpio_set_options( MC_MOSI_PIN, GPIO_DIRECTION__INPUT, GPIO_PULL_UP__ENABLE,
                      GPIO_GLITCH_FILTER__DISABLE, GPIO_INTERRUPT__NONE, 1 );

    gpio_set_options( MC_CS_PIN, GPIO_DIRECTION__INPUT, GPIO_PULL_UP__ENABLE,
                      GPIO_GLITCH_FILTER__DISABLE, GPIO_INTERRUPT__NONE, 1 );
}

/* See io.h for details. */
bsp_status_t io_enable( void )
{
    bsp_status_t status;

    static const gpio_map_t map[] = { { MC_SCK_PIN,     MC_SCK_FUNCTION     },
                                      { MC_MISO_PIN,    MC_MISO_FUNCTION    },
                                      { MC_MOSI_PIN,    MC_MOSI_FUNCTION    },
                                      { MC_CS_PIN,      MC_CS_FUNCTION      },
                                      { MC_FAKE_CS_PIN, MC_FAKE_CS_FUNCTION } };

    /* Initialize the hardware. */
    gpio_enable_module( map, sizeof(map)/sizeof(gpio_map_t) );

    spi_reset( MC_SPI );

    MC_SPI->MR.mstr = 1;    /* master mode */
    MC_SPI->MR.modfdis = 1; /* ignore faults */
    MC_SPI->MR.dlybcs = 8;  /* make sure there is a delay between CSs */

    status = spi_set_baudrate( MC_SPI, MC_CS, MC_BAUDRATE_INITIALIZATION );
    if( BSP_RETURN_OK != status ) {
        return status;
    }

    status = spi_set_baudrate( MC_SPI, MC_FAKE_CS, MC_BAUDRATE_INITIALIZATION );
    if( BSP_RETURN_OK != status ) {
        return status;
    }

    /* Allow 8 clock cycles of up time for the chip select
     * prior to enabling/disabling it */
    (MC_CSR(MC_CS)).dlybs  = 8;

    /* We need a small delay between bytes, otherwise we seem to
     * get data corruption unless we are going really slow. */
    (MC_CSR(MC_CS)).dlybct = 1;

    /* scbr is set by spi_set_baudrate() */

    (MC_CSR(MC_CS)).bits   = 0;
    (MC_CSR(MC_CS)).csaat  = 1;
    (MC_CSR(MC_CS)).csnaat = 0;
    (MC_CSR(MC_CS)).ncpha  = 1;
    (MC_CSR(MC_CS)).cpol   = 0;

    /* Do the same thing for the fake interface. */
    (MC_CSR(MC_FAKE_CS)).dlybs  = 8;
    (MC_CSR(MC_FAKE_CS)).dlybct = 1;
    (MC_CSR(MC_FAKE_CS)).bits   = 0;
    (MC_CSR(MC_FAKE_CS)).csaat  = 1;
    (MC_CSR(MC_FAKE_CS)).csnaat = 0;
    (MC_CSR(MC_FAKE_CS)).ncpha  = 1;
    (MC_CSR(MC_FAKE_CS)).cpol   = 0;

    spi_enable( MC_SPI );

    return BSP_RETURN_OK;
}

/* See io.h for details. */
void io_wakeup_card( void )
{
    int i;

    /* Send 74+ clock cycles. */
    spi_select( MC_SPI, MC_FAKE_CS );
    for( i = 0; i < 10; i++ ) {
        io_send_dummy();
    }
    io_unselect();
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
