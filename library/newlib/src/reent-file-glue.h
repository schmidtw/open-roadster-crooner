/*
 * Copyright (c) 2010  Weston Schmidt
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
#ifndef __REENT_FILE_GLUE_H__
#define __REENT_FILE_GLUE_H__

#include <stdint.h>
#include <stdbool.h>

#include <sys/reent.h>

#include <linked-list/linked-list.h>

#define REENT_GLUE_MAGIC        0x22221234
#define REENT_GLUE_MAX_FILES    3

struct reent_glue {
    struct _reent reent;    /* Needs to be first so we can re-cast this struct */
    uint32_t magic;         /* Should always be REENT_GLUE_MAGIC if valid. */
    __FILE _stdout;

    bool active[REENT_GLUE_MAX_FILES];
    void *file[REENT_GLUE_MAX_FILES];
};

/**
 *  Used to initialize the reent_glue_t structure.
 *
 *  @param p the pointer to initialize
 */
#define reent_glue_init( p )                            \
{                                                       \
    int __i;                                            \
                                                        \
    _REENT_INIT_PTR( (&((p)->reent)) );                 \
    (p)->reent._stdout = &((p)->_stdout);               \
    (p)->magic = REENT_GLUE_MAGIC;                      \
    for( __i = 0; __i < REENT_GLUE_MAX_FILES; __i++ ) { \
        (p)->active[__i] = false;                       \
        (p)->file[__i] = NULL;                          \
    }                                                   \
}

#endif
