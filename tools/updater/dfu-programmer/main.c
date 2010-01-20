/*
 * dfu-programmer
 *
 * $Id: main.c 71 2008-10-02 05:48:41Z schmidtw $
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

#include <stdio.h>
#include <string.h>
#include <usb.h>

//#include "config.h"
#include "dfu-device.h"
#include "dfu.h"
#include "atmel.h"
#include "intel_hex.h"
//#include "arguments.h"
//#include "commands.h"

#define VENDOR_ID                   0x03eb
#define CHIP_ID                     0x2FF8
#define INITIAL_ABORT               false
#define HONOR_INTERFACE_CLASS       true
#define FLASH_ADDRESS_BOTTOM        0x2000
#define FLASH_ADDRESS_TOP           (0x80000 - 1)
#define BOOTLOADER_BOTTOM           0
#define BOOTLOADER_TOP              (0x2000 - 1)
#define IGNORE_BOOTLOADER_OVERLAP   true
#define FLASH_PAGE_SIZE             512

#define MY_EOL  0x01
#define MY_EOF  0x02

#include "../object.c"
/* Gives us:
 * const uint8_t binary_data[]
 * const uint32_t binary_data_length
 */

static int32_t my_fgets_fn( void *data_stream, const size_t bytes, char *data );
static int32_t my_getc( void *data_stream );
static int32_t flash( dfu_device_t *device );

int debug;

struct data_payload_t {
    uint8_t *data;
    uint8_t *last;
};

int main( int argc, char **argv )
{
    static const char *progname = "Crooner Updater";
    int retval = 0;
    dfu_device_t dfu_device;
    struct usb_device *device = NULL;


    memset( &dfu_device, 0, sizeof(dfu_device) );

/*
    args.target = tar_at32uc3a0512;
    args.chip_id = 0x2FF8;
    args.vendor_id = 0x03eb;
    args.device_type = adc_AVR32;
    args.eeprom_memory_size = 0;
    args.flash_page_size = 512;
    args.eeprom_page_size = 0;
    args.initial_abort = false;
    args.honor_interfaceclass = true;
    args.memory_address_top = 0x80000 - 1;
    args.memory_address_bottom = 0;
    args.flash_address_top = 0x80000 - 1;
    args.flash_address_bottom = 0x2000;
    args.bootloader_bottom = 0;
    args.bootloader_top = 0x2000 - 1;
    args.bootloader_at_highmem = false;
    args.command = com_erase;
    args.com_erase_data.suppress_validation = 0;
    strcpy( args.device_type_string, "AVR32" );

    if( args.command == com_version ) {
        printf( PACKAGE_STRING "\n" );
        return 0;
    }
*/
    if( debug >= 200 ) {
        usb_set_debug( debug );
    }

    usb_init();

    device = dfu_device_init( VENDOR_ID, CHIP_ID, &dfu_device,
                              INITIAL_ABORT, HONOR_INTERFACE_CLASS );

    if( NULL == device ) {
        fprintf( stderr, "%s: no device present.\n", progname );
        retval = 1;
        goto error;
    }

    dfu_device.type = adc_AVR32;

    /* Erase the device */
    fprintf( stderr, "Erasing Crooner..." );
    fflush( stderr );
    if( 0 != atmel_erase_flash(&dfu_device, ATMEL_ERASE_ALL) ) {
        fprintf( stderr, "FAILED\n" );
        retval = 1;
        goto error;
    }
    fprintf( stderr, "SUCCESS\n" );

    fprintf( stderr, "Verifying erased..." );
    fflush( stderr );
    if( 0 != atmel_blank_check(&dfu_device, FLASH_ADDRESS_BOTTOM, FLASH_ADDRESS_TOP) ) {
        fprintf( stderr, "FAILED\n" );
        retval = 1;
        goto error;
    }
    fprintf( stderr, "SUCCESS\n" );

    /* Flash the new code to the device */
    if( 0 != flash(&dfu_device) ) {
        retval = 1;
        goto error;
    }

    retval = 0;

error:

    if( NULL != dfu_device.handle ) {
        if( 0 != usb_release_interface(dfu_device.handle, dfu_device.interface) ) {
            fprintf( stderr, "%s: failed to release interface %d.\n",
                             progname, dfu_device.interface );
            retval = 1;
        }
    }

    if( NULL != dfu_device.handle ) {
        if( 0 != usb_close(dfu_device.handle) ) {
            fprintf( stderr, "%s: failed to close the handle.\n", progname );
            retval = 1;
        }
    }

    return retval;
}

static int32_t my_fgets_fn( void *data_stream, const size_t bytes, char *data )
{
    size_t count = 0;
    struct data_payload_t *d = (struct data_payload_t*) data_stream;

    memset( data, 0, bytes );

    /* Go to the next line */
    if( MY_EOL == *d->data ) {
        d->data++;
    }

    while( (MY_EOL != *d->data) &&
           (MY_EOF != *d->data) &&
           ('\r'   != *d->data) &&
           ('\n'   != *d->data) &&
           (d->data < d->last) &&
           (count < (bytes - 1)) )
    {
        *data = *d->data;
        data++;
        d->data++;
        count++;
    }

    return count;
}

static int32_t my_getc( void *data_stream )
{
    int32_t rv;
    struct data_payload_t *d = (struct data_payload_t*) data_stream;

    rv = EOF;

try_again:
    if( d->data < d->last ) {
        if( MY_EOL == *d->data ) {          /* End of Line */
            d->data++;
            goto try_again;
        } else if( MY_EOF == *d->data ) {   /* End of File */
            d->data++;
        } else {
            rv = *d->data;
            d->data++;
        }
    }

    return rv;
}

static int32_t flash( dfu_device_t *device )
{
    int16_t *hex_data = NULL;
    int32_t  usage = 0;
    int32_t  retval = -1;
    int32_t  result = 0;
    uint8_t *buffer = NULL;
    int32_t  i,j;
    uint32_t memory_size;
    uint32_t adjusted_flash_top_address;

    struct data_payload_t payload;
    intel_hex_data_reader_t readers;

    payload.data = (uint8_t*) binary_data;
    payload.last = &payload.data[binary_data_length + 1];

    readers.fgets = &my_fgets_fn;
    readers.fgetc = &my_getc;
    readers.data_stream = &payload;

    /* Why +1? Because the flash_address_top location is inclusive, as
     * apposed to most times when sizes are specified by length, etc.
     * and they are exclusive. */
    adjusted_flash_top_address = FLASH_ADDRESS_TOP + 1;

    memory_size = adjusted_flash_top_address - FLASH_ADDRESS_BOTTOM;

    buffer = (uint8_t *) malloc( memory_size );

    if( NULL == buffer ) {
        fprintf( stderr, "Request for %d bytes of memory failed.\n",
                 memory_size );
        goto error;
    }

    memset( buffer, 0, memory_size );

    hex_data = intel_hex_to_buffer( &readers, adjusted_flash_top_address, &usage );
    if( NULL == hex_data ) {
        fprintf( stderr,
                 "Something went wrong with creating the memory image.\n" );
        goto error;
    }

    for( i = BOOTLOADER_BOTTOM; i <= BOOTLOADER_TOP; i++) {
        if( -1 != hex_data[i] ) {
            if( true == IGNORE_BOOTLOADER_OVERLAP ) {
                //If we're ignoring the bootloader, don't write to it
                hex_data[i] = -1;
            } else {
                fprintf( stderr, "Bootloader and code overlap.\n" );
                fprintf( stderr, "Use --suppress-bootloader-mem to ignore\n" );
                goto error;
            }
        }
    }

    for( i = 0; i < FLASH_ADDRESS_BOTTOM; i++ ) {
        if( -1 != hex_data[i] ) {
            fprintf( stderr, "Attempted to write to illegal memory address.\n" );
            goto error;
        }
    }

    fprintf( stderr, "Flashing Crooner..." );
    fflush( stderr );
    result = atmel_flash( device, hex_data, FLASH_ADDRESS_BOTTOM,
                          adjusted_flash_top_address, FLASH_PAGE_SIZE, false );

    if( result < 0 ) {
        fprintf( stderr, "FAILED\n" );
        goto error;
    }
    fprintf( stderr, "SUCCESS\n" );

    fprintf( stderr, "Validating transfer..." );

    result = atmel_read_flash( device, FLASH_ADDRESS_BOTTOM,
                               adjusted_flash_top_address, buffer,
                               memory_size, false, false );

    if( memory_size != result ) {
        fprintf( stderr, "FAILED\n" );
        goto error;
    }

    for( i = 0, j = FLASH_ADDRESS_BOTTOM; i < result; i++, j++ ) {
        if( (0 <= hex_data[j]) && (hex_data[j] < UINT8_MAX) ) {
            /* Memory should have been programmed in this location. */
            if( ((uint8_t) hex_data[j]) != buffer[i] ) {
                fprintf( stderr, "FAILED\n" );
                goto error;
            }
        }
    }
    fprintf( stderr, "SUCCESS\n" );

    retval = 0;

error:
    if( NULL != buffer ) {
        free( buffer );
        buffer = NULL;
    }

    if( NULL != hex_data ) {
        free( hex_data );
        hex_data = NULL;
    }

    return retval;
}

