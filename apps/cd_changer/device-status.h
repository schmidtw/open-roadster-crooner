/*
 * Copyright (c) 2009  Weston Schmidt
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
#ifndef __DEVICE_STATUS_H__
#define __DEVICE_STATUS_H__

typedef enum {
    DS__NO_RADIO_CONNECTION,
    DS__CARD_BEING_SCANNED,
    DS__CARD_UNUSABLE,
    DS__NORMAL
} device_status_t;

/**
 *  Used to initialize the device status system.
 */
void device_status_init( void );

/**
 *  Used to indicate the status of the device via the LED display.
 *
 *  @param status the new status of the device
 */
void device_status_set( const device_status_t status );

/**
 *  Used to get the current status of the device LED display.
 *
 *  @return the status of the device
 */
device_status_t device_status_get( void );

#endif

