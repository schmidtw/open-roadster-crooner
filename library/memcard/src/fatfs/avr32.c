/*
 * Copyright (c) 2009  Weston Schmidt
 */

#include <stddef.h>

#include "../memcard.h"
#include "../block.h"
#include "diskio.h"

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
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
DSTATUS disk_initialize( BYTE drv )
{
    if( 0 != drv ) {
        return STA_NOINIT;
    }

    if( MC_CARD__MOUNTED == mc_get_status() ) {
#if	_READONLY == 0
        return 0;
#else
        return STA_PROTECT;
#endif
    }

    return STA_NODISK;
}

DSTATUS disk_status( BYTE drv )
{
    if( 0 != drv ) {
        return STA_NOINIT;
    }

#if	_READONLY == 0
        return 0;
#else
        return STA_PROTECT;
#endif
}

DRESULT disk_read( BYTE drv, BYTE *buf, DWORD lba, BYTE sector_count )
{
    if( (0 != drv) || (NULL == buf) || (0 == sector_count) ) {
        return RES_PARERR;
    }

    while( 0 < sector_count ) {
        if( MC_RETURN_OK != block_read(lba, buf) ) {
            return RES_ERROR;
        }
        sector_count--;
        lba += 512;
        buf += 512;
    }

    return RES_OK;
}

DRESULT disk_ioctl( BYTE drv, BYTE ctrl, void *buf )
{
    if( 0 != drv ) {
        return RES_PARERR;
    }
    switch( ctrl ) {
#if	_READONLY == 0
        case CTRL_SYNC:
            break;
#endif

        case GET_SECTOR_COUNT:
        {
            uint32_t blocks;
            if( MC_RETURN_OK == mc_get_block_count(&blocks) ) {
                return RES_ERROR;
            }
            *((DWORD*) buf) = (DWORD) blocks;
            break;
        }

        case GET_SECTOR_SIZE:
        {
            *((WORD*) buf) = 512;
            break;
        }

        case GET_BLOCK_SIZE:
        {
            *((DWORD*) buf) = 512;
            break;
        }

        default:
            return RES_PARERR;
    }

    return RES_OK;
}

#if	_READONLY == 0
DRESULT disk_write( BYTE drv, const BYTE *buf, DWORD lba, BYTE sector_count )
{
    if( (0 != drv) || (NULL == buf) || (0 == sector_count) ) {
        return RES_PARERR;
    }

    while( 0 < sector_count ) {
        if( MC_RETURN_OK != block_write(lba, buf) ) {
            return RES_ERROR;
        }
        sector_count--;
        lba += 512;
        buf += 512;
    }

    return RES_OK;
}

DWORD get_fattime( void )
{
    return (31 << 25) | /*   Year: 2011  */
           ( 4 << 21) | /*  Month: April */
           (11 << 16) | /*    Day: 11    */
           (23 << 11) | /*   Hour: 11 PM */
           (32 << 5 ) | /* Minute: 32    */
           ( 4      );  /* Second: 8     */
}
#endif

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
