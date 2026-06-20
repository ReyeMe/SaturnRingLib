/**
 *  SD card access module for Saturn Gamer's Cartridge
 *  by cafe-alpha
 *
 *  See LICENSE file for details.
**/
/**
 *  SD card module for Sega Saturn.
 *
 *  Links:
 *   - Easy to understand HW and SW ressources :
 *      http://matty.99k.org/sd-card-parallel-port/
 *   - About HW (well explained) :
 *      http://elm-chan.org/docs/mmc/mmc_e.html
 *      http://gandalf.arubi.uni-kl.de/avr_projects/arm_projects/arm_memcards/index.html
 *   - C source code for SD card SPI access :
 *      http://www.koders.com/c/fid2ED5A1007B1CC397DCC985205254C754392EC716.aspx
 *   - FAT32-related ressources (old debug versions of this library were made from here) :
 *      http://www.pjrc.com/tech/8051/ide/fat32.html
 *      http://code.google.com/p/thinfat32/
 *      http://www.dharmanitech.com/
**/

#ifndef _SDCARD_INCLUDE_H_
#define _SDCARD_INCLUDE_H_

#include <stdint.h>

/* Structure alignment macros. */
#define SC_ALIGN_4_BYTES(_STRUCT_NAME_) _STRUCT_NAME_ __attribute__((aligned(4)))
#define SC_ALIGN_2_BYTES(_STRUCT_NAME_) _STRUCT_NAME_ __attribute__((aligned(2)))


/**
 *------------------------------------------------------------------
 * Error codes returned by each functions.
 *------------------------------------------------------------------
**/
typedef long sdc_ret_t;
#define SDC_OK         0
#define SDC_FAILURE    1
#define SDC_NOHARDWARE 2
#define SDC_NOCARD     3
#define SDC_FATERROR   4
#define SDC_NOFILE     5
#define SDC_TIMEOUT    6



/**
 *------------------------------------------------------------------
 * Definitions & structures.
 *------------------------------------------------------------------
**/

// GCC have alternative #pragma pack(N) syntax and old gcc version not
// support pack(push,N), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

/* Some stats about SD card currently used. */
typedef struct _sdcard_stats_t
{
    /*------------------------------------------*/
    /* CID register - CMD10. */
    unsigned char cid[16];

    /* CSD register - CMD9. */
    unsigned char csd[16];

    /*------------------------------------------*/
    /* SD card type.
     * 0:SD, 1:SDHC, 2-3:unsupported type.
     */
    unsigned char sdhc;

    /* File format. */
#define SDC_FILEFORMAT_PARTITIONTABLE 0
#define SDC_FILEFORMAT_FLOPPYLIKE     1
#define SDC_FILEFORMAT_UNIVERSAL      2
#define SDC_FILEFORMAT_UNKNOWN        3
    unsigned char file_format;

    /* Unused, for 4 byte alignment. */
    unsigned char reserved[2];

    /* Number of 512 bytes sectors. */
    unsigned long sectors_cnt;
} SC_ALIGN_4_BYTES(sdcard_stats_t);


/**
 * Structure : sdcard_io_t
 * Stores SD card I/O data.
**/
typedef struct _sdcard_io_t
{
    /*----------------------*/
    /* SD card SPI signals. */
    unsigned char csl;         /* SD chip selection   : Sat -> SD */
    unsigned char din;         /* SD data input       : Sat -> SD */
    unsigned char clk;         /* SD clock            : Sat -> SD */
    //unsigned char dout;      /* SD data output      : Sat <- SD */
    unsigned char reserved[1]; /* Unused, for 4 byte alignment.   */
} SC_ALIGN_4_BYTES(sdcard_io_t);



/**
 * Structure : sdcard_t
 * Purpose : structure in RAM containing global data for use by SD card controller.
 * 
 * Note: BUP_Init function receives pointers to 2 buffers in RAM :
 *  unsigned long	BackUpLibrary[4096]; 16KB, 4 bytes aligned
 *  unsigned long	BackUpRamWork[2048];  8KB, 4 bytes aligned
 * So sdcard_t should be designed to get a size less than 16KB.
**/
#define SDC_CMDBUFFER_LEN 24
typedef struct _sdcard_t
{
    /* I/O status. */
    sdcard_io_t s;

    /* SD card related statistics. */
    sdcard_stats_t stats;

    /* Packet-related informations. */
    unsigned char packet_timeout;
    unsigned char ccs;
    unsigned char sd_ver2;
    unsigned char mmc;

    /*
     * Buffer to receive status and register data.
     * Note: 4 bytes alignment is required.
     * Note: register data can be found from the pointer below :
     *       stat_reg_buffer + status_len
     */
    unsigned char stat_reg_buffer[SDC_CMDBUFFER_LEN];
    unsigned char status_len;

    /* 0: SD card removed, 1: SD card inserted. */
    unsigned char inserted;

    /* Set to 1 when everything from SPI to FAT library is initialized correctly. */
    unsigned char init_ok;

    /* Unused. */
    unsigned char unused[1];

    /* SD card init status :
     *  - Value returned by sdc_init function (positive values).
     *  - Value returned by fl_attach_media function (negative values).
     */
    sdc_ret_t init_status;
} SC_ALIGN_4_BYTES(sdcard_t);


// GCC have alternative #pragma pack() syntax and old gcc version not
// support pack(pop), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif


/**
 * Extra function : turn OFF/ON LED on SD card reader board.
 *
 * Return SDC_OK when successed.
**/
/* First parameter : LED color. */
#define SDC_LED_RED   0
#define SDC_LED_GREEN 1
/* Second parameter : LED state. */
#define SDC_LED_OFF 0
#define SDC_LED_ON  1





/**
 *------------------------------------------------------------------
 * Logging/debugging-related define switchs.
 *------------------------------------------------------------------
**/

//#   define sdc_logout(_STR_, ...)       {scd_usbsd_logout(sc_get_sdcard_buff()->log_sdcard, _STR_, __VA_ARGS__);      }
//#   define sdc_msgout(_ID_, _STR_, ...) {scd_usbsd_msgout(sc_get_sdcard_buff()->log_sdcard, _ID_, _STR_, __VA_ARGS__);}
#   define sdc_logout(_STR_, ...)
#   define sdc_msgout(_ID_, _STR_, ...)


/**
 *------------------------------------------------------------------
 * End-user functions. (Packet-level related)
 * Note: don't need to call when using FAT-32 functions.
 *------------------------------------------------------------------
**/

/* Data block size : only 512 bytes supported. */
#define SDC_BLOCK_SIZE 512

/* Timeout value (in poll count) when waiting for data start bit. */
#define SDC_POLL_COUNTMAX 200000


//SD commands, many of these are not used here
#define SDC_GO_IDLE_STATE            0
#define SDC_SEND_OP_COND             1
#define SDC_SEND_IF_COND             8
#define SDC_SEND_CSD                 9
#define SDC_SEND_CID                 10
#define SDC_STOP_TRANSMISSION        12
#define SDC_SEND_STATUS              13
#define SDC_SET_BLOCK_LEN            16
#define SDC_READ_SINGLE_BLOCK        17
#define SDC_READ_MULTIPLE_BLOCKS     18
#define SDC_WRITE_SINGLE_BLOCK       24
#define SDC_WRITE_MULTIPLE_BLOCKS    25
#define SDC_ERASE_BLOCK_START_ADDR   32
#define SDC_ERASE_BLOCK_END_ADDR     33
#define SDC_ERASE_SELECTED_BLOCKS    38
#define SDC_SD_SEND_OP_COND          41   //ACMD
#define SDC_APP_CMD                  55
#define SDC_READ_OCR                 58
#define SDC_CRC_ON_OFF               59


/* -------------------------------------------------------------------------- */
/* SGCLIB binary dispatch wrappers                                             */
/*
 * sdc_* functions are implemented in sgclib.bin and loaded at 0x060BA000.
 * Use these inline wrappers from regular C/C++ code paths.
 */
#ifndef SGCLIB_BIN_BASE_ADDR
#define SGCLIB_BIN_BASE_ADDR 0x060BA000UL
#endif

#define SDC_BIN_FN(_ret_, _ofs_, ...) \
  ((_ret_ (*)(__VA_ARGS__))((uintptr_t)SGCLIB_BIN_BASE_ADDR + (uintptr_t)(_ofs_)))

static inline int sdc_is_reinsert(void)
{
    return SDC_BIN_FN(int, 0x0A24UL, void)();
}

static inline sdc_ret_t sdc_init(void)
{
    return SDC_BIN_FN(sdc_ret_t, 0x0F6CUL, void)();
}

static inline unsigned char sdc_sendpacket(unsigned char cmd, unsigned long arg, unsigned char* buffer, unsigned long blocks_count)
{
    return SDC_BIN_FN(unsigned char, 0x0A30UL, unsigned char, unsigned long, unsigned char*, unsigned long)(cmd, arg, buffer, blocks_count);
}

static inline unsigned char sdc_read_multiple_blocks(unsigned long start_block, unsigned char* buffer, unsigned long blocks_count)
{
    return SDC_BIN_FN(unsigned char, 0x14ECUL, unsigned long, unsigned char*, unsigned long)(start_block, buffer, blocks_count);
}

static inline unsigned char sdc_write_multiple_blocks(unsigned long start_block, unsigned char* buffer, unsigned long blocks_count)
{
    return SDC_BIN_FN(unsigned char, 0x156CUL, unsigned long, unsigned char*, unsigned long)(start_block, buffer, blocks_count);
}

static inline sdc_ret_t sdc_ledset(unsigned long led_color, unsigned long led_state)
{
    return SDC_BIN_FN(sdc_ret_t, 0x0794UL, unsigned long, unsigned long)(led_color, led_state);
}

static inline void sdc_output(void)
{
    SDC_BIN_FN(void, 0x0770UL, void)();
}

static inline void sdc_cs_assert(void)
{
    SDC_BIN_FN(void, 0x07BCUL, void)();
}

static inline void sdc_cs_deassert(void)
{
    SDC_BIN_FN(void, 0x07D8UL, void)();
}

static inline void sdc_sendbyte_array(unsigned char* ptr, unsigned short len)
{
    SDC_BIN_FN(void, 0x07F4UL, unsigned char*, unsigned short)(ptr, len);
}

static inline void sdc_sendbyte(unsigned char dat)
{
    SDC_BIN_FN(void, 0x0878UL, unsigned char)(dat);
}

static inline void sdc_sendlong(unsigned long dat)
{
    SDC_BIN_FN(void, 0x0894UL, unsigned long)(dat);
}

static inline void sdc_receivebyte_array(unsigned char* ptr, unsigned short len)
{
    SDC_BIN_FN(void, 0x08C4UL, unsigned char*, unsigned short)(ptr, len);
}

static inline unsigned char sdc_receivebyte(void)
{
    return SDC_BIN_FN(unsigned char, 0x0978UL, void)();
}

#undef SDC_BIN_FN

#endif /* _SDCARD_INCLUDE_H_ */

