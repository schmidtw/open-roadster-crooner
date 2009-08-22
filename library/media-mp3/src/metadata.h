/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: metadata.h 21695 2009-07-06 22:40:45Z mt $
 *
 * Copyright (C) 2005 Dave Chapman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _METADATA_H
#define _METADATA_H

#include <stdbool.h>


/* Audio file types. */
/* NOTE: The values of the AFMT_* items are used for the %fc tag in the WPS
         - so new entries MUST be added to the end to maintain compatibility.
 */
enum
{
    AFMT_UNKNOWN = 0,  /* Unknown file format */

    /* start formats */

    AFMT_MPA_L1,       /* MPEG Audio layer 1 */
    AFMT_MPA_L2,       /* MPEG Audio layer 2 */
    AFMT_MPA_L3,       /* MPEG Audio layer 3 */

    AFMT_NUM_CODECS,
};

/** Database of audio formats **/
/* record describing the audio format */
struct afmt_entry
{
    char label[8];      /* format label */
    char *ext_list;     /* double NULL terminated extension
                           list for type with the first as
                           the default for recording */
};

/* database of labels and codecs. add formats per above enum */
extern const struct afmt_entry audio_formats[AFMT_NUM_CODECS];

#if MEMORYSIZE > 2
#define ID3V2_BUF_SIZE 900
#else
#define ID3V2_BUF_SIZE 300
#endif

enum {
    ID3_VER_1_0 = 1,
    ID3_VER_1_1,
    ID3_VER_2_2,
    ID3_VER_2_3,
    ID3_VER_2_4
};

#define MAX_PATH 300

struct mp3entry {
    char path[MAX_PATH];
    char* title;
    char* artist;
    char* album;
    char* genre_string;
    char* disc_string;
    char* track_string;
    char* year_string;
    char* composer;
    char* comment;
    char* albumartist;
    char* grouping;
    int discnum;    
    int tracknum;
    int version;
    int layer;
    int year;
    unsigned char id3version;
    unsigned int codectype;
    unsigned int bitrate;
    unsigned long frequency;
    unsigned long id3v2len;
    unsigned long id3v1len;
    unsigned long first_frame_offset; /* Byte offset to first real MP3 frame.
                                         Used for skipping leading garbage to
                                         avoid gaps between tracks. */
    unsigned long vbr_header_pos;
    unsigned long filesize; /* without headers; in bytes */
    unsigned long length;   /* song length in ms */
    unsigned long elapsed;  /* ms played */

    int lead_trim;          /* Number of samples to skip at the beginning */
    int tail_trim;          /* Number of samples to remove from the end */

    /* Added for Vorbis */
    unsigned long samples;  /* number of samples in track */

    /* MP3 stream specific info */
    unsigned long frame_count; /* number of frames in the file (if VBR) */

    /* Used for A52/AC3 */
    unsigned long bytesperframe; /* number of bytes per frame (if CBR) */

    /* Xing VBR fields */
    bool vbr;
    bool has_toc;           /* True if there is a VBR header in the file */
    unsigned char toc[100]; /* table of contents */

    /* these following two fields are used for local buffering */
    char id3v2buf[ID3V2_BUF_SIZE];
    char id3v1buf[4][92];

    /* resume related */
    unsigned long offset;  /* bytes played */
    int index;             /* playlist index */

    /* runtime database fields */
    long tagcache_idx;     /* 0=invalid, otherwise idx+1 */
    int rating;
    int score;
    long playcount;
    long lastplayed;
    long playtime;
    
    /* replaygain support */
    
    char* track_gain_string;
    char* album_gain_string;
    double track_gain;
    double album_gain;
    double track_peak;
    double album_peak;

    /* Cuesheet support */
    int cuesheet_type;      /* 0: none, 1: external, 2: embedded */

    /* Musicbrainz Track ID */
    char* mb_track_id;
};

unsigned int probe_file_format(const char *filename);
bool get_metadata(struct mp3entry* id3, int fd, const char* trackname);
bool mp3info(struct mp3entry *entry, const char *filename);
void adjust_mp3entry(struct mp3entry *entry, void *dest, const void *orig);
void copy_mp3entry(struct mp3entry *dest, const struct mp3entry *orig);

#endif


