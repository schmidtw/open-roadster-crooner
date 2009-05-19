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

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <avr32/io.h>

#include <bsp/boards/boards.h>
#include <bsp/gpio.h>
#include <bsp/pm.h>
#include <bsp/intc.h>
#include <bsp/spi.h>
#include <bsp/pdca.h>

#include <freertos/queue.h>
#include <freertos/task.h>

#include "config.h"
#include "memcard.h"
#include "crc.h"
#include "commands.h"
#include "command.h"
#include "timing-parameters.h"
#include "fast-fill.h"
#include "io.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define MC_RX_ISR       PDCA_GET_ISR_NAME( PDCA_CHANNEL_ID_MC_RX )
#define MC_TX_ISR       PDCA_GET_ISR_NAME( PDCA_CHANNEL_ID_MC_TX )

#define MC_BLOCK_START  0xFE

#define MIN( a, b )     ((a) < (b)) ? (a) : (b)
#define MC_LOCK()
#define MC_UNLOCK()
#define MC_SLEEP( ms )
#define MC_SUSPEND()
#define MC_RESUME()

#define __MC_CSR( index )   MC_SPI->CSR##index
#define MC_CSR( index )     __MC_CSR( index )

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef enum { MCT_UNKNOWN = 0x00,
               MCT_MMC     = 0x10,
               MCT_SD      = 0x20,
               MCT_SD_20   = 0x21,
               MCT_SDHC    = 0x22 } mc_card_type_t;

typedef enum { MVL_27 = 0x00008000,
               MVL_28 = 0x00018000,
               MVL_29 = 0x00030000,
               MVL_30 = 0x00060000,
               MVL_31 = 0x000C0000,
               MVL_32 = 0x00180000,
               MVL_33 = 0x00300000,
               MVL_34 = 0x00600000,
               MVL_35 = 0x00C00000,
               MVL_36 = 0x00800000 } mc_voltage_levels_t;

typedef enum { MC_CRC_NONE,
               MC_CRC_7,
               MC_CRC_16 } mc_crc_type_t;

typedef struct {
    uint8_t *buffer;
} mc_message_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static mc_card_type_t mc_type;
static bool mc_mounted;
static uint32_t mc_Nac_read;    /**< In SPI clock cycles. */
static uint32_t mc_Nac_write;   /**< In SPI clock cycles. */
static uint32_t mc_Nac_erase;   /**< In SPI clock cycles. */
static uint32_t mc_blocks;
static uint32_t mc_block_size;  /**< In bytes. */
static uint32_t mc_baud_rate;   /**< In Hz. */

static volatile bool transfer_done; /* Only used by block transfer & ISR. */

static xQueueHandle __idle;
static xQueueHandle __pending;
static xQueueHandle __complete;
static mc_message_t __messages[MC_MSG_MAX];
static uint8_t *__fast_buffer;
static volatile uint32_t __last_Nac_wait_time;
static volatile uint32_t __last_bytes_sent;
static volatile uint32_t __total_bytes_sent;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static mc_status_t __send_card_to_idle_state( void );
static mc_status_t __is_card_voltage_compat( void );
static mc_status_t __is_sd_card( void );
static mc_status_t __complete_init( void );
static mc_status_t __determine_metrics( void );
static mc_status_t __wait_until_not_busy( void );
static mc_status_t __set_block_size( void );
__attribute__((__interrupt__))
static void __card_ejected( void );
__attribute__((__interrupt__))
static void __fast_read_handler( void );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See memcard.h for details. */
mc_status_t mc_init( void* (*fast_malloc_fn)(size_t) )
{
    mc_mounted = false;
    mc_type = MCT_UNKNOWN;

    /* Create an interrupt on the card ejection. */
    gpio_set_options( MC_CD_PIN,
                      GPIO_DIRECTION__INPUT,
                      GPIO_PULL_UP__DISABLE,
                      GPIO_GLITCH_FILTER__ENABLE,
#if (0 != MC_CD_ACTIVE_LOW)
                      GPIO_INTERRUPT__RISING_EDGE,
#else
                      GPIO_INTERRUPT__FALLING_EDGE,
#endif
                      0 );

    __idle = xQueueCreate( MC_MSG_MAX, sizeof(mc_message_t*) );
    __pending = xQueueCreate( MC_MSG_MAX, sizeof(mc_message_t*) );
    __complete = xQueueCreate( MC_MSG_MAX, sizeof(mc_message_t*) );

    __fast_buffer = (uint8_t *) (*fast_malloc_fn)( MC_FAST_BUFFER_SIZE );

    intc_register_isr( &__card_ejected, MC_CD_ISR, ISR_LEVEL__2 );
    //intc_register_isr( &__pdca_block_read_handler, MC_RX_ISR, ISR_LEVEL__1 );
    //intc_register_isr( &__pdca_block_read_handler, MC_TX_ISR, ISR_LEVEL__1 );

    pdca_channel_init( PDCA_CHANNEL_ID_MC_RX, MC_PDCA_RX_PERIPHERAL_ID, 8 );
    pdca_channel_init( PDCA_CHANNEL_ID_MC_TX, MC_PDCA_TX_PERIPHERAL_ID, 8 );

    return MC_RETURN_OK;
}

/* See memcard.h for details. */
bool mc_present( void )
{
#if (0 != MC_CD_ACTIVE_LOW)
    return (0 == gpio_read_pin(MC_CD_PIN));
#else
    return (0 != gpio_read_pin(MC_CD_PIN));
#endif
}

/* See memcard.h for details. */
mc_status_t mc_mount( void )
{
    static const gpio_map_t map[] = { { MC_SCK_PIN,  MC_SCK_FUNCTION  },
                                      { MC_MISO_PIN, MC_MISO_FUNCTION },
                                      { MC_MOSI_PIN, MC_MOSI_FUNCTION },
                                      { MC_CS_PIN,   MC_CS_FUNCTION   } };

    mc_status_t status;
    int retries;
    int i;
    bool legal_command;

    retries = 10;

    MC_LOCK();

    if( true == mc_mounted ) {
        status = MC_IN_USE;
        goto done;
    }

retry:

    /* Make sure the power is stable. */
    MC_SLEEP( 250 ); /* 250 ms */

    /* Initialize the hardware. */
    gpio_enable_module( map, sizeof(map)/sizeof(gpio_map_t) );

    spi_reset( MC_SPI );

    MC_SPI->MR.mstr = 1;      /* master mode */
    MC_SPI->MR.modfdis = 1;   /* ignore faults */

    if( MC_RETURN_OK !=
            spi_set_baudrate(MC_SPI, MC_CS, MC_BAUDRATE_INITIALIZATION) )
    {
        status = MC_INIT_ERROR;
        goto done;
    }

    /* Allow 8 clock cycles of up time for the chip select
     * prior to enabling/disabling it */
    (MC_CSR(MC_CS)).dlybct = 8;
    (MC_CSR(MC_CS)).dlybs  = 8;

    /* scbr is set by spi_set_baudrate() */

    (MC_CSR(MC_CS)).bits   = 0;
    (MC_CSR(MC_CS)).csaat  = 1;
    (MC_CSR(MC_CS)).csnaat = 0;
    (MC_CSR(MC_CS)).ncpha  = 1;
    (MC_CSR(MC_CS)).cpol   = 0;

    spi_enable( MC_SPI );

    /* Send 74+ clock cycles. */
    for( i = 0; i < 10; i++ ) {
        io_send_dummy();
    }

    /* Initialize the card into idle state. */
    status = __send_card_to_idle_state();
    if( MC_RETURN_OK != status ) {
        goto done;
    }

    /* See if this card is an SD 2.00 compliant card. */
    status = mc_send_if_cond( 3300, &legal_command );
    if( MC_RETURN_OK != status ) {
        goto done;
    }

    if( false == legal_command ) {
        mc_type = MCT_SD;
    }

    /* See if this card has a compatible voltage range. */
    status = __is_card_voltage_compat();
    if( MC_RETURN_OK != status ) {
        goto done;
    }

    /* See if the card is an SD card. */
    status = __is_sd_card();
    if( MC_RETURN_OK != status ) {
        goto done;
    }

    status = __complete_init();
    if( MC_RETURN_OK != status ) {
        goto done;
    }

    if( MCT_SDHC == mc_type ) {
        mc_ocr_t ocr;

        status = mc_read_ocr( &ocr );
        if( MC_RETURN_OK != status ) {
            status = MC_INIT_ERROR;
            goto done;
        }

        if( false == ocr.card_capacity_status ) {
            mc_type = MCT_SD_20;
        }
    }

    mc_Nac_read = MC_Ncr;

    status = __determine_metrics();
    if( MC_RETURN_OK != status ) {
        goto done;
    }

    status = __wait_until_not_busy();
    if( MC_RETURN_OK != status ) {
        goto done;
    }

    status = __set_block_size();
    if( MC_RETURN_OK != status ) {
        goto done;
    }

    mc_mounted = true;

    printf( "Mounted Card type: " );
    switch( mc_type ) {
        case MCT_UNKNOWN:   printf( "Unknown" ); break;
        case MCT_MMC:       printf( "MMC" ); break;
        case MCT_SD:        printf( "SD" ); break;
        case MCT_SD_20:     printf( "SD 2.0" ); break;
        case MCT_SDHC:      printf( "SDHC" ); break;
    }

    printf( " Size: %llu mc_Nac_read: %lu\n", (512ULL * (uint64_t)mc_blocks), mc_Nac_read );

    status = MC_RETURN_OK;

done:
    /* I don't like retrying, but for now it works. */
    if( (MC_RETURN_OK != status) && (0 < retries--) ) {
        goto retry;
    }

    MC_UNLOCK();
    return status;
}

/* See memcard.h for details. */
mc_status_t mc_unmount( void )
{
    MC_LOCK();

    gpio_reset_pin( MC_SCK_PIN );
    gpio_reset_pin( MC_MISO_PIN );
    gpio_reset_pin( MC_MOSI_PIN );
    gpio_reset_pin( MC_CS_PIN );

    io_unselect();

    if( false == mc_mounted ) {
        MC_UNLOCK();
        return MC_NOT_MOUNTED;
    }

    mc_mounted = false;

    MC_UNLOCK();

    return MC_RETURN_OK;
}

/* See memcard.h for details. */
mc_status_t mc_read_block( const uint32_t lba, uint8_t *buffer )
{
    uint32_t address = lba;
    mc_status_t status;

#if (0 < STRICT_PARAMS)
    if( NULL == buffer ) {
        return MC_ERROR_PARAMETER;
    }
#endif

    MC_LOCK();

    if( false == mc_mounted ) {
        MC_UNLOCK();
        return MC_NOT_MOUNTED;
    }

    if( mc_type != MCT_SDHC ) {
        address <<= 9;
    }

    status = __wait_until_not_busy();
    if( MC_ERROR_TIMEOUT == status ) {
        MC_UNLOCK();
        return status;
    }

    /* Do something */

    MC_UNLOCK();

    return status;
}

/* See memcard.h for details. */
mc_status_t mc_write_block( const uint32_t lba, const uint8_t *buffer )
{
    MC_LOCK();

    if( false == mc_mounted ) {
        MC_UNLOCK();
        return MC_NOT_MOUNTED;
    }

    MC_UNLOCK();

    return MC_NOT_SUPPORTED;
}

/* See memcard.h for details. */
mc_status_t mc_get_block_count( uint32_t *blocks )
{

#if (0 < STRICT_PARAMS)
    if( NULL == blocks ) {
        return MC_ERROR_PARAMETER;
    }
#endif

    if( false == mc_mounted ) {
        return MC_NOT_MOUNTED;
    }

    *blocks = mc_blocks;

    return MC_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static mc_status_t __send_card_to_idle_state( void )
{
    int32_t i;

    for( i = 0; i < 10; i++ ) {
        if( MC_RETURN_OK == mc_go_idle_state() ) {
            return MC_RETURN_OK;
        }
    }

    return MC_INIT_ERROR;
}

static mc_status_t __is_card_voltage_compat( void )
{
    return MC_RETURN_OK;
}

static mc_status_t __is_sd_card( void )
{
    bool ready;

    do {
        mc_status_t status;

        status = mc_send_sd_op_cond( (MCT_SDHC == mc_type), &ready );
        if( MC_ERROR_TIMEOUT == status ) {
            /* Not an SD card - it must be a MMC card. */
            mc_type = MCT_MMC;
            return __send_card_to_idle_state();
        }
    } while( false == ready );

    return MC_RETURN_OK;
}

static mc_status_t __complete_init( void )
{
    int i;

    for( i = 0; i < 50000; i++ ) {
        mc_status_t status;
        bool ready;

        status = mc_send_op_cond( (MCT_SDHC == mc_type), &ready );
        if( MC_ERROR_TIMEOUT == status ) {
            return MC_ERROR_TIMEOUT;
        }

        if( true == ready ) {
            return MC_RETURN_OK;
        }
    }

    return MC_ERROR_TIMEOUT;
}

static mc_status_t __fast_read_single_block( const uint32_t lba )
{
#define MC_READ_SINGLE_BLOCK 17

    uint8_t *buffer;
    uint32_t __last_bytes_sent;
    volatile avr32_pdca_channel_t *rx, *tx;

    buffer = __fast_buffer;

    fast_fill( buffer );

    __total_bytes_sent = 0;
    __last_bytes_sent = MC_Ncs; /* Skip this number of bytes for this timing parameter. */

    buffer[__last_bytes_sent++] = 0x40 | (0x3f & MC_READ_SINGLE_BLOCK);
    buffer[__last_bytes_sent++] = lba >> 24;
    buffer[__last_bytes_sent++] = 0xff & (lba >> 16);
    buffer[__last_bytes_sent++] = 0xff & (lba >> 8);
    buffer[__last_bytes_sent++] = 0xff & lba;
    buffer[__last_bytes_sent++] = (crc7(&buffer[MC_Ncs], 5) << 1) | 0x01;

    __last_bytes_sent += MC_Ncr;                /* Includes R1 response */
    __last_bytes_sent += __last_Nac_wait_time;  /* Delay until we get the payload */
    __last_bytes_sent += 10;                    /* Give us some wiggle room from the Nac */
    __last_bytes_sent += 512;                   /* payload */

    /* Don't overrun the buffer */
    __last_bytes_sent = MIN( __last_bytes_sent, MC_FAST_BUFFER_SIZE );

    intc_register_isr( &__fast_read_handler, MC_RX_ISR, ISR_LEVEL__1 );

    pdca_queue_buffer( PDCA_CHANNEL_ID_MC_RX, buffer, __last_bytes_sent );
    /* Q: Why (length - 1)?
     * A: Because we do an extra write to start with, the
     * DMA controller does this: [R][W]......[R][W][R]  Since
     * we start and end with reading a byte, the writes must
     * be 1 less.
     */
    pdca_queue_buffer( PDCA_CHANNEL_ID_MC_TX, &buffer[1], (__last_bytes_sent - 1) );

    io_select();
    io_send_dummy();

    rx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_RX];
    tx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_TX];

    /* Enable the transfer complete ISR */
    rx->ier = AVR32_PDCA_TRC_MASK;

    /* Enable the transfer. */
    rx->cr = AVR32_PDCA_CR_ECLR_MASK | AVR32_PDCA_CR_TEN_MASK;
    tx->cr = AVR32_PDCA_CR_ECLR_MASK | AVR32_PDCA_CR_TEN_MASK;

    rx->isr;
    tx->isr;

    return MC_RETURN_OK;
}

static mc_status_t __fast_process_block( void )
{
    uint8_t *buffer, *end;
    portBASE_TYPE yield;
    mc_message_t *msg;

    /* 6 is the size of the command buffer sent */
    buffer = &__fast_buffer[MC_Ncs + 6];
    end = &__fast_buffer[__last_bytes_sent];

    buffer = memchr( buffer, 0, MC_Ncr );
    if( NULL == buffer ) {
        io_clean_unselect();

        return MC_ERROR_TIMEOUT;
    }
    buffer++;

    buffer = memchr( buffer, MC_BLOCK_START, (end - buffer) );
    if( NULL == buffer ) {
        /* For now, bail. */
        io_clean_unselect();

        return MC_ERROR_TIMEOUT;
#if 0
        /* We didn't get the starting data byte yet. */
        __total_bytes_sent += (end - buffer);
        fast_fill( __fast_buffer );
        __last_bytes_sent = MC_FAST_BUFFER_SIZE;

        pdca_queue_buffer( PDCA_CHANNEL_ID_MC_RX, buffer, __last_bytes_sent );
        pdca_queue_buffer( PDCA_CHANNEL_ID_MC_TX, &buffer[1], (__last_bytes_sent - 1) );

        rx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_RX];
        tx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_TX];

        /* Enable the transfer complete ISR */
        rx->ier = AVR32_PDCA_TRC_MASK;

        /* Enable the transfer. */
        rx->cr = AVR32_PDCA_CR_ECLR_MASK | AVR32_PDCA_CR_TEN_MASK;
        tx->cr = AVR32_PDCA_CR_ECLR_MASK | AVR32_PDCA_CR_TEN_MASK;

        rx->isr;
        tx->isr;
        return MC_STILL_WAITING;
#endif
    }

    if( (end - buffer) < 514 ) {    /* 512 + CRC */
        /* For now, bail. */
        io_clean_unselect();

        return MC_ERROR_TIMEOUT;
    }
    buffer++;

    io_clean_unselect();

    if( pdTRUE != xQueueReceiveFromISR(__pending, &msg, &yield) ) {
        return MC_ERROR_MODE;
    }

    memcpy( msg->buffer, buffer, 512 );

    xQueueSendToBackFromISR( __complete, &msg, &yield );

    return MC_RETURN_OK;
}

static mc_status_t __determine_metrics( void )
{
    mc_csd_t csd;
    uint32_t pba_hz;
    mc_status_t status;

    pba_hz = pm_get_frequency( PM__PBA );

    status = mc_get_csd( &csd, pba_hz );
    if( MC_RETURN_OK != status ) {
        return status;
    }

    mc_block_size = csd.block_size;
    if( 512 != mc_block_size ) {
        return MC_UNUSABLE;
    }

    mc_blocks = csd.total_blocks;
    mc_Nac_read = csd.nac_read;
    mc_Nac_write = csd.nac_write;
    mc_Nac_erase = csd.nac_erase;

    if( (0 == mc_blocks) || (0 == mc_Nac_read) ||
        (0 == mc_Nac_write) || (0 == mc_Nac_erase) )
    {
        return MC_UNUSABLE;
    }

    /* Convert the clock cycles into bytes & make
     * sure the timeout is at least 1 */
    mc_Nac_read  = mc_Nac_read / 8 + 1;
    mc_Nac_write = mc_Nac_write / 8 + 1;
    mc_Nac_erase = mc_Nac_erase / 8 + 1;

    mc_baud_rate = csd.max_speed;
    if( 0 == mc_baud_rate ) {
        return MC_UNUSABLE;
    }

    if( MC_BAUDRATE_MAX < mc_baud_rate ) {
        mc_baud_rate = MC_BAUDRATE_MAX;
    }

    spi_set_baudrate( MC_SPI, MC_CS, mc_baud_rate );

    return MC_RETURN_OK;
}

static mc_status_t __wait_until_not_busy( void )
{
    uint8_t tmp;
    size_t retry = 50000;

    io_clean_select();

    tmp = 0;
    while( (0 < retry--) && (0xff != tmp) ) {
        io_send_read( 0xff, &tmp );
    }

    io_clean_unselect();

    if( 0 == retry ) {
        return MC_ERROR_TIMEOUT;
    }

    return MC_RETURN_OK;
}

static mc_status_t __set_block_size( void )
{
    mc_status_t status;

    /* Only set this if we need to - some cards don't like
     * being set when they are already set to 512 byte blocks. */
    if( 512 == mc_block_size ) {
        return MC_RETURN_OK;
    }

    status = mc_set_block_size( 512 );
    if( MC_RETURN_OK == status ) {
        return MC_RETURN_OK;
    }

    return MC_INIT_ERROR;
}

#if 0
__attribute__((__interrupt__))
static void __pdca_block_read_handler( void )
{
    volatile avr32_pdca_channel_t *tx, *rx;

    tx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_TX];
    rx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_RX];

    /* Disable 'transfer complete' */
    tx->idr = AVR32_PDCA_TRC_MASK;
    rx->idr = AVR32_PDCA_TRC_MASK;

    /* Clear the ISR values */
    tx->isr;
    rx->isr;

    /* Disable 'tx' pdca channel */
    tx->cr = AVR32_PDCA_CR_TDIS_MASK;

    /* Disable 'rx' pdca channel */
    rx->cr = AVR32_PDCA_CR_TDIS_MASK;

    transfer_done = true;
    MC_RESUME();
}
#endif

__attribute__((__interrupt__))
static void __card_ejected( void )
{
    /* Force clean up now so we're ready for a new card insertion. */
    mc_mounted = false;
    mc_type = MCT_UNKNOWN;
    gpio_reset_pin( MC_SCK_PIN );
    gpio_reset_pin( MC_MISO_PIN );
    gpio_reset_pin( MC_MOSI_PIN );
    gpio_reset_pin( MC_CS_PIN );

    gpio_clr_interrupt_flag( MC_CD_PIN );
}

__attribute__((__interrupt__))
static void __fast_read_handler( void )
{
    volatile avr32_pdca_channel_t *tx, *rx;

    tx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_TX];
    rx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_RX];

    /* Disable 'transfer complete' */
    tx->idr = AVR32_PDCA_TRC_MASK;
    rx->idr = AVR32_PDCA_TRC_MASK;

    /* Clear the ISR values */
    tx->isr;
    rx->isr;

    /* Disable 'tx' pdca channel */
    tx->cr = AVR32_PDCA_CR_TDIS_MASK;

    /* Disable 'rx' pdca channel */
    rx->cr = AVR32_PDCA_CR_TDIS_MASK;

    transfer_done = true;
    MC_RESUME();
}
