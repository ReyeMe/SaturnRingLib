/* FLD kernel, rebuilt in C from SEGA's FLD 1.21 (original/fld_knl.bin, 1996-02-28).
 *
 * SEGA's kernel is FLD itself plus a cut-down copy of the CDC library (CD block commands). This file keeps only
 * the code the kernel runs, with the same CD block commands in the same order, the same timeouts (as loop counts)
 * and the same error codes. Function comments give the address of the original function.
 *
 * Flow: the game IP code calls the loader vector [0x06002270] = [0x00200000] = FLD_1stRead. It reads the first
 * file (id 2, after "." and "..") of the DDS directory to the IP's 1st read address. On error it starts the BIOS
 * CD player instead.
 */
#include <stdint.h>

/* BIOS and IP addresses */
#define BIOS_CD_PLAYER (*(int (*volatile *)(void))0x0600026C)
#define BIOS_1ST_READ_ADDRESS (*(volatile uint32_t *)0x060002B0)
#define BIOS_SET_INTERRUPT (*(void (*volatile *)(uint32_t, void (*)(void)))0x06000300)
#define DDS_DIRECTORY_ID (*(volatile uint16_t *)0x06000CCE)
#define IP_VBLANK_IN ((void (*)(void))0x060022E4)
#define IP_LOGO_COUNTER (*(volatile int32_t *)0x06002D80)
#define VBLANK_IN_VECTOR 0x40

/* CD block registers */
#define CD_HIRQ (*(volatile uint16_t *)0x25890008)
#define CD_CR1 (*(volatile uint16_t *)0x25890018)
#define CD_CR2 (*(volatile uint16_t *)0x2589001C)
#define CD_CR3 (*(volatile uint16_t *)0x25890020)
#define CD_CR4 (*(volatile uint16_t *)0x25890024)
#define CD_DATA16 (*(volatile uint16_t *)0x25898000)
#define CD_DATA32 (*(volatile uint32_t *)0x25818000)

/* HIRQ bits */
#define HIRQ_CMOK 0x0001
#define HIRQ_DRDY 0x0002
#define HIRQ_DCHG 0x0020
#define HIRQ_ESEL 0x0040
#define HIRQ_EHST 0x0080
#define HIRQ_EFLS 0x0200

/* Drive status (low nibble of the status byte) */
#define STATUS_PAUSE 1
#define STATUS_STANDBY 2
#define STATUS_OPEN 6
#define STATUS_NODISC 7
#define STATUS_ERROR 9
#define STATUS_FATAL 10

#define ROOT_DIRECTORY 0xFFFFFF
#define FIRST_FILE 2
#define SECTOR_WORDS 512

/* HIRQ bits seen by the commands (0x00201378 in the original) */
static uint16_t hirq_seen;

/* --- Interrupt mask (SR.I = 15) around CD block register accesses --- */

static inline uint32_t mask_interrupts(void)
{
    uint32_t sr;
    __asm__ volatile("stc sr,%0" : "=r"(sr));
    __asm__ volatile("ldc %0,sr" : : "r"(sr | 0xF0) : "memory");
    return sr;
}

static inline void restore_interrupts(uint32_t saved)
{
    uint32_t sr;
    __asm__ volatile("stc sr,%0" : "=r"(sr));
    __asm__ volatile("ldc %0,sr" : : "r"((sr & ~0xF0u) | (saved & 0xF0u)) : "memory");
}

/* --- CD block command layer --- */

/* 0x002011E0: waits for one of the HIRQ bits */
static int wait_hirq_any(uint16_t bits)
{
    for (int32_t i = 0; i < 0x240000; i++)
    {
        if (CD_HIRQ & bits)
        {
            return 0;
        }
    }

    return -3;
}

/* 0x00201126: sends a command. The bits of `done` must have been seen (previous command of that kind over) */
static int execute(uint16_t done, const uint16_t cmd[4], uint16_t response[4])
{
    hirq_seen |= CD_HIRQ;

    if ((hirq_seen & done) != done)
    {
        return -1;
    }

    if (!(hirq_seen & HIRQ_CMOK))
    {
        return -2;
    }

    CD_HIRQ = ~(done | HIRQ_CMOK);
    hirq_seen &= ~HIRQ_CMOK;
    CD_CR1 = cmd[0];
    CD_CR2 = cmd[1];
    CD_CR3 = cmd[2];
    CD_CR4 = cmd[3];

    int result = wait_hirq_any(HIRQ_CMOK);

    if (result != 0)
    {
        return result;
    }

    response[0] = CD_CR1;
    response[1] = CD_CR2;
    response[2] = CD_CR3;
    response[3] = CD_CR4;

    uint8_t status = response[0] >> 8;

    if (status == 0xFF)
    {
        return -5; /* rejected */
    }

    if (status & 0x80)
    {
        return -6; /* wait */
    }

    hirq_seen &= ~done;
    return 0;
}

/* 0x00201276 (and its wrappers 0x00201260, 0x00201264): a command with interrupts masked */
static int command(uint16_t done, uint16_t cr1, uint16_t cr2, uint16_t cr3, uint16_t cr4, uint16_t response[4])
{
    const uint16_t cmd[4] = {cr1, cr2, cr3, cr4};
    uint32_t sr = mask_interrupts();
    int result = execute(done, cmd, response);
    restore_interrupts(sr);
    return result;
}

/* Commands whose response is not used */
static int simple_command(uint16_t done, uint16_t cr1, uint16_t cr2, uint16_t cr3, uint16_t cr4)
{
    uint16_t response[4];
    return command(done, cr1, cr2, cr3, cr4, response);
}

/* 0x00200724: Get Status (0x00) */
static int get_current_status(void)
{
    return simple_command(0, 0x0000, 0, 0, 0);
}

/* 0x00200912: Initialize CD System (0x04), soft reset, default standby, ECC and retries */
static int reset_cd_block(void)
{
    for (volatile int32_t delay = 0x2904; delay > 0; delay--)
    {
    }

    hirq_seen |= HIRQ_ESEL | HIRQ_CMOK;
    int result = simple_command(HIRQ_ESEL, 0x0401, 0, 0, 0);

    if (result == 0)
    {
        CD_HIRQ = 0x0BE1;
        hirq_seen = 0x0BE1;
    }

    return result;
}

/* 0x00200AFA: Seek Disc (0x11) to the default position */
static void seek_default(void)
{
    simple_command(0, 0x1100, 0, 0, 0);
}

/* 0x00200A06: End Data Transfer (0x06), gives the number of words transferred */
static int end_transfer(int32_t *words)
{
    uint16_t response[4] = {0, 0, 0, 0};
    int result = command(0, 0x0600, 0, 0, 0, response);
    *words = ((response[0] & 0xFF) << 16) | response[1];
    CD_HIRQ = ~HIRQ_DRDY;
    return result;
}

/* 0x002009BC: waits for the data transfer to be ready */
static int wait_transfer(void)
{
    int result = wait_hirq_any(HIRQ_DRDY);

    if (result != 0)
    {
        int32_t words;
        end_transfer(&words);
    }

    CD_HIRQ = ~HIRQ_DRDY;
    return result;
}

/* 0x00201096: reads 16-bit words from the data port, after a command that transfers data */
static int read_words(int32_t count, uint16_t *destination)
{
    int result = wait_transfer();

    if (result != 0)
    {
        return result;
    }

    for (int32_t i = 0; i < count; i++)
    {
        *destination++ = CD_DATA16;
    }

    int32_t transferred;
    result = end_transfer(&transferred);
    return result == 0 && transferred != count ? -7 : result;
}

/* 0x00200BA6: Get Sector Number (0x51) of buffer partition 0 */
static int get_sector_number(int32_t *sectors)
{
    uint16_t response[4];
    int result = command(0, 0x5100, 0, 0, 0, response);
    *sectors = response[3];
    return result;
}

/* 0x00200DE2: Get Then Delete Sector Data (0x63) from buffer partition 0 */
static int get_delete_sectors(uint16_t sectors)
{
    return simple_command(HIRQ_EHST, 0x6300, 0, 0, sectors);
}

/* 0x00200EE8: Change Directory (0x70) */
static int change_directory(uint8_t filter, uint32_t fid)
{
    return simple_command(HIRQ_EFLS, 0x7000, 0, (filter << 8) | ((fid >> 16) & 0xFF), fid & 0xFFFF);
}

/* 0x00200F10: Read Directory (0x71) */
static int read_directory(uint8_t filter, uint32_t fid)
{
    return simple_command(HIRQ_EFLS, 0x7100, 0, (filter << 8) | ((fid >> 16) & 0xFF), fid & 0xFFFF);
}

/* File info returned by Get File Info: 6 words */
typedef struct
{
    uint32_t fad;
    uint32_t size;
    uint8_t unit, gap, number, attribute;
} FileInfo;

/* 0x00200F88: Get File Info (0x73) */
static int get_file_info(uint32_t fid, FileInfo *info)
{
    uint16_t response[4];
    int result = command(0, 0x7300, 0, (fid >> 16) & 0xFF, fid & 0xFFFF, response);
    return result != 0 ? result : read_words(((response[0] & 0xFF) << 16) | response[1], (uint16_t *)info);
}

/* 0x00200FD8: Read File (0x74) from offset 0 */
static int read_file(uint8_t filter, uint32_t fid)
{
    return simple_command(HIRQ_EFLS, 0x7400, 0, (filter << 8) | ((fid >> 16) & 0xFF), fid & 0xFFFF);
}

/* 0x00201002: Abort File (0x75) */
static void abort_file(void)
{
    hirq_seen |= HIRQ_EFLS;
    simple_command(HIRQ_EFLS, 0x7500, 0, 0, 0);
}

/* 0x00201046 (in 0x002005EE) and 0x002005B4: waits for one of the HIRQ bits, shorter timeout result */
static int wait_hirq(uint16_t bits)
{
    for (int32_t i = 0; i < 0x2C4020; i++)
    {
        if (CD_HIRQ & bits)
        {
            return 0;
        }
    }

    return -1;
}

/* 0x002007A8 and 0x00200764: status from the periodic report, read until two reads agree */
static int get_periodic_status(uint8_t *status)
{
    uint16_t first[4], second[4];

    for (int32_t tries = 0;; tries++)
    {
        if (tries >= 100)
        {
            return -3;
        }

        uint32_t sr = mask_interrupts();
        first[0] = CD_CR1, first[1] = CD_CR2, first[2] = CD_CR3, first[3] = CD_CR4;
        second[0] = CD_CR1, second[1] = CD_CR2, second[2] = CD_CR3, second[3] = CD_CR4;
        restore_interrupts(sr);

        if (first[0] == second[0] && first[1] == second[1] && first[2] == second[2] && first[3] == second[3])
        {
            break;
        }
    }

    *status = first[0] >> 8;
    return *status == 0xFF || !(*status & 0x20) ? -8 : 0;
}

/* 0x0020060A: drive status, fails when there is no usable disc */
static int get_status(int32_t *drive)
{
    uint8_t status;
    *drive = -1;

    for (int32_t tries = 0;; tries++)
    {
        int result = get_periodic_status(&status);

        if (result == 0)
        {
            break;
        }

        if (result != -8)
        {
            return -102;
        }

        if (tries > 90000)
        {
            return -103;
        }
    }

    *drive = status & 0x0F;

    if (*drive == STATUS_OPEN || *drive == STATUS_NODISC)
    {
        return -2;
    }

    if (*drive == STATUS_FATAL)
    {
        if (!(CD_HIRQ & HIRQ_DCHG))
        {
            seek_default();
        }

        return -2;
    }

    return CD_HIRQ & HIRQ_DCHG ? -2 : 0;
}

/* --- FLD --- */

/* 0x002004E6: whether the drive finished reading the file */
static int is_read_over(int32_t drive, uint16_t done)
{
    if (drive == STATUS_PAUSE || drive == STATUS_STANDBY)
    {
        return (CD_HIRQ & done) != 0;
    }

    return drive == STATUS_ERROR;
}

/* 0x00200522: moves sectors from the CD buffer to memory */
static int read_sectors(int32_t sectors, int32_t words, uint32_t *destination)
{
    for (int32_t tries = 0;; tries++)
    {
        int result = get_delete_sectors(sectors);

        if (result == 0)
        {
            break;
        }

        if (result != -1)
        {
            return -65;
        }

        if (tries > 0xA1220)
        {
            return -66;
        }
    }

    if (wait_transfer() != 0)
    {
        return -67;
    }

    for (int32_t i = 0; i < words; i++)
    {
        *destination++ = CD_DATA32;
    }

    int32_t transferred;
    return end_transfer(&transferred) != 0 ? -68 : 0;
}

/* 0x00200410: reads the file being played to memory, returns the bytes read */
static int read_data(uint16_t done, int32_t size, uint32_t *destination)
{
    int32_t left = (size + 3) >> 2;
    int32_t total = 0;
    int32_t idle = 0;

    while (left > 0)
    {
        int32_t drive, sectors;
        int result = get_status(&drive);

        if (result != 0)
        {
            return result;
        }

        if (get_sector_number(&sectors) != 0)
        {
            return -60;
        }

        if (sectors == 0)
        {
            if (is_read_over(drive, done))
            {
                break;
            }

            if (idle++ > 3000)
            {
                return -61;
            }

            continue;
        }

        idle = 0;
        int32_t words = sectors * SECTOR_WORDS < left ? sectors * SECTOR_WORDS : left;
        result = read_sectors(sectors, words, destination);

        if (result != 0)
        {
            return result;
        }

        destination += words;
        total += words;
        left -= words;
    }

    return total << 2;
}

/* 0x002003DC: the file must fit in high work RAM after the IP, or in low work RAM */
static int check_range(uint32_t address, int32_t size)
{
    uint32_t start = address & 0x1FFFFFFF;
    uint32_t end = start + size;

    if (end <= start)
    {
        return -79;
    }

    if ((start >= 0x06003000 && end <= 0x06100000) || (start >= 0x00200000 && end <= 0x00300000))
    {
        return 0;
    }

    return -79;
}

/* 0x0020033E: reads a file of the current directory (already started by read_file) */
static int load_file(uint32_t fid, uint32_t address)
{
    get_current_status();
    get_current_status();

    int32_t drive;
    int result = get_status(&drive);

    if (result != 0)
    {
        return result;
    }

    FileInfo info;

    if (get_file_info(fid, &info) != 0)
    {
        return -77;
    }

    result = check_range(address, info.size);

    if (result == 0)
    {
        result = read_data(HIRQ_EFLS, info.size, (uint32_t *)address);
    }

    abort_file();
    return result >= 0 && result < (int32_t)info.size ? -78 : result;
}

/* 0x002002F2: back to the root directory */
static int reset_directory(void)
{
    change_directory(23, ROOT_DIRECTORY);
    abort_file();

    if (wait_hirq(HIRQ_EFLS) != 0 || change_directory(0, ROOT_DIRECTORY) != 0 || wait_hirq(HIRQ_EFLS) != 0)
    {
        return -1;
    }

    return 0;
}

/* 0x00200208: resets the CD block, enters the game directory and starts reading the file */
static int start_reading(uint32_t directory, uint32_t fid)
{
    int32_t drive;
    int result = get_status(&drive);

    if (result != 0)
    {
        return result;
    }

    if (reset_cd_block() != 0 || wait_hirq(HIRQ_ESEL) != 0)
    {
        return -1;
    }

    result = reset_directory();

    if (result != 0)
    {
        return result;
    }

    if (read_directory(0, directory) != 0 || wait_hirq(HIRQ_EFLS) != 0)
    {
        return -1;
    }

    if (directory >= 2 && (change_directory(0, directory) != 0 || wait_hirq(HIRQ_EFLS) != 0))
    {
        return -1;
    }

    if (read_directory(0, fid) != 0 || wait_hirq(HIRQ_EFLS) != 0 || read_file(0, fid) != 0)
    {
        return -1;
    }

    return 0;
}

/* 0x00200700: VBlank-in handler: the IP's own one, without letting its logo counter go negative */
static void vblank_in(void)
{
    IP_VBLANK_IN();
    int32_t counter = IP_LOGO_COUNTER;
    IP_LOGO_COUNTER = counter < 0 ? 0 : counter;
}

/* 0x002001BE: loads the first file of the DDS directory */
static int load_first_file(void)
{
    uint32_t directory = DDS_DIRECTORY_ID;
    uint32_t address = BIOS_1ST_READ_ADDRESS;

    BIOS_SET_INTERRUPT(VBLANK_IN_VECTOR, vblank_in);

    int result = start_reading(directory, FIRST_FILE);

    if (result != 0)
    {
        return result;
    }

    return load_file(FIRST_FILE, address) < 0 ? -1 : 0;
}

/* 0x002001A4: kernel entry, called by the IP code instead of the BIOS loader */
int FLD_1stRead(void)
{
    if (load_first_file() != 0)
    {
        return BIOS_CD_PLAYER();
    }

    return 0;
}
