/**
 *  SD card access module for Saturn Gamer's Cartridge
 *  by cafe-alpha
 *
 *  See LICENSE file for details.
**/

#ifndef _SGCLIB_H_
#define _SGCLIB_H_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <stdint.h>

#define C_SEEK_SET  0
#define C_SEEK_CUR  1
#define C_SEEK_END  2

/* File access control and file status flags (FIL.flag) */
#define	FA_READ				0x01
#define	FA_OPEN_EXISTING	0x00

#define	FA_WRITE			0x02
#define	FA_CREATE_NEW		0x04
#define	FA_CREATE_ALWAYS	0x08
#define	FA_OPEN_ALWAYS		0x10

/* File attribute bits for directory entry */
#define AM_RDO  0x01    /* Read only */
#define AM_HID  0x02    /* Hidden */
#define AM_SYS  0x04    /* System */
#define AM_VOL  0x08    /* Volume label */
#define AM_LFN  0x0F    /* LFN entry */
#define AM_DIR  0x10    /* Directory */
#define AM_ARC  0x20    /* Archive */
#define AM_MASK 0x3F    /* Mask of defined bits */

typedef enum
{
	SGC_FR_OK = 0,				/* (0) Succeeded */
	SGC_FR_DISK_ERR,			/* (1) A hard error occurred in the low level disk I/O layer */
	SGC_FR_INT_ERR,				/* (2) Assertion failed */
	SGC_FR_NOT_READY,			/* (3) The physical drive cannot work */
	SGC_FR_NO_FILE,				/* (4) Could not find the file */
	SGC_FR_NO_PATH,				/* (5) Could not find the path */
	SGC_FR_INVALID_NAME,		/* (6) The path name format is invalid */
	SGC_FR_DENIED,				/* (7) Access denied due to prohibited access or directory full */
	SGC_FR_EXIST,				/* (8) Access denied due to prohibited access */
	SGC_FR_INVALID_OBJECT,		/* (9) The file/directory object is invalid */
	SGC_FR_WRITE_PROTECTED,		/* (10) The physical drive is write protected */
	SGC_FR_INVALID_DRIVE,		/* (11) The logical drive number is invalid */
	SGC_FR_NOT_ENABLED,			/* (12) The volume has no work area */
	SGC_FR_NO_FILESYSTEM,		/* (13) There is no valid FAT volume */
	SGC_FR_MKFS_ABORTED,		/* (14) The f_mkfs() aborted due to any parameter error */
	SGC_FR_TIMEOUT,				/* (15) Could not get a grant to access the volume within defined period */
	SGC_FR_LOCKED,				/* (16) The operation is rejected according to the file sharing policy */
	SGC_FR_NOT_ENOUGH_CORE,		/* (17) LFN working buffer could not be allocated */
	SGC_FR_TOO_MANY_OPEN_FILES,	/* (18) Number of open files > _FS_SHARE */
	SGC_FR_INVALID_PARAMETER	/* (19) Given parameter is invalid */
} SGC_FRESULT;

// API
typedef struct
{
    uint32_t size;
    uint16_t date, time;
    uint8_t attrib;
    char name[13]; /* 8.3 file name */
} __attribute__((packed)) sgc_stat_t;


/*
 * API functions prototypes.
 */
typedef int (*Fct_sgc_init    )(void);
typedef int (*Fct_sgc_open    )(const char *filename, int flags);
typedef int (*Fct_sgc_close   )(int fd);
typedef int (*Fct_sgc_seek    )(int fd, int offset, int whence);
typedef int (*Fct_sgc_read    )(int fd, void *buf, int len);
typedef int (*Fct_sgc_write   )(int fd, const void *buf, int len);
typedef int (*Fct_sgc_sync    )(int fd);
typedef int (*Fct_sgc_truncate)(int fd);
typedef int (*Fct_sgc_stat    )(const char *filename, sgc_stat_t *stat, int statsize);
typedef int (*Fct_sgc_rename  )(const char *oldname, const char *newname);
typedef int (*Fct_sgc_mkdir   )(const char *filename);
typedef int (*Fct_sgc_unlink  )(const char *filename);
typedef int (*Fct_sgc_opendir )(const char *path);
typedef int (*Fct_sgc_chdir   )(const char *path);
typedef int (*Fct_sgc_getcwd  )(char *buff, int buflen);


/*
 * The API itself.
 */
typedef struct _sgclib_api_t
{
    Fct_sgc_init     init    ;
    Fct_sgc_open     open    ;
    Fct_sgc_close    close   ;
    Fct_sgc_seek     seek    ;
    Fct_sgc_read     read    ;
    Fct_sgc_write    write   ;
    Fct_sgc_sync     sync    ;
    Fct_sgc_truncate truncate;
    Fct_sgc_stat     stat    ;
    Fct_sgc_rename   rename  ;
    Fct_sgc_mkdir    mkdir   ;
    Fct_sgc_unlink   unlink  ;
    Fct_sgc_opendir  opendir ;
    Fct_sgc_chdir    chdir   ;
    Fct_sgc_getcwd   getcwd  ;
} __attribute__((packed)) sgclib_api_t;

#define SGCLIB_API ((sgclib_api_t*)0x060BA000)


#endif // _SGCLIB_H_
