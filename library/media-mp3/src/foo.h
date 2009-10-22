/* Copyright (C) 2006-2008, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __ID3_H__
#define __ID3_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
   uint8_t ver1;
   uint8_t ver10;
   uint8_t ver100;
} St_id3ver;

typedef struct
{
  size_t prepended_tag;
  size_t appended_tag;
} St_id3size;


/**
 *  Returns the ID3 size from ID3 of current file selected.
 */
extern St_id3size reader_id3_size ( void );

//! @brief Return ID3 version from ID3 of current file selected
//!
extern St_id3ver reader_id3_version ( void );

//! @brief Return title from ID3 of current file selected
//!
extern bool reader_id3_title   ( char *sz_title , uint8_t u8_size_max );

//! @brief Return artist from ID3 of current file selected
//!
extern bool reader_id3_artist ( char *sz_artist , uint8_t u8_size_max );

//! @brief Return album from ID3 of current file selected
//!
extern bool reader_id3_album   ( char *sz_album , uint8_t u8_size_max );

//! @brief Return year from ID3 of current file selected
//!
extern uint16_t reader_id3_year    ( void );

//! @brief Return track from ID3 of current file selected
//!
extern uint32_t  reader_id3_track   ( void );

//! @brief Return genre from ID3 of current file selected
//!
extern bool reader_id3_genre   ( char *sz_genre , uint8_t u8_size_max );

//! @brief Return the length of the audiofile in milliseconds
//!
extern uint32_t  reader_id3_duration ( void );


#endif  // _READER_ID3_H_
