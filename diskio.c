/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include "sdcard.h"		/* Example: Header file of existing MMC/SDC contorl module */

/* Definitions of physical drive number for each drive */
#define ATA		0	/* Example: Map ATA harddisk to physical drive 0 */
#define MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define USB		2	/* Example: Map USB MSD to physical drive 2 */

static DSTATUS status = STA_NOINIT;
/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	if (pdrv != 0)
      return STA_NOINIT;

	return status;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	if (pdrv != 0)
      return STA_NOINIT;

	if (sdcard_initialize() == OK) {
		status = 0;
		uart_print("> init DONE\n");
	}
	else {
		status = STA_NOINIT;
	}
	return status;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
	if (disk_status(pdrv) & STA_NOINIT)
		return RES_NOTRDY;

	if (sdcard_readBlock(sector, buff, count) == OK) {
		uart_print("> read DONE\n");
		return RES_OK;
	}

	return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count			/* Number of sectors to write */
)
{
	if (disk_status(pdrv) & STA_NOINIT)
		return RES_NOTRDY;

	if (sdcard_writeBlock(sector, buff, count) == OK) {
		uart_print("> write DONE\n");
		return RES_OK;
	}

	return RES_ERROR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;
	BYTE CSD[16];

	if (disk_status(pdrv) & STA_NOINIT)
		return RES_NOTRDY;

	res = RES_ERROR;
	switch (cmd) {
		case CTRL_SYNC:
			if (sdcard_enable())
				res = RES_OK;
			break;

		case GET_SECTOR_COUNT:
			sdcard_sendCommand(CMD9, 0);
			if (!sdcard_getResponse(R1, 0x00)) {
				break;
			}
			if (sdcard_receiveDataBlock(CSD, 16) == OK) {
				if ((CSD[0] >> 6) == 1) {	/* SDC ver 2.00 */
					DWORD cs = CSD[9] + ((WORD)CSD[8] << 8) + ((DWORD)(CSD[7] & 63) << 16) + 1;
					*(DWORD*)buff = cs << 10;
				} else {					/* SDC ver 1.XX or MMC */
					BYTE n = (CSD[5] & 15) + ((CSD[10] & 128) >> 7) + ((CSD[9] & 3) << 1) + 2;
					DWORD cs = (CSD[8] >> 6) + ((WORD)CSD[7] << 2) + ((WORD)(CSD[6] & 3) << 10) + 1;
					*(DWORD*)buff = cs << (n - 9);
				}
				res = RES_OK;
			}

		case GET_BLOCK_SIZE:
			*(DWORD*)buff = 512;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
	}

	sdcard_disable();

	return res;
}
#endif
