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

#include <stddef.h>
#include <stdint.h>

#include <avr32/io.h>

#include "spi.h"
#include "pm.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define SPI_TIMEOUT_CYCLES  10000

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
/* See spi.h for details. */
bsp_status_t spi_reset( volatile avr32_spi_t *spi )
{
#if (0 < STRICT_PARAMS)
    if( NULL == spi ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    /* disable the existing setup to prevent glitches. */
    spi->CR.spidis = 1;

    /* Reset the SPI & clear it's values. */
    spi->CR.swrst = 1;
    spi->mr = AVR32_SPI_MR_PCS_MASK;
    spi->csr0 = 0;
    spi->csr1 = 0;
    spi->csr2 = 0;
    spi->csr3 = 0;
    spi->idr = AVR32_SPI_IDR_TXEMPTY_MASK |
               AVR32_SPI_IDR_NSSR_MASK |
               AVR32_SPI_IDR_TXBUFE_MASK |
               AVR32_SPI_IDR_RXBUFF_MASK |
               AVR32_SPI_IDR_ENDTX_MASK |
               AVR32_SPI_IDR_ENDRX_MASK |
               AVR32_SPI_IDR_OVRES_MASK |
               AVR32_SPI_IDR_MODF_MASK |
               AVR32_SPI_IDR_TDRE_MASK |
               AVR32_SPI_IDR_RDRF_MASK;

    return BSP_RETURN_OK;
}

/* See spi.h for details. */
bsp_status_t spi_set_baudrate(  volatile avr32_spi_t *spi,
                                const uint8_t chip,
                                const uint32_t max_frequency )
{
    uint32_t pba_hz;

#if (0 < STRICT_PARAMS)
    if( (NULL == spi) || (0 == max_frequency) || (3 < chip) ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    pba_hz = pm_get_frequency( PM__PBA );

#if (0 < STRICT_PARAMS)
    if( 0 == pba_hz ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    pba_hz += max_frequency - 1;
    pba_hz /= max_frequency;

    if( pba_hz < 1 ) {
        pba_hz++;
    }

    if( 0xff < pba_hz ) {
        return BSP_ERROR_PARAMETER;
    }

    if( 0 == chip ) {
        spi->CSR0.scbr = pba_hz;
    } else if( 1 == chip ) {
        spi->CSR1.scbr = pba_hz;
    } else if( 2 == chip ) {
        spi->CSR2.scbr = pba_hz;
    } else {
        spi->CSR3.scbr = pba_hz;
    }

    return BSP_RETURN_OK;
}

/* See spi.h for details. */
bsp_status_t spi_get_baudrate( volatile avr32_spi_t *spi,
                               const uint8_t chip,
                               uint32_t *baud_rate )
{
    uint32_t pba_hz;

#if (0 < STRICT_PARAMS)
    if( (NULL == spi) || (NULL == baud_rate) || (3 < chip) ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    pba_hz = pm_get_frequency( PM__PBA );

#if (0 < STRICT_PARAMS)
    if( 0 == pba_hz ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    if( 0 == chip ) {
        *baud_rate = pba_hz / spi->CSR0.scbr;
    } else if( 1 == chip ) {
        *baud_rate = pba_hz / spi->CSR1.scbr;
    } else if( 2 == chip ) {
        *baud_rate = pba_hz / spi->CSR2.scbr;
    } else {
        *baud_rate = pba_hz / spi->CSR3.scbr;
    }

    return BSP_RETURN_OK;
}

/* See spi.h for details. */
bsp_status_t spi_enable( volatile avr32_spi_t *spi )
{
#if (0 < STRICT_PARAMS)
    if( NULL == spi ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    spi->CR.spien = 1;

    return BSP_RETURN_OK;
}

/* See spi.h for details. */
bsp_status_t spi_select( volatile avr32_spi_t *spi, const uint8_t chip )
{
#if (0 < STRICT_PARAMS)
    if( (NULL == spi) || (3 < chip) ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    spi->MR.pcs = chip;

    return BSP_RETURN_OK;
}

/* See spi.h for details. */
bsp_status_t spi_unselect( volatile avr32_spi_t *spi )
{
#if (0 < STRICT_PARAMS)
    if( NULL == spi ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    spi->mr |= AVR32_SPI_MR_PCS_MASK;

    return BSP_RETURN_OK;
}

/* See spi.h for details. */
bsp_status_t spi_write( volatile avr32_spi_t *spi, const uint16_t data )
{
    int32_t retries;

#if (0 < STRICT_PARAMS)
    if( NULL == spi ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    retries = SPI_TIMEOUT_CYCLES;

    while( !(AVR32_SPI_SR_TDRE_MASK & spi->sr) ) {
        retries--;
        if( retries < 0 ) {
            return BSP_ERROR_TIMEOUT;
        }
    }

    spi->tdr = (uint32_t) data;

    return BSP_RETURN_OK;
}

/* See spi.h for details. */
bsp_status_t spi_write_last( volatile avr32_spi_t *spi, const uint16_t data )
{
    int32_t retries;

#if (0 < STRICT_PARAMS)
    if( NULL == spi ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    retries = SPI_TIMEOUT_CYCLES;

    while( !(AVR32_SPI_SR_TDRE_MASK & spi->sr) ) {
        retries--;
        if( retries < 0 ) {
            return BSP_ERROR_TIMEOUT;
        }
    }

    spi->tdr = AVR32_SPI_TDR_LASTXFER_MASK | ((uint32_t) data);

    return BSP_RETURN_OK;
}

/* See spi.h for details. */
bsp_status_t spi_read( volatile avr32_spi_t *spi, uint16_t *data )
{
    int32_t retries;
    uint16_t rdr;

#if (0 < STRICT_PARAMS)
    if( NULL == spi ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    retries = SPI_TIMEOUT_CYCLES;

    while( (AVR32_SPI_SR_RDRF_MASK|AVR32_SPI_SR_TXEMPTY_MASK) != 
           ((AVR32_SPI_SR_RDRF_MASK|AVR32_SPI_SR_TXEMPTY_MASK) & spi->sr) )
    {
        retries--;
        if( retries < 0 ) {
            return BSP_ERROR_TIMEOUT;
        }
    }

    rdr = spi->RDR.rd;

    if( NULL != data ) {
        *data = rdr;
    }

    return BSP_RETURN_OK;
}

/* See spi.h for details. */
bsp_status_t spi_read8( volatile avr32_spi_t *spi, uint8_t *data )
{
    int32_t retries;
    uint16_t rdr;

#if (0 < STRICT_PARAMS)
    if( NULL == spi ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    retries = SPI_TIMEOUT_CYCLES;

    while( (AVR32_SPI_SR_RDRF_MASK|AVR32_SPI_SR_TXEMPTY_MASK) != 
           ((AVR32_SPI_SR_RDRF_MASK|AVR32_SPI_SR_TXEMPTY_MASK) & spi->sr) )
    {
        retries--;
        if( retries < 0 ) {
            return BSP_ERROR_TIMEOUT;
        }
    }

    rdr = spi->RDR.rd;

    if( NULL != data ) {
        *data = (uint8_t) (0x00ff & rdr);
    }

    return BSP_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
