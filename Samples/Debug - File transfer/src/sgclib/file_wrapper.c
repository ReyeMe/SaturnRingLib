/**
 *  SD card access module for Saturn Gamer's Cartridge
 *  by cafe-alpha
 *
 *  See LICENSE file for details.
**/

#include "file_wrapper.h"

/* sgclib API and functions. */
#include "sgclib.h"

/* sgclib stub (binary format). */
#include "binwrap.h"

/* Text display, for debug. */
//#include "conio.h"


void file_load_and_init_stub(void)
{
    unsigned char* stub_ptr  = (unsigned char*)(&_sgclib_stub_dat);
    unsigned long  stub_size = (unsigned long )(&_sgclib_stub_end - &_sgclib_stub_dat);
    int row = 12;

    //conio_printf(2, row++, COLOR_YELLOW, "Stub ptr: %08X, size: %d", stub_ptr, stub_size);

    //conio_printf(2, row, COLOR_YELLOW, "Stub exec address: ");
    //conio_printf(21, row++, COLOR_WHITE, "%08X", SGCLIB_API);

    /* Load (copy) the stub to its execution area near end of LRAM.
     * After being loaded, data in &_sgclib_stub_dat array is no longer needed.
     */
    memcpy(SGCLIB_API, (void*)stub_ptr, stub_size);

    /* Display some debug stuff */
    //conio_printf(2, row++, COLOR_YELLOW, "API entry points :");
    //conio_printf
    // (
    //     2, row++, 
    //     COLOR_WHITE, 
    //     "%08X %08X %08X %08X", 
    //     SGCLIB_API->init, 
    //     SGCLIB_API->open, 
    //     SGCLIB_API->close, 
    //     SGCLIB_API->seek
    // );

    SGCLIB_API->init();
}

unsigned long file_get_size(char* filename)
{
    unsigned long ret = 0;

//    int fd = SGCLIB_API->open(filename, FA_READ);

//    if(fd >= 0)
//    {
//    }

    sgc_stat_t stat = { 0 };

    SGCLIB_API->stat(filename, &stat, sizeof(stat));

    ret = stat.size;

    return ret;
}

char* split_path_and_name(char* filename)
{
    /* Extract folder and file name from full path. */
    int len = strlen(filename);
    int path_len = 0;
    for(int i=0; i<len; i++)
    {
        char c = filename[i];
        if(c == '/')
        {
            path_len = i;
        }
    }

    /* Path set in input string. */
    filename[path_len] = '\0';

    /* Return file name. */
    char* name = filename + path_len+1;
    return name;
}

char _filename_buffer[256] = {'\0'};

unsigned long file_read(char* filename, unsigned long offset, unsigned long size, void* ptr)
{
    unsigned long retsize = 0;
    unsigned long file_size = file_get_size(filename);

    strcpy(_filename_buffer, filename);
    char* file = split_path_and_name(_filename_buffer);
    char* folder = _filename_buffer;
    ////conio_printf(2, 10, COLOR_WHITE, "folder:\"%s\"", folder);
    ////conio_printf(2, 11, COLOR_WHITE, "file  :\"%s\"", file);

    SGCLIB_API->chdir(folder);

    if(offset > file_size)
    {
        offset = 0;
    }
    unsigned long read_size = size;
    if((offset + read_size) > file_size)
    {
        read_size = file_size - offset;
    }


    if(read_size)
    {
        int fd = SGCLIB_API->open(file, FA_READ);

        //scd_logout("[SATIS]read(%s) fd:%d", file, fd);

        if(fd >= 0)
        {
            SGCLIB_API->seek(fd, offset, C_SEEK_SET);

            int sret = SGCLIB_API->read(fd, ptr, read_size);
            if(sret > 0)
            {
                retsize = sret;
            }

            //scd_logout("[SATIS]read(OFS:%d, LEN:%d) ret:%d", read_offset, chunk_len, sret);

            SGCLIB_API->close(fd);
        }
    }

    return retsize;
}


unsigned long file_write(void* ptr, unsigned long size, char* filename)
{
    unsigned long retsize = 0;

    strcpy(_filename_buffer, filename);
    char* file = split_path_and_name(_filename_buffer);
    char* folder = _filename_buffer;

    SGCLIB_API->chdir(folder);

    /* FatFs flags memo :
     *
     *  FA_READ             Specifies read access to the object. Data can be read from the file. Combine with FA_WRITE for read-write access.
     *  FA_WRITE            Specifies write access to the object. Data can be written to the file. Combine with FA_READ for read-write access.
     *  FA_OPEN_EXISTING    Opens the file. The function fails if the file is not existing. (Default)
     *  FA_OPEN_ALWAYS      Opens the file if it is existing. If not, a new file is created.
     *                      To append data to the file, use f_lseek function after file open in this method.
     *  FA_CREATE_NEW       Creates a new file. The function fails with FR_EXIST if the file is existing.
     *  FA_CREATE_ALWAYS    Creates a new file. If the file is existing, it is truncated and overwritten.
     */
    int fd = SGCLIB_API->open(file, FA_WRITE|FA_CREATE_ALWAYS);

    if(fd >= 0)
    {
        int sret = SGCLIB_API->write(fd, ptr, size);
        if(sret > 0)
        {
            retsize = sret;
        }

        SGCLIB_API->close(fd);
    }

    return retsize;
}

