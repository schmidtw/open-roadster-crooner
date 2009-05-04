#ifndef __FLAC_AVR32_H__
#define __FLAC_AVR32_H__

#include <stdint.h>

void lpc_decode_avr32( int32_t blocksize, int32_t qlevel,
                       int32_t pred_order, int32_t* data,
                       int16_t* coeffs, int32_t* output );

#endif
