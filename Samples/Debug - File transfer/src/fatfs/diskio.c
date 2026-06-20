/*-----------------------------------------------------------------------*/
/* Low level disk control module for Win32              (C)ChaN, 2019    */
/*-----------------------------------------------------------------------*/

#include "../fatfs/diskio.h"		/* Declarations of disk functions */

#include "../sgclib/sdcard.h"
//#include "../log.h"

#define WINDOWS_PLATFORM 0


#define	SZ_BLOCK	256		/* Block size to be returned by GET_BLOCK_SIZE command */



/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv		/* Physical drive nmuber */
)
{
    //log_out("@@@ disk_initialize STT");

    //sdc_ret_t ret = sdc_init();
    sdc_init();

    //log_out("@@@ disk_initialize END");

    return 0;

#if WINDOWS_PLATFORM == 1
	DSTATUS sta;

	if (pdrv >= Drives) {
		sta = STA_NOINIT;
	} else {
		get_status(pdrv);
		sta = Stat[pdrv].status;
	}

	return sta;
#endif // WINDOWS_PLATFORM == 1
return 0;
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0) */
)
{
    //log_out("@@@ disk_status STT");
    //log_out("@@@ disk_status END");
    return 0;

#if WINDOWS_PLATFORM == 1
	DSTATUS sta;

	if (pdrv >= Drives) {
		sta = STA_NOINIT;
	} else {
		sta = Stat[pdrv].status;
	}

	return sta;
#endif // WINDOWS_PLATFORM == 1
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,			/* Physical drive nmuber (0) */
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	LBA_t sector,		/* Start sector number (LBA) */
	UINT count			/* Number of sectors to read */
)
{
    //log_out("@@@ disk_read STT sector[%08X] count[%u]", sector, count);

    sdc_read_multiple_blocks(sector, buff, count);

    //log_out("@@@ disk_read END");

    return RES_OK;

#if WINDOWS_PLATFORM == 1
	DWORD nc/*, rnc*/;
	LARGE_INTEGER ofs;
	DSTATUS res;


	if (pdrv >= Drives || Stat[pdrv].status & STA_NOINIT) {
		return RES_NOTRDY;
	}

	nc = (DWORD)count * Stat[pdrv].sz_sector;
	ofs.QuadPart = (QWORD)sector * Stat[pdrv].sz_sector;

	/* RAM disk */
	if (ofs.QuadPart >= (QWORD)SZ_RAMDISK * 1024 * 1024) {
		res = RES_ERROR;
	} else {
		memcpy(buff, RamDisk + ofs.LowPart, nc);
		res = RES_OK;
	}

	return res;
#endif // WINDOWS_PLATFORM == 1
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0) */
	const BYTE *buff,	/* Pointer to the data to be written */
	LBA_t sector,		/* Start sector number (LBA) */
	UINT count			/* Number of sectors to write */
)
{
    //log_out("@@@ disk_write STT sector[%08X] count[%u]", sector, count);

    sdc_write_multiple_blocks(sector, (unsigned char*)buff, count);

    //log_out("@@@ disk_write END");

    return RES_OK;

#if WINDOWS_PLATFORM == 1
	DWORD nc = 0/*, rnc*/;
	LARGE_INTEGER ofs;
	DRESULT res;


	if (pdrv >= Drives || Stat[pdrv].status & STA_NOINIT) {
		return RES_NOTRDY;
	}

	res = RES_OK;
	if (Stat[pdrv].status & STA_PROTECT) {
		res = RES_WRPRT;
	} else {
		nc = (DWORD)count * Stat[pdrv].sz_sector;
		if (nc > BUFSIZE) res = RES_PARERR;
	}

	ofs.QuadPart = (QWORD)sector * Stat[pdrv].sz_sector;

	/* RAM disk */
	if (ofs.QuadPart >= (QWORD)SZ_RAMDISK * 1024 * 1024) {
		res = RES_ERROR;
	} else {
		memcpy(RamDisk + ofs.LowPart, buff, nc);
		res = RES_OK;
	}

	return res;
#endif // WINDOWS_PLATFORM == 1
return 0;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive data */
)
{
    //log_out("@@@ disk_ioctl ctrl[%u]", ctrl);

	DRESULT res = RES_OK;

	switch (ctrl) {
	case CTRL_SYNC:			/* Nothing to do */
		res = RES_OK;
		break;

	case GET_SECTOR_COUNT:	/* Get number of sectors on the drive */
        //log_out("@@@ disk_ioctl GET_SECTOR_COUNT");
		*(LBA_t*)buff = 0;
		res = RES_OK;
		break;

	case GET_SECTOR_SIZE:	/* Get size of sector for generic read/write */
        //log_out("@@@ disk_ioctl GET_SECTOR_SIZE");
		*(WORD*)buff = 512;
		res = RES_OK;
		break;

	case GET_BLOCK_SIZE:	/* Get internal block size in unit of sector */
        //log_out("@@@ disk_ioctl GET_BLOCK_SIZE");
		//*(DWORD*)buff = SZ_BLOCK;
		res = RES_OK;
		break;

	case 200:				/* Load disk image file to the RAM disk (drive 0) */
        //log_out("@@@ disk_ioctl 200");
		break;

	}

	return res;



#if WINDOWS_PLATFORM == 1
	DRESULT res;


	if (pdrv >= Drives || (Stat[pdrv].status & STA_NOINIT)) {
		return RES_NOTRDY;
	}

	res = RES_PARERR;
	switch (ctrl) {
	case CTRL_SYNC:			/* Nothing to do */
		res = RES_OK;
		break;

	case GET_SECTOR_COUNT:	/* Get number of sectors on the drive */
		*(LBA_t*)buff = Stat[pdrv].n_sectors;
		res = RES_OK;
		break;

	case GET_SECTOR_SIZE:	/* Get size of sector for generic read/write */
		*(WORD*)buff = Stat[pdrv].sz_sector;
		res = RES_OK;
		break;

	case GET_BLOCK_SIZE:	/* Get internal block size in unit of sector */
		*(DWORD*)buff = SZ_BLOCK;
		res = RES_OK;
		break;

	case 200:				/* Load disk image file to the RAM disk (drive 0) */
		{
			HANDLE h;
			DWORD br;

			if (pdrv == 0) {
				h = CreateFileW(buff, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
				if (h != INVALID_HANDLE_VALUE) {
					if (ReadFile(h, RamDisk, SZ_RAMDISK * 1024 * 1024, &br, 0)) {
						res = RES_OK;
					}
					CloseHandle(h);
				}
			}
		}
		break;

	}

	return res;
#endif // WINDOWS_PLATFORM == 1
return 0;
}




#if !FF_FS_READONLY && !FF_FS_NORTC
DWORD get_fattime(void)
{
DWORD _sd_timestamp = 0;
  if(_sd_timestamp == 0)
  {
    /* If no better timestamp available, set it up
     * according to the build time stamp of this module.
     */
    const char *months_list[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    char build_date[32] = {'\0'};
    strcpy(build_date, __DATE__);
    build_date[6] = '\0';
    build_date[11] = '\0';

    unsigned long year = (unsigned long)atoi(build_date + 7);

    unsigned long month = 0;
    for (int i = 0; i < 12; i++)
    {
        if (memcmp(build_date, months_list[i], 3) == 0)
        {
            month = i + 1;
            break;
        }
    }

    unsigned long day = (unsigned long)atoi(build_date + 4);

    char build_time[32] = {'\0'};
    strcpy(build_time, __TIME__);
    build_time[2] = '\0';
    build_time[5] = '\0';
    build_time[8] = '\0';

    unsigned long hour   = (unsigned long)atoi(build_time + 0);
    unsigned long minute = (unsigned long)atoi(build_time + 3);
    unsigned long second = (unsigned long)atoi(build_time + 6);

    _sd_timestamp =
        ((year - 1980) << 25)
        | (month       << 21)
        | (day         << 16)
        | (hour        << 11)
        | (minute      <<  5)
        | (second      >>  1);
  }

  return _sd_timestamp;
}
#endif


