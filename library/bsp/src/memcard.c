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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <avr32/io.h>

#include "boards/boards.h"
#include "preprocessor/repeat.h"
#include "gpio.h"
#include "memcard.h"
#include "delay.h"
#include "crc.h"
#include "pm.h"
#include "intc.h"
#include "spi.h"
#include "pdca.h"
#include "bsp_errors.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define MC_RX_ISR               PDCA_GET_ISR_NAME( PDCA_CHANNEL_ID_MC_RX )
#define MC_TX_ISR               PDCA_GET_ISR_NAME( PDCA_CHANNEL_ID_MC_TX )

#define MC_GO_IDLE_STATE        0
#define MC_SEND_OP_COND         1
#define MC_SEND_IF_COND         8
#define MC_SEND_CSD             9
#define MC_SEND_CID             10
#define MC_SET_BLOCKLEN         16
#define MC_READ_SINGLE_BLOCK    17
#define MC_APP_CMD              55
#define MC_READ_OCR             58
#define MC_SD_SEND_OP_COND      41

#define MC_R1_IN_IDLE_STATE         (1 << 0)
#define MC_R1_ERASE_RESET           (1 << 1)
#define MC_R1_ILLEGAL_COMMAND       (1 << 2)
#define MC_R1_COM_CRC_ERROR         (1 << 3)
#define MC_R1_ERASE_SEQUENCE_ERROR  (1 << 4)
#define MC_R1_ADDRESS_ERROR         (1 << 5)
#define MC_R1_PARAMETER_ERROR       (1 << 6)

/*
 *     Constant |  Min  |  Max  |
 *  ------------+-------+-------+
 *          Ncs |   0   |   -   |
 *          Ncr |   0   |   8   |
 *          Nac |   1   |   *   |
 *          Nwr |   1   |   -   |
 *          Nec |   0   |   -   |
 *          Nds |   0   |   -   |
 *          Nbr |   0   |   1   |
 *
 *  f = clock frequency
 *
 *  * = [10 * ((TAAC * f) + (100 * NSAC)) / 8] for MMC
 *  * = min[ ((TAAC * f) + (100 *NSAC)) / 8 or (100ms * f) / 8)] for SD
 *  * = [(100ms * f) / 8] for SD20
 */
#define MC_Ncs  1
#define MC_Ncr  10
/* Nac is card dependent. */
#define MC_Nwr  1
#define MC_Nec  1
#define MC_Nds  1

#define MC_BAUDRATE_INITIALIZATION  200000
#define MC_BAUDRATE_MAX             16500000

#define MC_BLOCK_START  0xFE

#define MC_LOCK()                   \
    if( NULL != mc_lock ) {         \
        (*mc_lock)( mc_user_data ); \
    }

#define MC_UNLOCK()                     \
    if( NULL != mc_unlock ) {           \
        (*mc_unlock)( mc_user_data );   \
    }

#define MC_SLEEP( ms )                  \
    if( NULL != mc_sleep ) {            \
        (*mc_sleep)( ms );              \
    } else {                            \
        delay_time( (ms) * 1000000 );   \
    }

#define MC_SUSPEND()                        \
    if( NULL != mc_suspend ) {              \
        (*mc_suspend)();                    \
    } else {                                \
        while( false == transfer_done ) {   \
            MC_SLEEP( 1 ); /* 1 ms */       \
        }                                   \
    }

#define MC_RESUME()             \
{                               \
    if( NULL != mc_resume ) {   \
        (*mc_resume)();         \
    }                           \
}

#define MC_WRITE( data )    spi_write( MC_SPI, data )
#define MC_WRITE_DUMMY()    spi_write( MC_SPI, 0xff )
#define MC_READ( data )     spi_read( MC_SPI, data )
#define MC_READ8( data )    spi_read8( MC_SPI, data )
#define MC_SELECT()         spi_select( MC_SPI, MC_CS )
#define MC_UNSELECT()       spi_unselect( MC_SPI )
#define __MC_CSR( index )   MC_SPI->CSR##index
#define MC_CSR( index )     __MC_CSR( index )

#define MC_APPLY_Ncs() REPEAT( MC_Ncs, __MC_DELAY_CARD, 0 )
#define MC_APPLY_Nwr() REPEAT( MC_Nwr, __MC_DELAY_CARD, 0 )
#define MC_APPLY_Nec() REPEAT( MC_Nec, __MC_DELAY_CARD, 0 )
#define MC_APPLY_Nds() REPEAT( MC_Nds, __MC_DELAY_CARD, 0 )

#define __MC_DELAY_CARD( unused )   MC_WRITE( 0xff );

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

typedef enum { MRT_R1 = 1,
               MRT_R3 = 5,
               MRT_R7 = 5 } mc_response_type_t;

typedef enum { MC_CRC_NONE,
               MC_CRC_7,
               MC_CRC_16 } mc_crc_type_t;

typedef struct {
    unsigned int csd_structure      : 2;
    unsigned int                    : 6;
    unsigned int taac_unit          : 3;
    unsigned int taac_value         : 4;
    unsigned int                    : 1;
    unsigned int nsac               : 8;
    unsigned int                    : 1;
    unsigned int tran_value         : 4;
    unsigned int tran_unit          : 3;

    unsigned int ccc                : 12;
    unsigned int read_bl_len        : 4;
    unsigned int read_bl_partial    : 1;
    unsigned int write_blk_misalign : 1;
    unsigned int read_blk_misalign  : 1;
    unsigned int dsr_imp            : 1;
    unsigned int                    : 12;

    unsigned int                    : 17;
    unsigned int erase_blk_en       : 1;
    unsigned int sector_size        : 7;
    unsigned int wp_grp_size        : 7;

    unsigned int wp_grp_enable      : 1;
    unsigned int                    : 2;
    unsigned int r2w_factor         : 3;
    unsigned int write_bl_len       : 4;
    unsigned int write_bl_partial   : 1;
    unsigned int                    : 5;
    unsigned int file_format_grp    : 1;
    unsigned int copy               : 1;
    unsigned int perm_write_protect : 1;
    unsigned int tmp_write_protect  : 1;
    unsigned int file_format        : 2;
    unsigned int                    : 2;
    unsigned int crc                : 7;
    unsigned int                    : 1;
} mc_csd_common_t;

typedef struct {
    unsigned int                    : 32;

    unsigned int                    : 22;
    unsigned int c_size_u           : 10;   /* Can't go across int boundary */

    unsigned int c_size_l           : 2;
    unsigned int vdd_r_curr_min     : 3;
    unsigned int vdd_r_curr_max     : 3;
    unsigned int vdd_w_curr_min     : 3;
    unsigned int vdd_w_curr_max     : 3;
    unsigned int c_size_mult        : 3;
    unsigned int                    : 15;

    unsigned int                    : 32;
} mc_csd_sd_v1_t;

typedef struct {
    unsigned int                    : 32;

    unsigned int                    : 26;
    unsigned int c_size_u           : 6;   /* Can't go across int boundary */

    unsigned int c_size_l           : 16;
    unsigned int                    : 16;

    unsigned int                    : 32;
} mc_csd_sd_v2_t;

typedef struct {
    unsigned int                    : 9;
    unsigned int taac_value         : 4;
    unsigned int taac_unit          : 3;
    unsigned int nsac               : 8;
    unsigned int                    : 1;
    unsigned int tran_value         : 4;
    unsigned int tran_unit          : 3;
    unsigned int                    : 22;
    unsigned int c_size_u           : 10;   /* Can't go across int boundary */

    unsigned int c_size_l           : 2;
    unsigned int vdd_r_curr_min     : 3;
    unsigned int vdd_r_curr_max     : 3;
    unsigned int vdd_w_curr_min     : 3;
    unsigned int vdd_w_curr_max     : 3;
    unsigned int c_size_mult        : 3;
    unsigned int                    : 15;
    unsigned int                    : 32;
} mc_csd_mmc_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* Callback functions & data */
static lock_fct mc_lock;
static unlock_fct mc_unlock;
static sleep_fct mc_sleep;
static suspend_fct mc_suspend;
static resume_fct mc_resume;
static volatile void *mc_user_data;

static mc_card_type_t mc_type;
static bool mc_mounted;
static uint32_t mc_Nac_read;    /**< In SPI clock cycles. */
static uint32_t mc_Nac_write;   /**< In SPI clock cycles. */
static uint32_t mc_Nac_erase;   /**< In SPI clock cycles. */
static uint32_t mc_blocks;
static uint32_t mc_block_size;  /**< In bytes. */
static uint32_t mc_baud_rate;   /**< In Hz. */

static volatile bool transfer_done; /* Only used by block transfer & ISR. */

#define C( value )  value,
static const uint8_t mc_dummy_data[511] = {
    REPEAT( 510, C, 0xff )
    0xff
};

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static bsp_status_t __select_and_command( const uint8_t command,
                                          const uint32_t argument,
                                          const mc_response_type_t type,
                                          uint8_t *response );
static bsp_status_t __command( const uint8_t command,
                               const uint32_t argument,
                               uint8_t *response );
static bsp_status_t __write_and_read( const uint8_t out, uint8_t *response );
static bsp_status_t __send_card_to_idle_state( void );
static bsp_status_t __is_card_sd_20( void );
static bsp_status_t __is_card_voltage_compat( void );
static bsp_status_t __is_sd_card( void );
static bsp_status_t __complete_init( void );
static bsp_status_t __read_block( const uint8_t command,
                                  const uint32_t argument,
                                  uint8_t *data,
                                  const size_t length,
                                  const mc_crc_type_t type );
static bsp_status_t __wait_for_block( const uint32_t retries );
static bsp_status_t __check_crc( const uint8_t *data,
                                 const size_t length, const mc_crc_type_t crc );
static bsp_status_t __determine_metrics( void );
static bsp_status_t __wait_until_not_busy( void );
static bsp_status_t __set_block_size( void );
__attribute__((__interrupt__))
static void __pdca_block_read_handler( void );
__attribute__((__interrupt__))
static void __card_ejected( void );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See memcard.h for details. */
bsp_status_t mc_init( lock_fct lock,
                      unlock_fct unlock,
                      sleep_fct sleep,
                      suspend_fct suspend,
                      resume_fct resume,
                      volatile void *user_data )
{
    mc_lock      = lock;
    mc_unlock    = unlock;
    mc_sleep     = sleep;
    mc_suspend   = suspend;
    mc_resume    = resume;
    mc_user_data = user_data;

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

    intc_register_isr( &__card_ejected, MC_CD_ISR, ISR_LEVEL__2 );

    intc_register_isr( &__pdca_block_read_handler,
                       MC_RX_ISR, ISR_LEVEL__1 );

    intc_register_isr( &__pdca_block_read_handler,
                       MC_TX_ISR, ISR_LEVEL__1 );

    pdca_channel_init( PDCA_CHANNEL_ID_MC_RX,
                       MC_PDCA_RX_PERIPHERAL_ID, 8 );

    pdca_channel_init( PDCA_CHANNEL_ID_MC_TX,
                       MC_PDCA_TX_PERIPHERAL_ID, 8 );

    return BSP_RETURN_OK;
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
bsp_status_t mc_mount( void )
{
    static const gpio_map_t map[] = { { MC_SCK_PIN,  MC_SCK_FUNCTION  },
                                      { MC_MISO_PIN, MC_MISO_FUNCTION },
                                      { MC_MOSI_PIN, MC_MOSI_FUNCTION },
                                      { MC_CS_PIN,   MC_CS_FUNCTION   } };

    bsp_status_t status;
    int retries;

    retries = 10;

    MC_LOCK();

    if( true == mc_mounted ) {
        status = BSP_MEMCARD_IN_USE;
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

    if( BSP_RETURN_OK !=
            spi_set_baudrate(MC_SPI, MC_CS, MC_BAUDRATE_INITIALIZATION) )
    {
        status = BSP_MEMCARD_INIT_ERROR;
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
    REPEAT( 10, __MC_DELAY_CARD, 0 );

    /* Initialize the memory */
    mc_type = MCT_SDHC;

    /* Initialize the card into idle state. */
    status = __send_card_to_idle_state();
    if( BSP_RETURN_OK != status ) {
        goto done;
    }

    /* See if this card is an SD 2.00 compliant card. */
    status = __is_card_sd_20();
    if( BSP_RETURN_OK != status ) {
        goto done;
    }

    /* See if this card has a compatible voltage range. */
    status = __is_card_voltage_compat();
    if( BSP_RETURN_OK != status ) {
        goto done;
    }

    /* See if the card is an SD card. */
    status = __is_sd_card();
    if( BSP_RETURN_OK != status ) {
        goto done;
    }

    status = __complete_init();
    if( BSP_RETURN_OK != status ) {
        goto done;
    }

    if( MCT_SDHC == mc_type ) {
        uint8_t r3[5];
        status = __select_and_command( MC_READ_OCR, 0, MRT_R3, r3 );
        if( BSP_RETURN_OK != status ) {
            status = BSP_MEMCARD_INIT_ERROR;
            goto done;
        }

        if( 0x40 != (0x40 & r3[1]) ) {
            mc_type = MCT_SD_20;
        }
    }

    mc_Nac_read = MC_Ncr;

    status = __determine_metrics();
    if( BSP_RETURN_OK != status ) {
        goto done;
    }

    status = __wait_until_not_busy();
    if( BSP_RETURN_OK != status ) {
        goto done;
    }

    status = __set_block_size();
    if( BSP_RETURN_OK != status ) {
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

    status = BSP_RETURN_OK;

done:
    /* I don't like retrying, but for now it works. */
    if( (BSP_RETURN_OK != status) && (0 < retries--) ) {
        goto retry;
    }

    MC_UNLOCK();
    return status;
}

/* See memcard.h for details. */
bsp_status_t mc_unmount( void )
{
    MC_LOCK();

    gpio_reset_pin( MC_SCK_PIN );
    gpio_reset_pin( MC_MISO_PIN );
    gpio_reset_pin( MC_MOSI_PIN );
    gpio_reset_pin( MC_CS_PIN );

    MC_UNSELECT();

    if( false == mc_mounted ) {
        MC_UNLOCK();
        return BSP_MEMCARD_NOT_MOUNTED;
    }

    mc_mounted = false;

    MC_UNLOCK();

    return BSP_RETURN_OK;
}

/* See memcard.h for details. */
bsp_status_t mc_read_block( const uint32_t lba, uint8_t *buffer )
{
    uint32_t address = lba;
    bsp_status_t status;

#if (0 < STRICT_PARAMS)
    if( NULL == buffer ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    MC_LOCK();

    if( false == mc_mounted ) {
        MC_UNLOCK();
        return BSP_MEMCARD_NOT_MOUNTED;
    }

    if( mc_type != MCT_SDHC ) {
        address <<= 9;
    }

    status = __wait_until_not_busy();
    if( BSP_ERROR_TIMEOUT == status ) {
        MC_UNLOCK();
        return status;
    }

    status = __read_block( MC_READ_SINGLE_BLOCK, address, buffer, 512, MC_CRC_16 );

    MC_UNLOCK();

    return status;
}

/* See memcard.h for details. */
bsp_status_t mc_write_block( const uint32_t lba, const uint8_t *buffer )
{
    MC_LOCK();

    if( false == mc_mounted ) {
        MC_UNLOCK();
        return BSP_MEMCARD_NOT_MOUNTED;
    }

    MC_UNLOCK();

    return BSP_MEMCARD_NOT_SUPPORTED;
}

/* See memcard.h for details. */
bsp_status_t mc_get_block_count( uint32_t *blocks )
{

#if (0 < STRICT_PARAMS)
    if( NULL == blocks ) {
        return BSP_ERROR_PARAMETER;
    }
#endif

    if( false == mc_mounted ) {
        return BSP_MEMCARD_NOT_MOUNTED;
    }

    *blocks = mc_blocks;

    return BSP_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static bsp_status_t __select_and_command( const uint8_t command,
                                          const uint32_t argument,
                                          const mc_response_type_t type,
                                          uint8_t *response )
{
    bsp_status_t status;
    size_t count;

    MC_SELECT();
    MC_APPLY_Ncs();

    status = __command( command, argument, response );

    if( BSP_RETURN_OK == status ) {
        for( count = 1; count < (size_t) type; count++ ) {
            __write_and_read( 0xFF, &response[count] );
        }
    }

    MC_APPLY_Nec();
    MC_UNSELECT();
    MC_WRITE_DUMMY();

    return status;
}

static bsp_status_t __command( const uint8_t command,
                               const uint32_t argument,
                               uint8_t *response )
{
    volatile avr32_pdca_channel_t *tx;
    uint8_t message[6];
    uint8_t retry;
    uint8_t c;

    message[0] = 0x40 | (0x3f & command);
    message[1] = argument >> 24;
    message[2] = 0xff & (argument >> 16);
    message[3] = 0xff & (argument >> 8);
    message[4] = 0xff & argument;
    message[5] = (crc7(message, 5) << 1) | 0x01;

    transfer_done = false;

    MC_WRITE( message[0] ); MC_READ8( &c );
    MC_WRITE( message[1] ); MC_READ8( &c );
    MC_WRITE( message[2] ); MC_READ8( &c );
    MC_WRITE( message[3] ); MC_READ8( &c );
    MC_WRITE( message[4] ); MC_READ8( &c );
    MC_WRITE( message[5] ); MC_READ8( &c );

    /* Wait for a response - if no response has been
     * received after 8 attempts, the card has timed-out */
    retry = MC_Ncr;
    do {
        uint8_t tmp;
        MC_WRITE_DUMMY();
        MC_READ8( &tmp );

        if( 0xff != tmp ) {
            *response = (uint8_t) tmp;
            return BSP_RETURN_OK;
        }
    }
    while( 0 < retry-- );

    return BSP_ERROR_TIMEOUT;
}

static bsp_status_t __write_and_read( const uint8_t out,
                                      uint8_t *response )
{
    uint16_t read_back;

    MC_WRITE( out );

    if( BSP_ERROR_TIMEOUT == MC_READ(&read_back) ) {
        return BSP_ERROR_TIMEOUT;
    }

    *response = (uint8_t) (0x00FF & read_back);

    return BSP_RETURN_OK;
}


static bsp_status_t __send_card_to_idle_state( void )
{
    int32_t i;
    uint8_t r1;

    r1 = 0;

    for( i = 0; i < 10; i++ ) {
        int32_t retry = 10;

        MC_SELECT();
        __command( MC_GO_IDLE_STATE, 0, &r1 );

        while( (0 < retry--) && (0x01 != r1) ) {
            MC_WRITE_DUMMY();
            MC_READ8( &r1 );
        }

        MC_WRITE_DUMMY();
        MC_APPLY_Nec();
        MC_UNSELECT();
        MC_WRITE_DUMMY();

        if( 0x01 == r1 ) {
            return BSP_RETURN_OK;
        }
   
        /* If we don't delay, the time between the unselect and
         * select is too short to register with some SD cards. */
        MC_SLEEP( 1 ); /* 1 ms */
    }

    return BSP_MEMCARD_INIT_ERROR;
}

static bsp_status_t __is_card_sd_20( void )
{
    bsp_status_t status;
    uint8_t r7[5];

    /* Why 0x0001AA - 2.7-3.6 Volt range, plus recommended pattern
     * from SD 2.00 4.3.13 */
    status = __select_and_command( MC_SEND_IF_COND, 0x0001AA, MRT_R7, r7 );
    MC_WRITE_DUMMY();
    if( BSP_RETURN_OK != status ) {
        /* The card isn't an SD 2.0 card for certain. */
        mc_type = MCT_SD;
    } else {
        switch( r7[0] ) {
            case MC_R1_IN_IDLE_STATE:
                /* This is a 2.0 card or an error. */
                if( (0x01 != r7[3]) || (0xAA != r7[4]) ) {
                    return BSP_MEMCARD_UNUSABLE;
                }
                break;

            case (MC_R1_ILLEGAL_COMMAND | MC_R1_IN_IDLE_STATE):
                /* This is a standard 1.x card. */
                mc_type = MCT_SD;
                break;

            default:
                return BSP_MEMCARD_UNUSABLE;
        }
    }

    return BSP_RETURN_OK;
}

static bsp_status_t __is_card_voltage_compat( void )
{
    return BSP_RETURN_OK;
}

static bsp_status_t __is_sd_card( void )
{
    bsp_status_t status;
    uint32_t cmd41_arg = 0;
    uint8_t r1;

    if( MCT_SDHC == mc_type ) {
        cmd41_arg = 0x40000000;
    }

    status = __select_and_command( MC_APP_CMD, 0, MRT_R1, &r1 );
    MC_WRITE_DUMMY();
    if( BSP_RETURN_OK != status ) {
        return BSP_MEMCARD_UNUSABLE;
    }

    status = __select_and_command( MC_SD_SEND_OP_COND, cmd41_arg, MRT_R1, &r1 );
    MC_WRITE_DUMMY();

    if( (BSP_RETURN_OK == status) && (0 == (0xFE & r1)) ) {
        return BSP_RETURN_OK;
    }

    /* Not an SD card - it must be a MMC card. */
    mc_type = MCT_MMC;
    return __send_card_to_idle_state();
}

static bsp_status_t __complete_init( void )
{
    bsp_status_t status;
    uint32_t cmd1_arg = 0;
    uint8_t r1;
    size_t retries = 50000;

    if( MCT_SDHC == mc_type ) {
        cmd1_arg = 0x40000000;
    }

    do {
        r1 = 0xff;
        status = __select_and_command( MC_SEND_OP_COND, cmd1_arg, MRT_R1, &r1 );
        MC_WRITE_DUMMY();
    } while( (0 < retries--) && (BSP_RETURN_OK == status) && (0 != r1) );

    if( (BSP_RETURN_OK == status) && (0 == r1) ) {
        return BSP_RETURN_OK;
    }

    return status;
}

static bsp_status_t __read_block( const uint8_t command,
                                  const uint32_t argument,
                                  uint8_t *data,
                                  const size_t length,
                                  const mc_crc_type_t crc )
{
    bsp_status_t status;
    uint8_t r1;

    MC_SELECT();
    MC_APPLY_Ncs();

    status = __command( command, argument, &r1 );

    if( (BSP_RETURN_OK == status) && (0 == r1) ) {
        if( BSP_RETURN_OK == __wait_for_block(mc_Nac_read) ) {
            volatile avr32_pdca_channel_t *rx, *tx;
            transfer_done = false;

            pdca_queue_buffer( PDCA_CHANNEL_ID_MC_RX, data, length );
            /* Q: Why (length - 1)?
             * A: Because we do an extra write to start with, the
             * DMA controller does this: [R][W]......[R][W][R]  Since
             * we start and end with reading a byte, the writes must
             * be 1 less.
             */
            pdca_queue_buffer( PDCA_CHANNEL_ID_MC_TX, (void*)mc_dummy_data, (length - 1) );

            MC_WRITE( 0xff );

            rx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_RX];
            tx = &AVR32_PDCA.channel[PDCA_CHANNEL_ID_MC_TX];

            /* Enable the transfer complete ISR */
            rx->ier = AVR32_PDCA_TRC_MASK;

            /* Enable the transfer. */
            rx->cr = AVR32_PDCA_CR_ECLR_MASK | AVR32_PDCA_CR_TEN_MASK;
            tx->cr = AVR32_PDCA_CR_ECLR_MASK | AVR32_PDCA_CR_TEN_MASK;

            rx->isr;
            tx->isr;

            MC_SUSPEND();

            status = __check_crc( data, length, crc );
        }
    } else {
        status = BSP_MEMCARD_ERROR_MODE;
    }

    MC_APPLY_Nec();
    MC_UNSELECT();
    MC_WRITE_DUMMY();

    return status;
}

static bsp_status_t __wait_for_block( const uint32_t retries )
{
    uint8_t c = 0;
    uint32_t retry = retries;

    do {
        MC_WRITE( 0xff );
        MC_READ8( &c );

        if( MC_BLOCK_START == c ) {
            return BSP_RETURN_OK;
        }

    } while( 0 < retry-- );

    return BSP_ERROR_TIMEOUT;
}

static bsp_status_t __check_crc( const uint8_t *data,
                                 const size_t length,
                                 const mc_crc_type_t type )
{
#if ENABLE_CRC
    uint16_t crc;
#endif
    uint8_t tmp;

    switch( type ) {
        case MC_CRC_7:
            MC_WRITE( 0xff );
            MC_READ8( &tmp );
            if( 0x01 != (0x01 & tmp) ) {
                return BSP_MEMCARD_ERROR_MODE;
            }
#if ENABLE_CRC
            tmp >>= 1;
            if( tmp != crc7(data, length) ) {
                return BSP_MEMCARD_CRC_FAILURE;
            }
#endif
            break;

        case MC_CRC_16:
            MC_WRITE( 0xff );
            MC_READ8( &tmp );
#if ENABLE_CRC
            crc = tmp;
#endif
            MC_WRITE( 0xff );
            MC_READ8( &tmp );
#if ENABLE_CRC
            crc = (crc << 8) | tmp;
            if( crc != crc16(data, length) ) {
                return BSP_MEMCARD_CRC_FAILURE;
            }
#endif
            break;

        default:
            return BSP_MEMCARD_ERROR_MODE;
    }

    return BSP_RETURN_OK;
}

static bsp_status_t __determine_metrics( void )
{
    const uint8_t value_map[] = {  0, 10, 12, 13,  15, 20, 25, 30,
                                  35, 40, 45, 50,  55, 60, 70, 80 };
    const uint32_t taac_unit_map[] = { 1000000000,
                                       100000000,
                                       10000000,
                                       1000000,
                                       100000,
                                       10000,
                                       1000,
                                       100 };
    const uint32_t tran_unit_map[] = { 10000,
                                       100000,
                                       1000000,
                                       10000000 };

    uint8_t data[15];
    uint32_t pba_hz;
    bsp_status_t status;
    mc_csd_common_t *com = (mc_csd_common_t *) data;

    pba_hz = pm_get_frequency( PM__PBA );

    status = __read_block( MC_SEND_CSD, 0, data, 15, MC_CRC_7 );
    if( BSP_RETURN_OK != status ) {
        return status;
    }

    if( com->read_bl_len < 9 ) {
        return BSP_MEMCARD_UNUSABLE;
    }
    if( (9 < com->read_bl_len) && (0 == com->read_bl_partial) ) {
        return BSP_MEMCARD_UNUSABLE;
    }

    mc_block_size = 1 << com->read_bl_len;

    switch( com->csd_structure ) {
        case 0: /* SD v1 */
        {
            mc_csd_sd_v1_t *v1 = (mc_csd_sd_v1_t *) com;
            mc_blocks = (v1->c_size_u << 2) | v1->c_size_l;
            mc_blocks++;
            mc_blocks *= (1 << (2 + v1->c_size_mult));
            mc_blocks <<= (com->read_bl_len - 9);

            mc_Nac_read = pba_hz * value_map[com->taac_value];
            mc_Nac_read /= 10;
            mc_Nac_read /= taac_unit_map[com->taac_unit];
            mc_Nac_read += (com->nsac * 100);
            mc_Nac_read /= 8;

            /* Round up. */
            mc_Nac_read++;

            if( (pba_hz / 80) > mc_Nac_read ) {
                mc_Nac_read = pba_hz / 80;
            }

            if( 5 < com->r2w_factor ) {
                return BSP_MEMCARD_UNUSABLE;
            }
            mc_Nac_write = mc_Nac_read * (1 << com->r2w_factor);
            mc_Nac_erase = mc_Nac_write;

            break;
        }

        case 1: /* SD v2 */
        {
            mc_csd_sd_v2_t *v2 = (mc_csd_sd_v2_t *) com;
            mc_blocks = (1 + ((v2->c_size_u << 16) | v2->c_size_l)) * 1024;
            mc_Nac_read = pba_hz / 80;
            mc_Nac_write = pba_hz / 32;
            mc_Nac_erase = pba_hz / 32;
            break;
        }

        case 2: /* MMC v1.2 */
        {
            mc_csd_mmc_t *mmc = (mc_csd_mmc_t *) com;
            mc_blocks = (mmc->c_size_u << 2) | mmc->c_size_l;
            mc_blocks++;
            mc_blocks *= (1 << (2 + mmc->c_size_mult));
            mc_blocks <<= (com->read_bl_len - 9);

            mc_Nac_read = pba_hz * value_map[mmc->taac_value];
            mc_Nac_read /= 10;
            mc_Nac_read /= taac_unit_map[mmc->taac_unit];
            mc_Nac_read += (mmc->nsac * 100);
            mc_Nac_read /= 8;

            /* Round up. */
            mc_Nac_read++;

            if( (pba_hz / 80) > mc_Nac_read ) {
                mc_Nac_read = pba_hz / 80;
            }

            mc_Nac_write = mc_Nac_read * (1 << com->r2w_factor);
            mc_Nac_erase = mc_Nac_write;
            break;
        }

        default:
            return BSP_MEMCARD_UNUSABLE;
    }

    if( 3 < com->tran_unit ) {
        return BSP_MEMCARD_UNUSABLE;
    }
    mc_baud_rate = value_map[com->tran_value] * tran_unit_map[com->tran_unit];

    if( MC_BAUDRATE_MAX < mc_baud_rate ) {
        mc_baud_rate = MC_BAUDRATE_MAX;
    }

    spi_set_baudrate( MC_SPI, MC_CS, mc_baud_rate );

    return BSP_RETURN_OK;
}

static bsp_status_t __wait_until_not_busy( void )
{
    uint8_t tmp;
    size_t retry = 50000;

    MC_SELECT();
    MC_APPLY_Ncs();

    tmp = 0;
    while( (0 < retry--) && (0xff != tmp) ) {
        MC_WRITE( 0xff );
        MC_READ8( &tmp );
    }

    MC_APPLY_Nec();
    MC_UNSELECT();
    MC_WRITE_DUMMY();

    if( 0 == retry ) {
        return BSP_ERROR_TIMEOUT;
    }

    return BSP_RETURN_OK;
}

static bsp_status_t __set_block_size( void )
{
    uint8_t r1;
    bsp_status_t status;

    /* Only set this if we need to - some cards don't like
     * being set when they are already set to 512 byte blocks. */
    if( 512 == mc_block_size ) {
        return BSP_RETURN_OK;
    }

    status = __select_and_command( MC_SET_BLOCKLEN, 512, MRT_R1, &r1 );
    if( BSP_RETURN_OK == status ) {
        if( (0 == r1) || (MC_R1_IN_IDLE_STATE == r1) ) {
            return BSP_RETURN_OK;
        }
    }

    return BSP_MEMCARD_INIT_ERROR;
}

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
