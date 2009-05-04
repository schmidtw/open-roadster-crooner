#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <bsp/boards/boards.h>
#include <bsp/bsp_errors.h>
#include <bsp/led.h>
#include <bsp/gpio.h>
#include <bsp/pm.h>
#include <bsp/delay.h>

#define MT48LC16M16A2
//#define MT48LC4M16A2

#ifdef MT48LC16M16A2

#define SDRAM_PRESENT   1
#define SDRAM_BUS_WIDTH 16      /* bits */
#define SDRAM_BANK_BITS 2       /* bits */
#define SDRAM_ROW_BITS  13      /* bits */
#define SDRAM_COL_BITS  9       /* bits */

#define SDRAM_CAS       2

#define SDRAM_tRC       60      /* ns */
#define SDRAM_tRP       15      /* ns */
#define SDRAM_tRCD      15      /* ns */
#define SDRAM_tRAS      37      /* ns */
#define SDRAM_tXSR      67      /* ns */
#define SDRAM_tREFRESH  7812    /* ns */
#define SDRAM_tRFC      66      /* ns */
/* defined in terms of clock */
#define SDRAM_tWR       14      /* ns */
#define SDRAM_tMRD      31      /* ns */

#endif

#ifdef MT48LC4M16A2

#define SDRAM_PRESENT   1
#define SDRAM_BUS_WIDTH 16      /* bits */
#define SDRAM_BANK_BITS 2       /* bits */
#define SDRAM_ROW_BITS  12      /* bits */
#define SDRAM_COL_BITS  8       /* bits */

#define SDRAM_CAS       2

#define SDRAM_tRC       66      /* ns */
#define SDRAM_tRP       20      /* ns */
#define SDRAM_tRCD      20      /* ns */
#define SDRAM_tRAS      44      /* ns */
#define SDRAM_tXSR      75      /* ns */
#define SDRAM_tREFRESH  15625   /* ns */
#define SDRAM_tRFC      66      /* ns */
/* defined in terms of clock */
#define SDRAM_tWR       14      /* ns */
#define SDRAM_tMRD      31      /* ns */
#endif

size_t sdram_get_size( void );
bsp_status_t sdram_init( void );
void* sdram_get_pointer( void );

int main( void )
{
    uint32_t *sdram;
    uint32_t sdram_size;
    uint32_t i;
    uint32_t foo[10];
    int error_count;

    led_init();

    sdram_size = sdram_get_size();
    sdram_size /= 4;

    sdram = sdram_get_pointer();

    sdram_init();

    led_on( led_blue );
    led_off( led_red );
    led_off( led_green );

    //sdram_size = 0x8000;

    printf( "Running: %lu 0x%08lx\n", sdram_size, sdram_size );

    for( i = 0; i < sdram_size; i++ ) {
        sdram[i] = 0xffffffff;
    }

    for( i = 0; i < sdram_size; i++ ) {
        sdram[i] = i;
    }

    led_off( led_blue );

    error_count = 0;

    for( i = 0; i < sdram_size; i++ ) {
        uint32_t f;

        f = sdram[i];
        if( f != i ) {
            if( error_count < 15 ) {
                printf( "failed at: 0x%08lx 0x%08lx\n", i, f );
            }
            error_count++;
        }
    }

    if( 0 == error_count ) {
        printf( "success\n" );
        led_on( led_green );
    } else {
        printf( "failure\n" );
        led_on( led_red );
    }

    printf( "done\n" );
    while( 1 ) { ; }

    return 0;
}

#if (1 != SDRAM_PRESENT)

size_t sdram_get_size( void )
{
    return 0;
}

bsp_status_t sdram_init( void )
{
    return BSP_RETURN_OK;
}

void* sdram_get_pointer( void )
{
    return NULL;
}

#else   /* SDRAM is present */

#if (16 != SDRAM_BUS_WIDTH)
#error "SDRAM_BUS_WIDTH must be 16"
#endif

#if ((1 != SDRAM_BANK_BITS) && (2 != SDRAM_BANK_BITS))
#error "SDRAM_BANK_BITS must be 1 or 2"
#endif

#if ((SDRAM_ROW_BITS < 11) || (13 < SDRAM_ROW_BITS))
#error "SDRAM_ROW_BITS must be [11 - 13]"
#endif

#if ((SDRAM_COL_BITS < 8) || (11 < SDRAM_COL_BITS))
#error "SDRAM_COL_BITS must be [8 - 11]"
#endif

#if ((SDRAM_CAS < 1) || (3 < SDRAM_CAS))
#error "SDRAM_CAS must be [1 - 3]"
#endif

size_t sdram_get_size( void )
{
    size_t size;

    /* banks = 2 ^ (bank bits)
     * rows  = 2 ^ (row bits)
     * cols  = 2 ^ (col bits)
     *
     * size = banks * rows * cols * (data bus width in bytes)
     * -or-
     * size = 2 ^ (bank bits + row bits + col bits) * (data bus width in bytes)
     */

    size = 1 << (SDRAM_BANK_BITS + SDRAM_ROW_BITS + SDRAM_COL_BITS);
    return (size * (SDRAM_BUS_WIDTH / 8));
}

bsp_status_t sdram_init( void )
{
    int32_t i;
    bsp_status_t status;
    uint32_t clock, clock_up;
    uint32_t dummy;
    volatile uint16_t *sdram = ((void*) AVR32_EBI_CS1_ADDRESS);

    static const gpio_map_t sdram_map[] = {
        /* Data bus lines */
        { AVR32_EBI_DATA_0_PIN,  AVR32_EBI_DATA_0_FUNCTION  },
        { AVR32_EBI_DATA_1_PIN,  AVR32_EBI_DATA_1_FUNCTION  },
        { AVR32_EBI_DATA_2_PIN,  AVR32_EBI_DATA_2_FUNCTION  },
        { AVR32_EBI_DATA_3_PIN,  AVR32_EBI_DATA_3_FUNCTION  },
        { AVR32_EBI_DATA_4_PIN,  AVR32_EBI_DATA_4_FUNCTION  },
        { AVR32_EBI_DATA_5_PIN,  AVR32_EBI_DATA_5_FUNCTION  },
        { AVR32_EBI_DATA_6_PIN,  AVR32_EBI_DATA_6_FUNCTION  },
        { AVR32_EBI_DATA_7_PIN,  AVR32_EBI_DATA_7_FUNCTION  },
        { AVR32_EBI_DATA_8_PIN,  AVR32_EBI_DATA_8_FUNCTION  },
        { AVR32_EBI_DATA_9_PIN,  AVR32_EBI_DATA_9_FUNCTION  },
        { AVR32_EBI_DATA_10_PIN, AVR32_EBI_DATA_10_FUNCTION },
        { AVR32_EBI_DATA_11_PIN, AVR32_EBI_DATA_11_FUNCTION },
        { AVR32_EBI_DATA_12_PIN, AVR32_EBI_DATA_12_FUNCTION },
        { AVR32_EBI_DATA_13_PIN, AVR32_EBI_DATA_13_FUNCTION },
        { AVR32_EBI_DATA_14_PIN, AVR32_EBI_DATA_14_FUNCTION },
        { AVR32_EBI_DATA_15_PIN, AVR32_EBI_DATA_15_FUNCTION },

        /* Address lines */
        { AVR32_EBI_ADDR_2_PIN,  AVR32_EBI_ADDR_2_FUNCTION  },  /* A00 */
        { AVR32_EBI_ADDR_3_PIN,  AVR32_EBI_ADDR_3_FUNCTION  },  /* A01 */
        { AVR32_EBI_ADDR_4_PIN,  AVR32_EBI_ADDR_4_FUNCTION  },  /* A02 */
        { AVR32_EBI_ADDR_5_PIN,  AVR32_EBI_ADDR_5_FUNCTION  },  /* A03 */
        { AVR32_EBI_ADDR_6_PIN,  AVR32_EBI_ADDR_6_FUNCTION  },  /* A04 */
        { AVR32_EBI_ADDR_7_PIN,  AVR32_EBI_ADDR_7_FUNCTION  },  /* A05 */
        { AVR32_EBI_ADDR_8_PIN,  AVR32_EBI_ADDR_8_FUNCTION  },  /* A06 */
        { AVR32_EBI_ADDR_9_PIN,  AVR32_EBI_ADDR_9_FUNCTION  },  /* A07 */
        { AVR32_EBI_ADDR_10_PIN, AVR32_EBI_ADDR_10_FUNCTION },  /* A08 */
        { AVR32_EBI_ADDR_11_PIN, AVR32_EBI_ADDR_11_FUNCTION },  /* A09 */
        { AVR32_EBI_SDA10_0_PIN, AVR32_EBI_SDA10_0_FUNCTION },  /* A10 */
        { AVR32_EBI_ADDR_13_PIN, AVR32_EBI_ADDR_13_FUNCTION },  /* A11 */
        { AVR32_EBI_ADDR_14_PIN, AVR32_EBI_ADDR_14_FUNCTION },  /* A12 */

        /* Bank address lines */
        { AVR32_EBI_ADDR_16_PIN, AVR32_EBI_ADDR_16_FUNCTION },
        { AVR32_EBI_ADDR_17_PIN, AVR32_EBI_ADDR_17_FUNCTION },

        /* Data mask lines */
        { AVR32_EBI_ADDR_0_PIN, AVR32_EBI_ADDR_0_FUNCTION },
        { AVR32_EBI_NWE1_0_PIN, AVR32_EBI_NWE1_0_FUNCTION },

        /* Control lines */
        { AVR32_EBI_SDWE_0_PIN, AVR32_EBI_SDWE_0_FUNCTION },
        { AVR32_EBI_CAS_0_PIN,  AVR32_EBI_CAS_0_FUNCTION  },
        { AVR32_EBI_RAS_0_PIN,  AVR32_EBI_RAS_0_FUNCTION  },
        { AVR32_EBI_NCS_1_PIN,  AVR32_EBI_NCS_1_FUNCTION  },

        /* Clock lines */
        { AVR32_EBI_SDCK_0_PIN,  AVR32_EBI_SDCK_0_FUNCTION  },
        { AVR32_EBI_SDCKE_0_PIN, AVR32_EBI_SDCKE_0_FUNCTION }
    };

    status = gpio_enable_module( sdram_map,
                                 sizeof(sdram_map)/sizeof(gpio_map_t) );

    if( BSP_RETURN_OK != status ) {
        return status;
    }

    /* Select SDRAM on NCS1 */
    AVR32_HMATRIX.sfr[AVR32_EBI_HMATRIX_NR] |= 1 << AVR32_EBI_SDRAM_CS;

    /* HSB clock is the same as the CPU for UC3 devices. */
    clock = pm_get_frequency( PM__PBB );

    clock_up = (clock + 999999) / 1000000;

#define ROUND_UP(x)     ((((x) * clock_up) + 999) / 1000)

    /* Configure the SDRAM settings */
    AVR32_SDRAMC.CR.nc = (SDRAM_COL_BITS - 8);
    AVR32_SDRAMC.CR.nr = (SDRAM_ROW_BITS - 11);
    AVR32_SDRAMC.CR.nb = (SDRAM_BANK_BITS - 1);
    AVR32_SDRAMC.CR.cas = SDRAM_CAS;
    AVR32_SDRAMC.CR.dbw = 1; /* Always 16 bits */

    /* Round up because these are minimums */
    AVR32_SDRAMC.CR.txsr = ROUND_UP( SDRAM_tXSR );
    if( AVR32_SDRAMC.CR.txsr < 2 ) {
        AVR32_SDRAMC.CR.txsr = 2;
    }

    AVR32_SDRAMC.CR.twr  = ROUND_UP( SDRAM_tWR );
    if( AVR32_SDRAMC.CR.twr < 2 ) {
        AVR32_SDRAMC.CR.twr = 2;
    }

    AVR32_SDRAMC.CR.trc  = ROUND_UP( SDRAM_tRC );
    AVR32_SDRAMC.CR.trp  = ROUND_UP( SDRAM_tRP );
    AVR32_SDRAMC.CR.trcd = ROUND_UP( SDRAM_tRCD );
    AVR32_SDRAMC.CR.tras = ROUND_UP( SDRAM_tRAS );

    AVR32_SDRAMC.lpr = 0;
    AVR32_SDRAMC.hsr = 0;
    AVR32_SDRAMC.mdr = 0;

    delay_time( 100000 );

    /* Start the generation of SDRAMC signals */
    AVR32_SDRAMC.mr = AVR32_SDRAMC_MR_MODE_NOP;
    dummy = AVR32_SDRAMC.mr;
    sdram[0] = 0;

    /* Wait for the clock to become stable */
    delay_time( 100000 );

    /* Start the generation of SDRAMC signals */
    AVR32_SDRAMC.mr = AVR32_SDRAMC_MR_MODE_NOP;
    dummy = AVR32_SDRAMC.mr;
    sdram[0] = 0;

    AVR32_SDRAMC.mr = AVR32_SDRAMC_MR_MODE_BANKS_PRECHARGE;
    dummy = AVR32_SDRAMC.mr;
    sdram[0] = 0;
    //delay_time( SDRAM_tRP );
    asm( "nop" );
    asm( "nop" );

    /* Start the sdram auto refreshing */
    AVR32_SDRAMC.mr = AVR32_SDRAMC_MR_MODE_AUTO_REFRESH;
    dummy = AVR32_SDRAMC.mr;
    for( i = 0; i < 8; i++ ) {
        sdram[0] = 0;
        asm( "nop" );
        asm( "nop" );
        asm( "nop" );
        asm( "nop" );
        asm( "nop" );
        //delay_time( SDRAM_tRFC );
    }

    /* Configure the memory with the desired parameters */
    AVR32_SDRAMC.mr = AVR32_SDRAMC_MR_MODE_LOAD_MODE;
    dummy = AVR32_SDRAMC.mr;
    sdram[0] = 0;
    asm( "nop" );
    asm( "nop" );
    asm( "nop" );
    asm( "nop" );
    asm( "nop" );
    asm( "nop" );
    //delay_time( SDRAM_tMRD );

    AVR32_SDRAMC.mr = AVR32_SDRAMC_MR_MODE_NORMAL;
    dummy = AVR32_SDRAMC.mr;
    sdram[0] = 0;

    asm( "nop" );
    asm( "nop" );
    asm( "nop" );
    asm( "nop" );
    asm( "nop" );
    asm( "nop" );

    /* Set the refresh value (rounding down since it is a maximum value) */
    AVR32_SDRAMC.TR.count = ((SDRAM_tREFRESH * (clock / 1000)) / 1000000);
    dummy = AVR32_SDRAMC.tr;

    printf( "clock: %lu\n", clock );
    printf( "----------\n" );
    printf( "   nc: %u\n", AVR32_SDRAMC.CR.nc );
    printf( "   nr: %u\n", AVR32_SDRAMC.CR.nr );
    printf( "   nb: %u\n", AVR32_SDRAMC.CR.nb );
    printf( "  dbw: %u\n", AVR32_SDRAMC.CR.dbw );
    printf( "----------\n" );
    printf( "  cas: %u\n", AVR32_SDRAMC.CR.cas );
    printf( "  twr: %u\n", AVR32_SDRAMC.CR.twr );
    printf( "  trc: %u\n", AVR32_SDRAMC.CR.trc );
    printf( "  trp: %u\n", AVR32_SDRAMC.CR.trp );
    printf( " trcd: %u\n", AVR32_SDRAMC.CR.trcd );
    printf( " tras: %u\n", AVR32_SDRAMC.CR.tras );
    printf( " txsr: %u\n", AVR32_SDRAMC.CR.txsr );
    printf( "   tr: %u\n", AVR32_SDRAMC.TR.count );
/*
*/

    return BSP_RETURN_OK;
}

void* sdram_get_pointer( void )
{
    return (void*) AVR32_EBI_CS1_ADDRESS;
}
#endif
