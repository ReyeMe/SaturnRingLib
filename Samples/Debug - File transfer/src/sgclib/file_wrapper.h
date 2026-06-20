/**
 *  SD card access module for Saturn Gamer's Cartridge
 *  by cafe-alpha
 *
 *  See LICENSE file for details.
**/

#ifndef _FILE_WRAPPER_
#define _FILE_WRAPPER_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>


void file_load_and_init_stub(void);

unsigned long file_get_size(char* filename);

unsigned long file_read(char* filename, unsigned long offset, unsigned long size, void* ptr);
unsigned long file_write(void* ptr, unsigned long size, char* filename);


#endif /* _FILE_WRAPPER_ */
