#include <stdlib.h>
#include <avr32/io.h>
#include <bsp/boards/board.h>
#include <bsp/boards/evk1100/led.h>
#include <bsp/cycle_counter.h>
#include <bsp/gpio.h>
#include <bsp/pm.h>
#include <bsp/usart.h>
#include <freertos/task.h>

void task_led1( void *params );
void task_led2( void *params );
void task_usart( void *params );

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

void setup_usart( void )
{
    static const gpio_map_t usart_gpio_map = {
        { AVR32_USART1_RXD_0_PIN, AVR32_USART1_RXD_0_FUNCTION },
        { AVR32_USART1_TXD_0_PIN, AVR32_USART1_TXD_0_FUNCTION }
    };

    static const usart_options_t usart_options = {
        .baudrate    = 115200,
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

    /* Turn all LEDs on. */
    LED_Display( 0xfffffff1 );

    /* Create the tasks */
    xTaskCreate( task_led1,  (signed portCHAR *) "LED1",  1000, NULL, (tskIDLE_PRIORITY+3), NULL );
    xTaskCreate( task_led2,  (signed portCHAR *) "LED2",  1000, NULL, (tskIDLE_PRIORITY+2), NULL );
    xTaskCreate( task_usart, (signed portCHAR *) "USART", 1000, NULL, (tskIDLE_PRIORITY+1), NULL );

    /* Start the RTOS - never returns. */
    vTaskStartScheduler();

    return 0;
}

void task_led1( void *params )
{
    uint32_t actual_leds, leds = 0;

    while( true ) {
        vTaskDelay( 100 );

        taskENTER_CRITICAL();
        actual_leds = LED_Read_Display();
        actual_leds &= 0xf0;
        actual_leds |= leds;
        LED_Display( actual_leds );
        taskEXIT_CRITICAL();
        leds++;
        leds &= 0x0f;
    }
}

void task_led2( void *params )
{
    uint32_t actual_leds, leds = 0;

    while( true ) {
        vTaskDelay( 110 );

        taskENTER_CRITICAL();
        actual_leds = LED_Read_Display();
        actual_leds &= 0x0f;
        actual_leds |= (leds << 4);
        LED_Display( actual_leds );
        taskEXIT_CRITICAL();
        leds++;
        leds &= 0x0f;
    }
}

void task_usart( void *params )
{
    while( true ) {
        vTaskDelay( 1000 );
        usart_write_line( &AVR32_USART1, "Hello, world.\n" );
    }
}
