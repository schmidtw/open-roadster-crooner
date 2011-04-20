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
#include <string.h>
#include <avr32/io.h>

#include <bsp/boards/boards.h>
#include <bsp/gpio.h>
#include <bsp/pm.h>
#include <bsp/intc.h>
#include <bsp/spi.h>

#include <freertos/os.h>

#include "config.h"
#include "block.h"
#include "memcard.h"
#include "memcard-private.h"
#include "memcard-constants.h"
#include "commands.h"
#include "command.h"
#include "timing-parameters.h"
#include "io.h"
#include "glue.h"
#include "fatfs/ff.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define MEMCARD_DEBUG 0

#define AUTOMOUNT_STACK_SIZE    (200)
#define AUTOMOUNT_PRIORITY      (1)
#define AUTOMOUNT_CALLBACK_MAX  3

#define _D1(...)
#define _D2(...)

#ifdef MEMCARD_DEBUG
#if (0 < MEMCARD_DEBUG)
#undef  _D1
#define _D1(...) printf( __VA_ARGS__ )
#endif
#if (1 < MEMCARD_DEBUG)
#undef  _D2
#define _D2(...) printf( __VA_ARGS__ )
#endif
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef enum {
    MCS_NO_CARD,
    MCS_CARD_DETECTED,
    MCS_POWERED_ON,
    MCS_ASSERT_SPI_MODE,
    MCS_INTERFACE_CONDITION_CHECK,
    MCS_INTERFACE_CONDITION_SUCCESS,
    MCS_INTERFACE_CONDITION_FAILURE,
    MCS_SD10_COMPATIBLE_VOLTAGE,
    MCS_SD20_COMPATIBLE_VOLTAGE,
    MCS_SD10_CARD_READY,
    MCS_SD20_CARD_READY,
    MCS_MMC_CARD,
    MCS_DETERMINE_METRICS,
    MCS_SET_BLOCK_SIZE,
    MCS_CARD_READY,
    MCS_CARD_UNUSABLE
} mc_mount_state_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static mc_card_type_t __mc_type;
static uint64_t __mc_size;
static uint32_t __mc_Nac_read;
static uint32_t __mc_block_size;  /**< In bytes. */
static volatile uint32_t __magic_insert_number;

static semaphore_handle_t __card_state_change;
static volatile mc_card_status_t __card_status;
static card_status_fct __callback_fns[AUTOMOUNT_CALLBACK_MAX];

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static mc_status_t __send_card_to_idle_state( void );
static mc_status_t __is_card_voltage_compat( void );
static mc_status_t __determine_metrics( void );
static mc_status_t __set_block_size( void );
__attribute__((__interrupt__))
static void __card_change( void );
static void __automount_task( void *data );
static mc_status_t __mc_mount( void );
static void __call_all( const mc_card_status_t status );
static bool __isr_card_present( void );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/* See memcard.h for details. */
mc_status_t mc_init( void* (*fast_malloc_fn)(size_t) )
{
    int32_t i;

    glue_init();

    __card_status = MC_CARD__REMOVED;
    __card_state_change = os_semaphore_create_binary();
    os_semaphore_take( __card_state_change, 0 );

    for( i = 0; i < AUTOMOUNT_CALLBACK_MAX; i++ ) {
        __callback_fns[i] = NULL;
    }

    block_init( fast_malloc_fn );

    io_disable();

    os_task_create( __automount_task, "Auto Mnt",
                    AUTOMOUNT_STACK_SIZE, NULL, AUTOMOUNT_PRIORITY, NULL );

    return MC_RETURN_OK;
}

/* See memcard.h for details. */
mc_status_t mc_register( card_status_fct card_status_fn )
{
    int32_t i;

    if( NULL == card_status_fn ) {
        return MC_ERROR_PARAMETER;
    }

    for( i = 0; i < AUTOMOUNT_CALLBACK_MAX; i++ ) {
        if( NULL == __callback_fns[i] ) {
            __callback_fns[i] = card_status_fn;
            return MC_RETURN_OK;
        }
    }

    return MC_TOO_MANY_REGISTERED;
}

mc_status_t mc_cancel( card_status_fct card_status_fn )
{
    if( NULL != card_status_fn ) {
        int32_t i;

        for( i = 0; i < AUTOMOUNT_CALLBACK_MAX; i++ ) {
            if( card_status_fn == __callback_fns[i] ) {
                __callback_fns[i] = NULL;
                return MC_RETURN_OK;
            }
        }
    }

    return MC_ERROR_PARAMETER;
}

/* See memcard.h for details. */
mc_card_status_t mc_get_status( void )
{
    return __card_status;
}

/* See memcard.h for details. */
mc_status_t mc_get_block_count( uint32_t *blocks )
{
    if( NULL == blocks ) {
        return MC_ERROR_PARAMETER;
    }

    if( (MC_CARD__MOUNTED != __card_status) &&
        (MC_CARD__MOUNTING != __card_status) )
    {
        return MC_NOT_MOUNTED;
    }

    *blocks = (uint32_t) (__mc_size / 512ULL);

    return MC_RETURN_OK;
}

/*----------------------------------------------------------------------------*/
/*                           Library Only Functions                           */
/*----------------------------------------------------------------------------*/
/* See memcard-private.h for details. */
mc_card_type_t mc_get_type( void )
{
    return __mc_type;
}

/* See memcard-private.h for details. */
uint32_t mc_get_Nac_read( void )
{
    return __mc_Nac_read;
}

/* See memcard-private.h for details. */
uint32_t mc_get_magic_insert_number( void )
{
    return __magic_insert_number;
}

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
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

static mc_status_t __determine_metrics( void )
{
    mc_csd_t csd;
    uint32_t pba_hz;
    mc_status_t status;
    uint32_t baud_rate;

    pba_hz = pm_get_frequency( PM__PBA );

    status = mc_get_csd( &csd, pba_hz );
    if( MC_RETURN_OK != status ) {
        return status;
    }

    if( (0 == csd.max_speed) ||
        (0 == csd.total_size) ||
        (0 == csd.block_size) )
    {
        return MC_UNUSABLE;
    }

    baud_rate = csd.max_speed;
    __mc_block_size = csd.block_size;
    __mc_size = csd.total_size;
    __mc_Nac_read = csd.nac_read;

    spi_set_baudrate( MC_SPI, MC_CS, baud_rate );
    spi_get_baudrate( MC_SPI, MC_CS, &baud_rate );
    _D2( "Actual baud rate: %lu\n", baud_rate );

    return MC_RETURN_OK;
}


static mc_status_t __set_block_size( void )
{
    mc_status_t status;

    /* Only set this if we need to - some cards don't like
     * being set when they are already set to 512 byte blocks. */
    if( 512 == __mc_block_size ) {
        return MC_RETURN_OK;
    }

    status = mc_set_block_size( 512 );
    if( MC_RETURN_OK == status ) {
        return MC_RETURN_OK;
    }

    return MC_INIT_ERROR;
}

__attribute__((__interrupt__))
static void __card_change( void )
{
    __magic_insert_number++;

    if( false == __isr_card_present() ) {
        /* Force clean up now so we're ready for a new card insertion. */
        __mc_type = MCT_UNKNOWN;
        io_disable();
        io_unselect();
    
        block_isr_cancel();
    }

    os_semaphore_give_ISR( __card_state_change, NULL );

    gpio_clr_interrupt_flag( MC_CD_PIN );
}

static void __automount_task( void *data )
{
    FATFS fs;
    
    /* Create an interrupt on the card ejection. */
    gpio_set_options( MC_CD_PIN,
                      GPIO_DIRECTION__INPUT,
                      GPIO_PULL_UP__DISABLE,
                      GPIO_GLITCH_FILTER__ENABLE,
                      GPIO_INTERRUPT__CHANGE,
                      0 );

    intc_register_isr( &__card_change, MC_CD_ISR, ISR_LEVEL__2 );

    /* In case we booted with a card in the slot. */
    if( true == __isr_card_present() ) {
        os_semaphore_give( __card_state_change );
    }

    while( 1 ) {
        os_semaphore_take( __card_state_change, WAIT_FOREVER );

        if( true == __isr_card_present() ) {
            __card_status = MC_CARD__INSERTED;
            __call_all( __card_status );
        } else {
            __card_status = MC_CARD__REMOVED;
            __call_all( __card_status );
        }

        if( MC_CARD__INSERTED == __card_status ) {
            mc_status_t status;

            status = __mc_mount();
            if( MC_RETURN_OK == status ) {
                _D1( "Mounting filesystem\n" );
                __card_status = MC_CARD__MOUNTING;
                __call_all( __card_status );
                if( FR_OK == f_mount(0, &fs) ) {
                    __card_status = MC_CARD__MOUNTED;
                    _D1( "Filesystem mounted\n" );
                } else {
                    __card_status = MC_CARD__UNUSABLE;
                    _D1( "MC_CARD__UNUSABLE\n" );
                }
                __call_all( __card_status );

            } else if( MC_UNUSABLE == status ) {
                __card_status = MC_CARD__UNUSABLE;
                __call_all( __card_status );
            } else {
                /* The card may have been removed during initialization. */
                if( true == __isr_card_present() ) {
                    /* Nope, just can't use the card. */
                    __card_status = MC_CARD__UNUSABLE;
                    __call_all( __card_status );
                }
            }
        }
    }
}

/**
 *  Used to mount a memory card when present in the defined hardware slot.
 *
 *  @return Status.
 *      @retval MC_RETURN_OK        Success.
 *      @retval MC_INIT_ERROR       The card did not respond properly.
 *      @retval MC_UNUSABLE         The card is not compatible.
 *      @retval MC_ERROR_TIMEOUT    The card timed out during IO.
 *      @retval MC_ERROR_MODE       The card is in a generic error state.
 *      @retval MC_CRC_FAILURE      The data retrieved from the card failed the CRC
 *                                      check - the card may not be present anymore.
 */
static mc_status_t __mc_mount( void )
{
    mc_mount_state_t mount_state;
    int retries;

    retries = 10;


    mount_state = MCS_NO_CARD;


    while( 1 ) {
        switch( mount_state ) {
            case MCS_NO_CARD:
                _D2( "MCS_NO_CARD:\n" );
                __mc_type = MCT_UNKNOWN;
                if( true == __isr_card_present() ) {
                    _D2( "MCS_NO_CARD -> MCS_CARD_DETECTED\n" );
                    mount_state = MCS_CARD_DETECTED;
                } else {
                    _D2( "MCS_NO_CARD -> Exit: MC_NOT_MOUNTED\n" );
                    return MC_NOT_MOUNTED;
                }
                break;

            case MCS_CARD_DETECTED:
                _D2( "MCS_CARD_DETECTED:\n" );
                /* Make sure the power is stable. */
                os_task_delay_ms( 250 );
                _D2( "MCS_CARD_DETECTED -> MCS_POWERED_ON\n" );
                mount_state = MCS_POWERED_ON;
                break;

            case MCS_POWERED_ON:
                _D2( "MCS_POWERED_ON:\n" );

                if( BSP_RETURN_OK != io_enable() ) {
                    return MC_INIT_ERROR;
                }

                io_wakeup_card();

                _D2( "MCS_POWERED_ON -> MCS_ASSERT_SPI_MODE\n" );
                mount_state = MCS_ASSERT_SPI_MODE;
                break;

            case MCS_ASSERT_SPI_MODE:
                _D2( "MCS_ASSERT_SPI_MODE:\n" );
                /* Initialize the card into idle state. */
                if( MC_RETURN_OK == __send_card_to_idle_state() ) {
                    _D2( "MCS_ASSERT_SPI_MODE -> MCS_INTERFACE_CONDITION_CHECK\n" );
                    mount_state = MCS_INTERFACE_CONDITION_CHECK;
                } else {
                    _D2( "MCS_ASSERT_SPI_MODE -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;

            case MCS_INTERFACE_CONDITION_CHECK:
            {
                bool legal_command;

                _D2( "MCS_INTERFACE_CONDITION_CHECK:\n" );
                /* See if this card is an SD 2.00 compliant card. */
                if( MC_RETURN_OK == mc_send_if_cond(MC_VOLTAGE, &legal_command) ) {
                    if( true == legal_command ) {
                        _D2( "MCS_INTERFACE_CONDITION_CHECK -> MCS_INTERFACE_CONDITION_SUCCESS\n" );
                        mount_state = MCS_INTERFACE_CONDITION_SUCCESS;
                    } else {
                        _D2( "MCS_INTERFACE_CONDITION_CHECK -> MCS_INTERFACE_CONDITION_FAILURE\n" );
                        mount_state = MCS_INTERFACE_CONDITION_FAILURE;
                    }
                } else {
                    _D2( "MCS_INTERFACE_CONDITION_CHECK -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;
            }

            case MCS_INTERFACE_CONDITION_SUCCESS:
                _D2( "MCS_INTERFACE_CONDITION_SUCCESS:\n" );
                /* See if this card has a compatible voltage range. */
                if( MC_RETURN_OK == __is_card_voltage_compat() ) {
                    _D2( "MCS_INTERFACE_CONDITION_SUCCESS -> MCS_SD20_COMPATIBLE_VOLTAGE\n" );
                    mount_state = MCS_SD20_COMPATIBLE_VOLTAGE;
                } else {
                    _D2( "MCS_INTERFACE_CONDITION_SUCCESS -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;

            case MCS_INTERFACE_CONDITION_FAILURE:
                _D2( "MCS_INTERFACE_CONDITION_FAILURE:\n" );
                /* See if this card has a compatible voltage range. */
                if( MC_RETURN_OK == __is_card_voltage_compat() ) {
                    _D2( "MCS_INTERFACE_CONDITION_FAILURE -> MCS_SD10_COMPATIBLE_VOLTAGE\n" );
                    mount_state = MCS_SD10_COMPATIBLE_VOLTAGE;
                } else {
                    _D2( "MCS_INTERFACE_CONDITION_FAILURE -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;

            case MCS_SD10_COMPATIBLE_VOLTAGE:
            {
                bool ready;

                _D2( "MCS_SD10_COMPATIBLE_VOLTAGE:\n" );
                do {
                    if( MC_RETURN_OK != mc_send_sd_op_cond(false, &ready) ) {
                        _D2( "MCS_SD10_COMPATIBLE_VOLTAGE -> MCS_MMC_CARD\n" );
                        mount_state = MCS_MMC_CARD;
                        break;
                    }
                } while( false == ready );
                _D2( "MCS_SD10_COMPATIBLE_VOLTAGE -> MCS_SD10_CARD_READY\n" );
                mount_state = MCS_SD10_CARD_READY;
                break;
            }

            case MCS_SD20_COMPATIBLE_VOLTAGE:
            {
                bool ready;

                _D2( "MCS_SD20_COMPATIBLE_VOLTAGE:\n" );
                do {
                    if( MC_RETURN_OK != mc_send_sd_op_cond(true, &ready) ) {
                        _D2( "MCS_SD20_COMPATIBLE_VOLTAGE -> MCS_CARD_UNUSABLE\n" );
                        mount_state = MCS_CARD_UNUSABLE;
                        break;
                    }
                } while( false == ready );
                _D2( "MCS_SD20_COMPATIBLE_VOLTAGE -> MCS_SD20_CARD_READY\n" );
                mount_state = MCS_SD20_CARD_READY;
                break;
            }

            case MCS_SD10_CARD_READY:
                _D2( "MCS_SD10_CARD_READY:\n" );
                __mc_type = MCT_SD;
                _D2( "MCS_SD10_CARD_READY -> MCS_DETERMINE_METRICS\n" );
                mount_state = MCS_DETERMINE_METRICS;
                break;

            case MCS_SD20_CARD_READY:
            {
                mc_ocr_t ocr;

                _D2( "MCS_SD20_CARD_READY:\n" );
                if( MC_RETURN_OK == mc_read_ocr(&ocr) ) {
                    if( true == ocr.card_capacity_status ) {
                        __mc_type = MCT_SDHC;
                    } else {
                        __mc_type = MCT_SD_20;
                    }
                    _D2( "MCS_SD20_CARD_READY -> MCS_DETERMINE_METRICS\n" );
                    mount_state = MCS_DETERMINE_METRICS;
                } else {
                    _D2( "MCS_SD20_CARD_READY -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;
            }

            case MCS_MMC_CARD:
                _D2( "MCS_MMC_CARD:\n" );
                if( MC_RETURN_OK == __send_card_to_idle_state() ) {
                    _D2( "MCS_MMC_CARD -> MCS_INTERFACE_CONDITION_CHECK\n" );
                    mount_state = MCS_INTERFACE_CONDITION_CHECK;
                } else {
                    _D2( "MCS_MMC_CARD -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;

            case MCS_DETERMINE_METRICS:
                _D2( "MCS_DETERMINE_METRICS:\n" );
                if( MC_RETURN_OK == __determine_metrics() ) {
                    _D2( "MCS_DETERMINE_METRICS -> MCS_SET_BLOCK_SIZE\n" );
                    mount_state = MCS_SET_BLOCK_SIZE;
                } else {
                    _D2( "MCS_DETERMINE_METRICS -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;

            case MCS_SET_BLOCK_SIZE:
                _D2( "MCS_SET_BLOCK_SIZE:\n" );
                if( MC_RETURN_OK == __set_block_size() ) {
                    _D2( "MCS_SET_BLOCK_SIZE -> MCS_CARD_READY\n" );
                    mount_state = MCS_CARD_READY;
                } else {
                    _D2( "MCS_SET_BLOCK_SIZE -> MCS_CARD_UNUSABLE\n" );
                    mount_state = MCS_CARD_UNUSABLE;
                }
                break;

            case MCS_CARD_READY:
                _D2( "MCS_CARD_READY:\n" );

                _D1( "Mounted Card type: " );
                switch( __mc_type ) {
                    case MCT_UNKNOWN:   _D1( "Unknown\n" ); break;
                    case MCT_MMC:       _D1( "MMC\n"     ); break;
                    case MCT_SD:        _D1( "SD\n"      ); break;
                    case MCT_SD_20:     _D1( "SD 2.0\n"  ); break;
                    case MCT_SDHC:      _D1( "SDHC\n"    ); break;
                }

                _D1( " Size: %llu\n", __mc_size );

                _D2( "MCS_CARD_READY -> Exit: MC_RETURN_OK\n" );
                return MC_RETURN_OK;
                
            case MCS_CARD_UNUSABLE:
                _D2( "MCS_CARD_UNUSABLE: (%d)\n", retries );
                /* I don't like retrying, but for now it works. */
                if( 0 < retries-- ) {
                    _D2( "MCS_CARD_UNUSABLE -> MCS_NO_CARD\n" );
                    mount_state = MCS_NO_CARD;
                } else {
                    _D2( "MCS_CARD_UNUSABLE -> Exit: MC_NOT_MOUNTED\n" );
                    return MC_NOT_MOUNTED;
                }
                break;
        }
    }
}


/**
 *  Used to call all the registered callbacks.
 *
 *  @param status the status to send all the callbacks
 */
static void __call_all( const mc_card_status_t status )
{
    int32_t i;

    for( i = 0; i < AUTOMOUNT_CALLBACK_MAX; i++ ) {
        if( NULL != __callback_fns[i] ) {
            (*__callback_fns[i])( status );
        }
    }
}

static bool __isr_card_present( void )
{
#if (0 != MC_CD_ACTIVE_LOW)
    return (0 == gpio_read_pin(MC_CD_PIN));
#else
    return (0 != gpio_read_pin(MC_CD_PIN));
#endif
}
