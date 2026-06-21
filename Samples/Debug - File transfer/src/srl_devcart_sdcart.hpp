#pragma once
#include <srl_devcart.hpp>

#include "sgclib/sgclib.h"
#include "fatfs/ff.h"

#include <cstdint> // For uintptr_t, size_t, uint8_t, uint32_t
#include <string.h>
#include <initializer_list>
#include <srl_register.hpp>

/**
 * @brief Namespace for interacting with a USB development cartridge for the
 * Sega Saturn.
 *
 * This provides access to registers for USB communication, SD card access, and
 * other hardware features.
 */
namespace SRL
{
    namespace DevCart
    {
        namespace SD
        {
           

            constexpr uintptr_t kSgclibBaseAddress = 0x060BA000UL;
            constexpr uintptr_t kSgclibDirOffset = 0x4F00UL;
            constexpr uintptr_t kSgclibFReaddirOffset = 0x3B80UL;
            constexpr size_t kMaxPathBytes = 255;

            typedef int (*Fct_sgc_init)(void);
            typedef int (*Fct_sgc_open)(const char *filename, int flags);
            typedef int (*Fct_sgc_close)(int fd);
            typedef int (*Fct_sgc_read)(int fd, void *buf, int len);
            typedef int (*Fct_sgc_write)(int fd, const void *buf, int len);
            typedef int (*Fct_sgc_stat)(const char *filename, sgc_stat_t *stat, int statsize);
            typedef int (*Fct_sgc_unlink)(const char *filename);
            typedef int (*Fct_sgc_opendir)(const char *path);
            typedef int (*Fct_sgc_chdir)(const char *path);
            typedef int (*Fct_sgc_getcwd)(char *buff, int buflen);

            typedef struct _sgclib_api_t
            {
                Fct_sgc_init init;
                Fct_sgc_open open;
                Fct_sgc_close close;
                Fct_sgc_read read;
                Fct_sgc_write write;
                Fct_sgc_stat stat;
                Fct_sgc_unlink unlink;
                Fct_sgc_opendir opendir;
                Fct_sgc_chdir chdir;
                Fct_sgc_getcwd getcwd;
            } __attribute__((packed)) sgclib_api_t;

            /** @brief Returns the SGCLIB API pointer. */
            static inline sgclib_api_t *sgclib_api()
            {
                return reinterpret_cast<sgclib_api_t*>(kSgclibBaseAddress);
            }

            using HiddenFReaddir = int (*)(DIR *dp, FILINFO *fno);

            HiddenFReaddir GetHiddenFReaddir()
            {
                return reinterpret_cast<HiddenFReaddir>(kSgclibBaseAddress +
                                                        kSgclibFReaddirOffset);
            }

            DIR *GetHiddenDirObject()
            {
                return reinterpret_cast<DIR *>(kSgclibBaseAddress + kSgclibDirOffset);
            }

            /** @brief Loads the SGCLIB binary stub into High Work RAM. */
            static inline void fs_load_stub(const void *stubPtr, size_t stubSize)
            {
                memcpy(sgclib_api(), stubPtr, stubSize);
            }

            /** @brief Initializes the SGCLIB FAT driver. Returns SGC_FR_OK on success. */
            static inline int fs_init()
            {
                return SGCLIB_API->init();
            }

            /** @brief Gets current working directory. */
            static inline int fs_getcwd(char *buff, int buflen)
            {
                return sgclib_api()->getcwd(buff, buflen);
            }

            /** @brief Changes current working directory. */
            static inline int fs_chdir(const char *path)
            {
                return sgclib_api()->chdir(path);
            }

            /** @brief Opens a file. Returns file descriptor or -1 on error. */
            static inline int fs_open(const char *filename, int flags)
            {
                return sgclib_api()->open(filename, flags);
            }

            /** @brief Closes a file descriptor. */
            static inline int fs_close(int fd)
            {
                return sgclib_api()->close(fd);
            }

            /** @brief Reads from an open file. Returns bytes read, 0 at EOF, <0 on error. */
            static inline int fs_read(int fd, void *buf, int len)
            {
                return sgclib_api()->read(fd, buf, len);
            }

            /** @brief Writes to an open file. Returns bytes written, <0 on error. */
            static inline int fs_write(int fd, const void *buf, int len)
            {
                return sgclib_api()->write(fd, buf, len);
            }

            /** @brief Gets file/directory metadata. Returns SGC_FR_OK on success. */
            static inline int fs_stat(const char *filename, sgc_stat_t *stat)
            {
                return sgclib_api()->stat(filename, stat, static_cast<int>(sizeof(sgc_stat_t)));
            }

            /** @brief Opens a directory for listing. Returns SGC_FR_OK on success. */
            static inline int fs_opendir(const char *path)
            {
                return sgclib_api()->opendir(path);
            }

            /** @brief Removes a file. Returns SGC_FR_OK on success. */
            static inline int fs_unlink(const char *path)
            {
                return sgclib_api()->unlink(path);
            }

            /** @brief Reads a directory entry. Returns SGC_FR_OK on success. */
            static inline int fs_readdir(FILINFO *entry)
            {
                HiddenFReaddir hiddenReaddir = GetHiddenFReaddir();
                DIR *hiddenDir = GetHiddenDirObject();
                if (hiddenReaddir == NULL || hiddenDir == NULL)
                {
                    return SGC_FR_DISK_ERR;
                }

                return hiddenReaddir(hiddenDir, entry);
            }

            /**
             * @brief Bit shifts for SD card LED and switch controls in registers (e.g.,
             * LED_SETTING).
             */
            enum class SD_LSHFT : uint8_t
            {
                SD_LEDG_LSHFT = 0, // Green LED bit position
                SD_LEDR_LSHFT = 1, // Red LED bit position (typo fix: SD_LEDR_LSHFT?)
                SD_SW1_LSHFT = 4,  // Switch 1 bit
                SD_EJECT_LSHFT = 7 // Eject detect bit
            };

            /**
             * @brief Bit shifts for SD card control signals (CS, DIN, CLK) in control
             * registers.
             */
            enum class SD_CTRL_LSHFT : uint8_t
            {
                SD_CSL_LSHFT = 0, // Chip Select low-active?
                SD_DIN_LSHFT = 1, // Data In
                SD_CLK_LSHFT = 2  // Clock signal
            };

            /**
             * @brief Returns SD write-protect/no-card status on USB Gamer's cartridge.
             *
             * 0: write enabled
             * 1: write protected or no SD card
             */
            static inline bool IsSdWriteProtectedOrMissing()
            {
                return (CS1::ReadRegister(CS1::Register::REG_SD_WRITE_PROTECT) & 0x01U) != 0;
            }

            /**
             * @brief Returns whether USB data path is expected to be enabled.
             *
             * On USB Gamer's cartridge, USB may be disabled when SD is write-protected
             * or missing. On other variants this returns true.
             */
            static inline bool IsUsbDataPathEnabled()
            {
                if (!CS1::IsUsbGamersCartridge())
                {
                    return true;
                }
                return !IsSdWriteProtectedOrMissing();
            }

            // Common SD Card Commands
            constexpr uint32_t BLOCK_SIZE = 512;
            constexpr uint16_t CMD_GO_IDLE_STATE = 0;
            constexpr uint16_t CMD_WRITE_SINGLE_BLOCK = 24;
            constexpr uint16_t CMD_WRITE_MULTIPLE_BLOCK = 25;
            constexpr uint16_t CMD_STOP_TRANSMISSION = 12;
            // A simple File handle struct
            struct FileHandle
            {
                uint32_t start_sector;
                uint32_t current_sector;
                uint32_t bytes_written;
                uint32_t file_size;
                bool is_open;
            };
            /**
             * @brief Send a low-level command to the SD Card via the DevCart
             * CPLD/Registers.
             */
            inline void send_sd_command(uint16_t cmd, uint32_t arg)
            {
                // 1. Wait for SD card to be ready (poll Auxiliary Status Register)
                while ((*(volatile uint16_t *)CS0::SDCardRegisters::CART_ASR) &
                       0x01)
                { /* busy wait */
                }

                // 2. Write the 32-bit argument
                *(volatile uint32_t *)CS0::SDCardRegisters::CART_CMD_ARG = arg;

                // 3. Send the command index
                *(volatile uint16_t *)CS0::SDCardRegisters::CART_CMD = cmd;
            }
            /**
             * @brief Wait for the SD Card to complete its current operation.
             */
            inline bool wait_sd_ready()
            {
                // Poll status register until ready bit is set
                uint32_t timeout = 0xFFFFF;
                while ((*(volatile uint32_t *)CS0::SDCardRegisters::CART_SR) &
                       0x00000100 /* Example busy bit */)
                {
                    if (--timeout == 0)
                        return false;
                }
                return true;
            }
            /**
             * @brief Opens a "file" on the SD Card.
             * Since we aren't using a FAT filesystem, we write to raw sectors.
             * @param handle The file handle to initialize
             * @param raw_sector_start The absolute sector on the SD card to start writing
             * to
             * @param size Total file size
             * @return true if successful
             */
            inline bool open(FileHandle &handle, uint32_t raw_sector_start, uint32_t size)
            {
                // Check if SD card is present and not write protected
                if (IsSdWriteProtectedOrMissing())
                {
                    return false;
                }
                handle.start_sector = raw_sector_start;
                handle.current_sector = raw_sector_start;
                handle.file_size = size;
                handle.bytes_written = 0;
                handle.is_open = true;
                // Optionally send an init command or CMD25 (Write Multiple Block) here
                return true;
            }
            /**
             * @brief Writes a buffer of data to the SD Card.
             * Buffers the data until a full 512-byte sector is ready, then commits to SD.
             */
            inline bool write(FileHandle &handle, const uint8_t *buffer, uint32_t length)
            {
                if (!handle.is_open)
                    return false;
                // Note: A true implementation needs a 512-byte static buffer here.
                // Once 512 bytes are accumulated, you execute CMD24 (Write Single Block)
                // For simplicity, assuming length == 512 for block writes:

                if (length == 512)
                {
                    send_sd_command(CMD_WRITE_SINGLE_BLOCK, handle.current_sector);
                    wait_sd_ready();
                    // Write the 512 bytes into the Data Port (usually a shared buffer register)
                    // volatile uint32_t* sd_data_port = (volatile uint32_t*)SOME_DATA_REGISTER;
                    // for(int i=0; i<128; i++) {
                    //     sd_data_port[i] = ((uint32_t*)buffer)[i];
                    // }
                    wait_sd_ready();
                    handle.current_sector++;
                    handle.bytes_written += 512;
                }
                return true;
            }
            /**
             * @brief Closes the file handle and finalizes SD card writes.
             */
            inline void close(FileHandle &handle)
            {
                if (!handle.is_open)
                    return;

                // If we were using CMD25 (Multi-block write), send CMD12 to stop transmission
                // send_sd_command(CMD_STOP_TRANSMISSION, 0);
                // wait_sd_ready();
                handle.is_open = false;
            }

            /**
             * @brief Helpers for writable DevCart SD raw paths.
             *
             * Supported path format:
             * - Root/capability listing: sdraw:/
             * - Raw range target: sdraw:<start_sector>:<sector_count>
             */
            namespace Backend
            {
                static inline bool MatchLiteral(const char *input, const char *literal)
                {
                    if (input == nullptr || literal == nullptr)
                    {
                        return false;
                    }

                    while (*literal != '\0')
                    {
                        if (*input != *literal)
                        {
                            return false;
                        }
                        ++input;
                        ++literal;
                    }

                    return true;
                }

                static inline int HexDigitValue(const char c)
                {
                    if (c >= '0' && c <= '9')
                        return c - '0';
                    if (c >= 'a' && c <= 'f')
                        return 10 + (c - 'a');
                    if (c >= 'A' && c <= 'F')
                        return 10 + (c - 'A');
                    return -1;
                }

                static inline bool TryParseU32(const char *begin,
                                               const char **end,
                                               uint32_t &value)
                {
                    if (begin == nullptr || end == nullptr)
                    {
                        return false;
                    }

                    int base = 10;
                    const char *cursor = begin;
                    if (cursor[0] == '0' && (cursor[1] == 'x' || cursor[1] == 'X'))
                    {
                        base = 16;
                        cursor += 2;
                    }

                    uint32_t parsed = 0;
                    bool consumed = false;
                    while (*cursor != '\0')
                    {
                        int digit = (base == 16) ? HexDigitValue(*cursor) : ((*cursor >= '0' && *cursor <= '9') ? (*cursor - '0') : -1);
                        if (digit < 0)
                        {
                            break;
                        }

                        if (parsed > ((0xFFFFFFFFUL - static_cast<uint32_t>(digit)) /
                                      static_cast<uint32_t>(base)))
                        {
                            return false;
                        }

                        parsed = (parsed * static_cast<uint32_t>(base)) +
                                 static_cast<uint32_t>(digit);
                        consumed = true;
                        ++cursor;
                    }

                    if (!consumed)
                    {
                        return false;
                    }

                    value = parsed;
                    *end = cursor;
                    return true;
                }

                static inline bool IsRawPath(const char *path)
                {
                    return MatchLiteral(path, "sdraw:");
                }

                static inline bool IsRawRootPath(const char *path)
                {
                    if (path == nullptr)
                    {
                        return false;
                    }
                    return path[0] == 's' && path[1] == 'd' && path[2] == 'r' &&
                           path[3] == 'a' && path[4] == 'w' && path[5] == ':' &&
                           path[6] == '/' && path[7] == '\0';
                }

                static inline bool TryParseRawRange(const char *path,
                                                    uint32_t &startSector,
                                                    uint32_t &sectorCount)
                {
                    if (!IsRawPath(path) || IsRawRootPath(path))
                    {
                        return false;
                    }

                    const char *cursor = path + 6;
                    const char *end = nullptr;
                    uint32_t parsedStart = 0;
                    if (!TryParseU32(cursor, &end, parsedStart) || end == nullptr || *end != ':')
                    {
                        return false;
                    }

                    cursor = end + 1;
                    uint32_t parsedCount = 0;
                    if (!TryParseU32(cursor, &end, parsedCount) || end == nullptr || *end != '\0')
                    {
                        return false;
                    }

                    startSector = parsedStart;
                    sectorCount = parsedCount;
                    return sectorCount > 0;
                }

                static inline bool EraseRawRange(uint32_t startSector, uint32_t sectorCount)
                {
                    if (sectorCount == 0)
                    {
                        return false;
                    }

                    FileHandle handle{};
                    if (!open(handle, startSector, sectorCount * BLOCK_SIZE))
                    {
                        return false;
                    }

                    uint8_t zeroBlock[BLOCK_SIZE] = {0};
                    for (uint32_t i = 0; i < sectorCount; ++i)
                    {
                        if (!write(handle, zeroBlock, BLOCK_SIZE))
                        {
                            close(handle);
                            return false;
                        }
                    }

                    close(handle);
                    return true;
                }
            } // namespace Backend
        } // namespace SD
    } // namespace DevCart
} // namespace SRL