/*
 * Copyright (c) 2011  Weston Schmidt
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
#ifndef __REBOOT_H__
#define __REBOOT_H__

#include <stdint.h>
#include <stdio.h>

#define MAX_STACK_DEPTH 16

typedef struct {
    uint32_t cpu_count;
    uint32_t bear;
    uint32_t config0;
    uint32_t exception_cause;
    uint32_t return_address;
    uint32_t r[16];
    uint32_t sr;
    uint32_t stack[MAX_STACK_DEPTH]; /* 0 if not needed */
    uint32_t checksum;
} reboot_trace_t;

/**
 *  Used to get the stack trace of the last reboot if there is one.
 *
 *  @note The last reboot stack trace will not be available if the device
 *        is powered down.
 *  @note The memory returned is static & should not be altered/freed.
 *
 *  @return the data of the last reboot if present, or NULL if not
 */
reboot_trace_t *reboot_get_last( void );

void reboot_output( FILE *out, reboot_trace_t *trace );
#endif
