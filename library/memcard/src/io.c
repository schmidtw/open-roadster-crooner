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

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
