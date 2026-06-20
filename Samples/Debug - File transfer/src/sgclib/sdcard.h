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


/**
 * Verify if SD card was reinserted or not.
 *
 * Return values :
 *  0 : SD card wasn't reinserted.
 *  1 : SD card was reinserted, hence SPI & FAT library reinit is needed.
**/
int sdc_is_reinsert(void);


/**
 * Detect SD card, and read its internal parameters.
 * Must be called when the user inserted a SD card in the slot.
 *
 * Return SDC_OK when init successed.
**/
sdc_ret_t sdc_init(void);



/**
 * Send packet to SD card and read the response from SD card.
 * Return the first 4 bytes read from SD card.
 * (Full returned data is stored in `sdcard_t* _sdcard' stuff)
**/
unsigned char sdc_sendpacket(unsigned char cmd, unsigned long arg, unsigned char* buffer, unsigned long blocks_count);



/**
 * Read/write multiple blocks from/to SD card.
 *
 * Return 0 if no error, otherwise the response byte is returned.
**/
unsigned char sdc_read_multiple_blocks(unsigned long start_block, unsigned char* buffer, unsigned long blocks_count);
unsigned char sdc_write_multiple_blocks(unsigned long start_block, unsigned char* buffer, unsigned long blocks_count);


/**
 * Extra function : turn OFF/ON LED on SD card reader board.
 *
 * Return SDC_OK when successed.
**/
sdc_ret_t sdc_ledset(unsigned long led_color, unsigned long led_state);


/**
 *------------------------------------------------------------------
 * Internal-use functions. (Low level I/O related)
 *------------------------------------------------------------------
**/

/**
 * Get/Set signals from SD card.
**/
void sdc_output(void);


/**
 * Assert/Deassert SD's "Chip Select" line.
**/
void sdc_cs_assert(void);
void sdc_cs_deassert(void);

/**
 * Send byte array to SD card.
 * Send byte (8 bits) to SD card.
 * Send long (32 bits) to SD card.
**/
void sdc_sendbyte_array(unsigned char* ptr, unsigned short len);
void sdc_sendbyte(unsigned char dat);
void sdc_sendlong(unsigned long dat);
/**
 * Receive byte array from SD card.
 * Receive byte (8 bits) from SD card.
**/
void sdc_receivebyte_array(unsigned char* ptr, unsigned short len);
unsigned char sdc_receivebyte(void);


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
} __attribute__((packed)) sdclib_api_t;

#define SDCLIB_API ((sdclib_api_t*)0x060BA000)


#endif /* _SDCARD_INCLUDE_H_ */

