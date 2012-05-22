/*
 *  system-log.h - the system logger utility
 *
 *  Written by Weston Schmidt (2012)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  In other words, you are welcome to use, share and improve this program.
 *  You are forbidden to forbid anyone else to use, share and improve
 *  what you give them.   Help stamp out software-hoarding!
 */
#ifndef _SYSTEM_LOG_H_
#define _SYSTEM_LOG_H_

#include <stdint.h>

typedef enum {
    SYSLOG_RETURN_OK,
    SYSLOG_RESOURCE_ERROR
} syslog_status_t;

syslog_status_t system_log_init( const uint32_t priority, const char *filename );

#endif
