#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr32/io.h>
#include <bsp/boards/board.h>
#include <bsp/boards/evk1100/led.h>
#include <bsp/cycle_counter.h>
#include <bsp/gpio.h>
#include <bsp/pm.h>
#include <bsp/memcard.h>
#include <bsp/usart.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <reent.h>
#include <efsl/efs.h>
#include <efsl/ls.h>

#include <util/xxd.h>

void task_led1( void *params );
void task_sd( void *params );

void* pvPortMalloc( size_t size )
{
    void *ret;

    vTaskSuspendAll();
    ret = malloc( size );
    xTaskResumeAll();

    return ret;
}

void vPortFree( void *ptr )
{
    if( NULL != ptr ) {
        vTaskSuspendAll();
        free( ptr );
        xTaskResumeAll();
    }
}

#if 0
//_ssize_t _write( int fd, const void *buf, size_t cnt )
_ssize_t _write_r( struct _reent *reent, int fd, const void *buf, size_t cnt )
{
    if( (1 == fd) || (2 == fd) ) {
        int i;
        for( i = 0; i < cnt; i++ ) {
            usart_putchar( &AVR32_USART1, ((char*) buf)[i] );
        }
    }

    return cnt;
}

int _open_r( struct _reent *reent, const char *file, int flags, int mode )
{
    printf( "_open_r( %p, '%s', 0x%08x, 0x%08x )\n", reent, file, flags, mode );
    return -1;
}

int _open( const char *file, int flags, int mode )
{
    printf( "_open( '%s', 0x%08x, 0x%08x )\n", file, flags, mode );
    return -1;
}
#endif

void sleep( uint32_t ms )
{
    vTaskDelay( ms );
}

void setup_usart( void )
{
    static const gpio_map_t usart_gpio_map[] = {
        { AVR32_USART1_RXD_0_PIN, AVR32_USART1_RXD_0_FUNCTION },
        { AVR32_USART1_TXD_0_PIN, AVR32_USART1_TXD_0_FUNCTION }
    };

    static const usart_options_t usart_options = {
        .baudrate    = 460800,
        .charlength  = USART_DATA_BITS_8,
        .paritytype  = USART_PARITY_NONE,
        .stopbits    = USART_STOPBITS_1,
        .channelmode = USART_MODE_NORMAL
    };

    gpio_enable_module( usart_gpio_map, 2 );
    usart_init_rs232( &AVR32_USART1, &usart_options, FOSC0 );
}

int main( void )
{
    /* Start the crystal oscillator 0 and switch the main clock to it. */
    pm_switch_to_osc0( &AVR32_PM, FOSC0, OSC0_STARTUP );

    /* Turn all LEDs off. */
    LED_Display( 0 );

    setup_usart();

    /* Create the tasks */
    xTaskCreate( task_led1, (signed portCHAR *) "LED1",  1000, NULL, (tskIDLE_PRIORITY+4), NULL );
    xTaskCreate( task_sd,   (signed portCHAR *) "SD",    5000, NULL, (tskIDLE_PRIORITY+3), NULL );

    printf( "--------------------------------------------------------------------------------\n" );
    /* Start the RTOS - never returns. */
    vTaskStartScheduler();

    return 0;
}

void task_led1( void *params )
{
    while( true ) {
        vTaskDelay( 1000 );

        taskENTER_CRITICAL();
        LED_Toggle( 0x01 );
        taskEXIT_CRITICAL();
    }
}


void task_sd( void *params )
{
    bsp_status_t status;
    memcard_settings_t mem_options = {
        .chip = SD_MMC_SPI_NPCS,
        .map  = {
                    { AVR32_SPI1_SCK_0_PIN,  AVR32_SPI1_SCK_0_FUNCTION  },  // SPI Clock.
                    { AVR32_SPI1_MISO_0_PIN, AVR32_SPI1_MISO_0_FUNCTION },  // MISO.
                    { AVR32_SPI1_MOSI_0_PIN, AVR32_SPI1_MOSI_0_FUNCTION },  // MOSI.
                    { AVR32_SPI1_NPCS_1_PIN, AVR32_SPI1_NPCS_1_FUNCTION },  // Chip Select NPCS.
                },
        .lock = NULL,
        .unlock = NULL,
        .user_data = "Hello, resource.",
        .sleep = sleep,
        .pdca_rx_channel_id = 0,
        .pdca_rx_peripheral_id = AVR32_PDCA_PID_SPI1_RX,
        .pdca_tx_channel_id = 1,
        .pdca_tx_peripheral_id = AVR32_PDCA_PID_SPI1_TX,
        .irq = AVR32_PDCA_IRQ_0,
        .level = AVR32_INTC_INT1,
        .path = "/dev/sd0"
    };

    int last = 0;

    mem_options.spi = SD_MMC_SPI;

    status = mc_init( &mem_options, 1, 12000000, FOSC0 );

    gpio_enable_gpio_pin( SD_MMC_CARD_DETECT_PIN );

    while( true ) {
        int cd, wp, current;

        vTaskDelay( 100 );

        cd = gpio_get_pin_value( SD_MMC_CARD_DETECT_PIN );
        wp = gpio_get_pin_value( SD_MMC_WRITE_PROTECT_PIN );

        current = wp << 1 | cd;

        if( (current != last) || (BSP_RETURN_OK != status) ) {
            last = current;
            //printf( "State changed! - cd: %#010x, wd: %#010x\n", cd, wp );
            if( 0 == cd ) {
                mc_unmount( 0 );
                taskENTER_CRITICAL();
                LED_Off( 0x06 );
                taskEXIT_CRITICAL();
            } else {
                taskENTER_CRITICAL();
                LED_On( 0x02 );
                if( 0 == wp ) {
                    LED_On( 0x04 );
                } else {
                    LED_Off( 0x04 );
                }
                taskEXIT_CRITICAL();

                status = mc_mount( 0 );
                if( BSP_RETURN_OK == status ) {
                    EmbeddedFileSystem efs;
                    int8_t res;
                    printf( "card mounted\n" );
                    res = efs_init(&efs, "/dev/sd0");
                    if( 0 == res ) {
                        DirList list;
                        ls_openDir( &list, &efs.myFs, "/" );
                        while( 0 == ls_getNext(&list) ) {
                            list.currentEntry.FileName[LIST_MAXLENFILENAME-1] = '\0';
                            printf( "%c %8lu %s\n", (ATTR_DIRECTORY & list.currentEntry.Attribute) ? 'd' : 'f', list.currentEntry.FileSize, list.currentEntry.FileName );
                        }
                        fs_umount( &efs.myFs );
                    } else {
                        printf( "res: %d\n", res );
                    }
                } else {
                    vTaskDelay( 1000 );
                }
            }
        }
    }
}
