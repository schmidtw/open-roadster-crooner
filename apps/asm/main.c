#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/sysregs.h>
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
extern void lpc_decode_avr32( int32_t blocksize, int32_t qlevel, int32_t pred_order,
                       int32_t* data, int16_t* coeffs );
void lpc_decode_working( int32_t blocksize, int32_t qlevel, int32_t pred_order,
                         int32_t* data, int16_t* coeffs );

int main( void )
{
#define DATA_SIZE   4000
    int32_t i, j, k;
    int16_t coeffs[32];
    int32_t data_loop[DATA_SIZE];
    int32_t data_asm[DATA_SIZE];

    uint32_t loop_before, loop_after;
    uint32_t asm_before, asm_after;

    for( i = 0; i < 32; i++ ) {
        coeffs[i] = i + 1;
    }


    printf( "--------------------------------------------------------------------------------\n" );

    for( j = 1; j < 33; j++ ) {
        for( k = -1; k < 2; k += 2 ) {

            for( i = 0; i < DATA_SIZE; i++ ) {
                data_loop[i] = i + 10;
                data_asm[i] = i + 10;
            }

            printf( "\nlpc_data_working( %lu, %ld, %lu %p, %p )\n", (DATA_SIZE-j), k, j, &data_loop[j], coeffs );

            loop_before = (uint32_t) __builtin_mfsr( SYSREG_COUNT );
            lpc_decode_working( (DATA_SIZE-j), k, j, &data_loop[j], coeffs );
            loop_after = (uint32_t) __builtin_mfsr( SYSREG_COUNT );

            asm_before = (uint32_t) __builtin_mfsr( SYSREG_COUNT );
            lpc_decode_avr32( (DATA_SIZE-j), k, j, &data_asm[j], coeffs );
            asm_after = (uint32_t) __builtin_mfsr( SYSREG_COUNT );

            for( i = 0; i < DATA_SIZE; i++ ) {
                if( data_loop[i] != data_asm[i] ) {
#if 0
                    for( j = 0; j < i; j++ ) {
                        printf( "data_loop[%ld] (0x%08lx) == data_asm[%ld] (0x%08lx)\n", j, data_loop[j], j, data_asm[j] );
                    }
#endif
                    printf( "data_loop[%ld] (0x%08lx) != data_asm[%ld] (0x%08lx)\n", i, data_loop[i], i, data_asm[i] );
                    while( 1 ) {;}
                }
            }

            printf( "Before: %lu After: %lu Savings: %lu (%lu%%)\n",
                    (loop_after - loop_before), (asm_after - asm_before),
                    (loop_after - loop_before) - (asm_after - asm_before),
                    (((loop_after - loop_before) * 100)/ (asm_after - asm_before)) );
        }
    }
    
    printf( "All ok - done!\n" );
    while( 1 ) {;}

    return 0;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
void lpc_decode_working( int32_t blocksize, int32_t qlevel, int32_t pred_order,
                         int32_t* data, int16_t* coeffs )
{
    int32_t i;
    int32_t j;

    if( 0 < qlevel  ) {
        for( i = 0; i < blocksize; i++ ) {
            int32_t sum = 0;
            for( j = 0; j < pred_order; j++ ) {
                sum += coeffs[j] * data[i-j-1];
            }

            data[i] += sum >> qlevel;
        }
    } else {
        qlevel *= -1;
        for( i = 0; i < blocksize; i++ ) {
            int32_t sum = 0;
            for( j = 0; j < pred_order; j++ ) {
                sum += coeffs[j] * data[i-j-1];
            }

            data[i] += sum << qlevel;
        }
    }
}

