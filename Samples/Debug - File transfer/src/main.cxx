#include <srl.hpp>
#include <srl_devcart.hpp>
#include "srl_devcart_sdcart.hpp"

#include <cstddef>
#include <cstdint>
#include <stdio.h>
#include <string.h>

using namespace SRL::Types;

namespace
{
  constexpr size_t kMaxRequestBytes = 255;
  constexpr size_t kMaxResponseBytes = 1023;
  constexpr size_t kMaxDirEntries = 96;

  bool g_sgcReady = false;
  bool g_sgcStubLoaded = false;
  FATFS g_fatfs;

  constexpr size_t kMaxPathBytes = 255;

#define APPEND_FMT(_BUF_, _CAP_, _USED_, _FMT_, ...)                        \
  do                                                                         \
  {                                                                          \
    if ((_USED_) < (_CAP_))                                                  \
    {                                                                        \
      const int _written = snprintf((_BUF_) + (_USED_),                     \
                                    (_CAP_) - (_USED_),                      \
                                    (_FMT_), ##__VA_ARGS__);                 \
      if (_written > 0)                                                      \
      {                                                                      \
        const size_t _advance = static_cast<size_t>(_written);               \
        (_USED_) = ((_USED_) + _advance >= (_CAP_)) ?                        \
                      (_CAP_) :                                               \
                      ((_USED_) + _advance);                                 \
      }                                                                      \
    }                                                                        \
  } while (0)

  /**
   * @brief Calculates or updates a CRC-8 checksum over a buffer of data.
   * @param crc The initial CRC value, or the previous CRC for chained updates.
   * @param data Pointer to the buffer of data to process.
   * @param dataLen Length of the data buffer in bytes.
   * @return The updated CRC-8 checksum.
   */
  uint8_t Crc8Update(uint8_t crc, const uint8_t *data, size_t dataLen)
  {
    static const uint8_t crcTable[256] = {
        0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31,
        0x24, 0x23, 0x2A, 0x2D, 0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
        0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D, 0xE0, 0xE7, 0xEE, 0xE9,
        0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
        0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1,
        0xB4, 0xB3, 0xBA, 0xBD, 0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
        0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA, 0xB7, 0xB0, 0xB9, 0xBE,
        0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
        0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16,
        0x03, 0x04, 0x0D, 0x0A, 0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
        0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A, 0x89, 0x8E, 0x87, 0x80,
        0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
        0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8,
        0xDD, 0xDA, 0xD3, 0xD4, 0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
        0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44, 0x19, 0x1E, 0x17, 0x10,
        0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
        0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F,
        0x6A, 0x6D, 0x64, 0x63, 0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
        0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13, 0xAE, 0xA9, 0xA0, 0xA7,
        0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
        0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF,
        0xFA, 0xFD, 0xF4, 0xF3};

    while (dataLen-- != 0)
    {
      const uint32_t index = static_cast<uint32_t>(crc ^ *data++) & 0xFFU;
      crc = crcTable[index];
    }
    return crc;
  }

  extern "C"
  {
    extern unsigned char __sgclib_stub_dat;
    extern unsigned char __sgclib_stub_end;
  }

  /**
   * @brief Ensures that the SGCLIB (SD Cart) stub is loaded into memory.
   * 
   * The SGCLIB stub provides low-level SD card access routines.
   */
  void EnsureSgclibStubLoaded()
  {
    if (g_sgcStubLoaded)
    {
      return;
    }

    const unsigned char *stubPtr = &__sgclib_stub_dat;
    const size_t stubSize = static_cast<size_t>(&__sgclib_stub_end - &__sgclib_stub_dat);
    SRL::DevCart::SD::fs_load_stub(stubPtr, stubSize);
    g_sgcStubLoaded = true;
  }

  /**
   * @brief Ensures that the SGCLIB and FAT file system are initialized and mounted.
   * @return True if the SD card is successfully mounted; false otherwise.
   */
  bool EnsureSgclibReady()
  {
    if (g_sgcReady)
    {
      return true;
    }

    EnsureSgclibStubLoaded();
    const int initRes = SRL::DevCart::SD::fs_init();
    if (initRes != 0) {
      return false;
    }

    const FRESULT mountRes = f_mount(&g_fatfs, "", 1);
    g_sgcReady = (mountRes == FR_OK);
    return g_sgcReady;
  }

  /**
   * @brief Checks if a path is intended for the SD card FAT file system.
   * @param path The path to check.
   * @return True if the path starts with a slash ('/'); false otherwise.
   */
  bool IsSdFsPath(const char *path)
  {
    return path != NULL && path[0] == '/';
  }

  /**
   * @brief Normalizes an SD FAT file system path.
   * @param path The path to normalize.
   * @return A pointer to the normalized path string.
   */
  const char *NormalizeSdFsPath(const char *path)
  {
    return path;
  }

  /**
   * @brief Saves the current FAT file system working directory to a buffer.
   * @param buffer Pointer to the buffer where the current directory will be saved.
   * @param size Size of the buffer in bytes.
   */
  void SaveCurrentDir(char *buffer, size_t size)
  {
    if (buffer == NULL || size == 0)
    {
      return;
    }

    buffer[0] = '\0';
    f_getcwd(buffer, static_cast<UINT>(size - 1));
    buffer[size - 1] = '\0';
  }

  /**
   * @brief Restores the FAT file system working directory from a buffer.
   * @param buffer Pointer to the buffer containing the directory path to restore.
   */
  void RestoreCurrentDir(const char *buffer)
  {
    if (buffer != NULL && buffer[0] != '\0')
    {
      f_chdir(buffer);
    }
  }

  /**
   * @brief Splits a full path into directory path and file name.
   * 
   * Modifies the input string by replacing the last slash with a null terminator.
   * 
   * @param fullPath The full path string to split (will be modified).
   * @return A pointer to the file name component.
   */
  char *SplitPathAndName(char *fullPath)
  {
    int slash = -1;
    for (int i = 0; fullPath[i] != '\0'; ++i)
    {
      if (fullPath[i] == '/')
      {
        slash = i;
      }
    }

    if (slash < 0)
    {
      return fullPath;
    }

    fullPath[slash] = '\0';
    return fullPath + slash + 1;
  }

  /**
   * @brief Placeholder function for opening an SD file for reading.
   * 
   * Currently simplified for testing purposes.
   * 
   * @param path The path of the file to open.
   * @param fd Reference to store the file descriptor.
   * @return True if successful; false otherwise.
   */
  bool OpenSdFileRead(const char *path, int &fd)
  {
    fd = -1;
    char cwd[kMaxPathBytes + 1] = {'\0'};
    char pathCopy[kMaxPathBytes + 1] = {'\0'};
    strncpy(pathCopy, path, kMaxPathBytes);
    pathCopy[kMaxPathBytes] = '\0';

    SaveCurrentDir(cwd, sizeof(cwd));
    char *name = SplitPathAndName(pathCopy);

    if (pathCopy[0] != '\0')
    {
      f_chdir(pathCopy);
    }

    // Note: Assuming fd logic requires custom mapping or simplified handle access
    fd = 0; // Simplified for placeholder
    return true; 
  }

  /**
   * @brief Handles a request from the host to list directory contents.
   * 
   * Supports SD card FAT filesystem paths and SD raw sector ranges.
   * 
   * @param path The path or raw sector range to list.
   * @param response Buffer to write the response string into.
   * @param responseLen Reference to the length of the written response string.
   * @return The status of the operation (e.g., Ok, BadRequest, Error).
   */
  SRL::DevCart::HostIo::Status HandleList(const char *path, char *response,
                                          size_t &responseLen)
  {
    bool detailed = false;
    if (strncmp(path, "-l ", 3) == 0)
    {
      detailed = true;
      path += 3;
    }
    if (SRL::DevCart::SD::Backend::IsRawPath(path))
    {
      if (SRL::DevCart::SD::Backend::IsRawRootPath(path))
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoList] sdraw:/ has no directory root.\n"
                   "Use sdraw:<start_sector>:<sector_count> for raw sector access.\n");
        return SRL::DevCart::HostIo::Status::Ok;
      }

      uint32_t startSector = 0;
      uint32_t sectorCount = 0;
      if (!SRL::DevCart::SD::Backend::TryParseRawRange(path, startSector, sectorCount))
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoList] Invalid SD raw path: %s\n", path);
        return SRL::DevCart::HostIo::Status::BadRequest;
      }

      APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                 "[DoList] %s [W RAW RANGE]\n", path);
      return SRL::DevCart::HostIo::Status::Ok;
    }

    if (IsSdFsPath(path))
    {
      if (!EnsureSgclibReady())
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoList] SGCLIB init failed\n");
        return SRL::DevCart::HostIo::Status::Error;
      }

      const char *sgclibPath = NormalizeSdFsPath(path);
      FILINFO stat;
      const FRESULT statRes = f_stat(sgclibPath, &stat);
      if (statRes == FR_OK && (stat.fattrib & AM_DIR) == 0)
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoList] %s [F]\n", path);
        return SRL::DevCart::HostIo::Status::Ok;
      }

      DIR dir;
      const FRESULT openDirRes = f_opendir(&dir, sgclibPath);
      if (openDirRes != FR_OK)
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoList] Can't open SD path (opendir=%d, stat=%d): %s\n", openDirRes, statRes, path);
        return SRL::DevCart::HostIo::Status::Error;
      }

      APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                 "[DoList] Listing: %s\n", path);

      bool any = false;
      for (size_t i = 0; i < kMaxDirEntries; ++i)
      {
        FILINFO entry{};
        const FRESULT rc = f_readdir(&dir, &entry);
        if (rc != FR_OK || entry.fname[0] == '\0')
        {
          break;
        }

        any = true;
        if (detailed)
        {
          APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                     "  %s %10lu %04u-%02u-%02u %02u:%02u %s\n",
                     (entry.fattrib & AM_DIR) ? "[D]" : "[F]",
                     (unsigned long)entry.fsize,
                     (entry.fdate >> 9) + 1980, (entry.fdate >> 5) & 15, entry.fdate & 31,
                     (entry.ftime >> 11), (entry.ftime >> 5) & 63,
                     entry.fname);
        }
        else
        {
          APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                     "  %s %s\n", (entry.fattrib & AM_DIR) ? "[D]" : "[F]",
                     entry.fname);
        }
      }

      if (!any)
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "  (empty directory)\n");
      }

      return SRL::DevCart::HostIo::Status::Ok;
    }

    APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
               "[DoList] Unsupported path scheme: %s\n"
               "Use /... for SD FAT or sdraw:... for raw sectors\n",
               path);
    return SRL::DevCart::HostIo::Status::BadRequest;
  }

  /**
   * @brief Handles a request from the host to remove a file or erase a raw sector range.
   * 
   * @param path The path of the file to remove, or the raw sector range to erase.
   * @param response Buffer to write the response string into.
   * @param responseLen Reference to the length of the written response string.
   * @return The status of the operation (e.g., Ok, BadRequest, Error).
   */
  SRL::DevCart::HostIo::Status HandleRemove(const char *path, char *response,
                                            size_t &responseLen)
  {
    if (SRL::DevCart::SD::Backend::IsRawPath(path))
    {
      uint32_t startSector = 0;
      uint32_t sectorCount = 0;
      if (!SRL::DevCart::SD::Backend::TryParseRawRange(path, startSector, sectorCount))
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoRemove] Invalid SD raw path: %s\n", path);
        return SRL::DevCart::HostIo::Status::BadRequest;
      }

      if (!SRL::DevCart::SD::Backend::EraseRawRange(startSector, sectorCount))
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoRemove] SD erase failed: %s\n", path);
        return SRL::DevCart::HostIo::Status::Error;
      }

      APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                 "[DoRemove] Erased SD raw range: %s\n", path);
      return SRL::DevCart::HostIo::Status::Ok;
    }

    if (IsSdFsPath(path))
    {
      if (!EnsureSgclibReady())
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoRemove] SGCLIB init failed\n");
        return SRL::DevCart::HostIo::Status::Error;
      }

      const FRESULT rc = f_unlink(path);
      if (rc != FR_OK)
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoRemove] Unlink failed (%d): %s\n", rc, path);
        return SRL::DevCart::HostIo::Status::Error;
      }

      APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                 "[DoRemove] Removed: %s\n", path);
      return SRL::DevCart::HostIo::Status::Ok;
    }

    APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
               "[DoRemove] Read-only or unsupported path: %s\n", path);
    return SRL::DevCart::HostIo::Status::Unsupported;
  }

  /**
   * @brief Handles a request from the host to create a directory.
   * 
   * @param path The path of the directory to create.
   * @param response Buffer to write the response string into.
   * @param responseLen Reference to the length of the written response string.
   * @return The status of the operation (e.g., Ok, BadRequest, Error).
   */
  SRL::DevCart::HostIo::Status HandleMkdir(const char *path, char *response,
                                            size_t &responseLen)
  {
    if (IsSdFsPath(path))
    {
      if (!EnsureSgclibReady())
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoMkdir] SGCLIB init failed\n");
        return SRL::DevCart::HostIo::Status::Error;
      }

      const FRESULT rc = f_mkdir(path);
      if (rc != FR_OK)
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoMkdir] Mkdir failed (%d): %s\n", rc, path);
        return SRL::DevCart::HostIo::Status::Error;
      }

      APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                 "[DoMkdir] Created directory: %s\n", path);
      return SRL::DevCart::HostIo::Status::Ok;
    }

    APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
               "[DoMkdir] Read-only or unsupported path: %s\n", path);
    return SRL::DevCart::HostIo::Status::Unsupported;
  }

  /**
   * @brief Handles a request from the host to remove a directory.
   * 
   * @param path The path of the directory to remove.
   * @param response Buffer to write the response string into.
   * @param responseLen Reference to the length of the written response string.
   * @return The status of the operation (e.g., Ok, BadRequest, Error).
   */
  SRL::DevCart::HostIo::Status HandleRmdir(const char *path, char *response,
                                            size_t &responseLen)
  {
    if (IsSdFsPath(path))
    {
      if (!EnsureSgclibReady())
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoRmdir] SGCLIB init failed\n");
        return SRL::DevCart::HostIo::Status::Error;
      }

      const FRESULT rc = f_unlink(path);
      if (rc != FR_OK)
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoRmdir] Rmdir failed (%d): %s\n", rc, path);
        return SRL::DevCart::HostIo::Status::Error;
      }

      APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                 "[DoRmdir] Removed directory: %s\n", path);
      return SRL::DevCart::HostIo::Status::Ok;
    }

    APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
               "[DoRmdir] Read-only or unsupported path: %s\n", path);
    return SRL::DevCart::HostIo::Status::Unsupported;
  }

  /**
   * @brief Handles a request from the host to calculate the CRC-8 checksum of a file.
   * 
   * Supports both SD FAT files and standard CD/host filesystem files.
   * 
   * @param path The path of the file to checksum.
   * @param response Buffer to write the response string into.
   * @param responseLen Reference to the length of the written response string.
   * @return The status of the operation (e.g., Ok, Error).
   */
  SRL::DevCart::HostIo::Status HandleCrc(const char *path, char *response,
                                         size_t &responseLen)
  {
    if (IsSdFsPath(path))
    {
      if (!EnsureSgclibReady())
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoCrc] SGCLIB init failed\n");
        return SRL::DevCart::HostIo::Status::Error;
      }

      FIL file;
      const FRESULT openRes = f_open(&file, path, FA_READ);
      if (openRes != FR_OK)
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoCrc] Can't open file '%s'\n", path);
        return SRL::DevCart::HostIo::Status::Error;
      }

      uint8_t buffer[1024];
      uint8_t checksum = 0;
      while (true)
      {
        UINT bytesRead = 0;
        const FRESULT readRes = f_read(&file, buffer, sizeof(buffer), &bytesRead);
        if (readRes != FR_OK)
        {
          f_close(&file);
          APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                     "[DoCrc] File read error.\n");
          return SRL::DevCart::HostIo::Status::Error;
        }
        if (bytesRead == 0)
        {
          break;
        }
        checksum = Crc8Update(checksum, buffer, static_cast<size_t>(bytesRead));
      }

      f_close(&file);
      APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                 "[DoCrc] %s CRC-8 = 0x%02X\n", path,
                 static_cast<unsigned int>(checksum));
      return SRL::DevCart::HostIo::Status::Ok;
    }

    SRL::Cd::File file(path);
    if (!file.Exists() || !file.Open())
    {
      APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                 "[DoCrc] Can't open file '%s'\n", path);
      return SRL::DevCart::HostIo::Status::Error;
    }

    uint8_t buffer[1024];
    uint8_t checksum = 0;
    while (true)
    {
      const int32_t read = file.Read(static_cast<int32_t>(sizeof(buffer)), buffer);
      if (read < 0)
      {
        file.Close();
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoCrc] File read error.\n");
        return SRL::DevCart::HostIo::Status::Error;
      }
      if (read == 0)
      {
        break;
      }
      checksum = Crc8Update(checksum, buffer, static_cast<size_t>(read));
    }

    file.Close();
    APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
               "[DoCrc] %s CRC-8 = 0x%02X\n", path,
               static_cast<unsigned int>(checksum));
    return SRL::DevCart::HostIo::Status::Ok;
  }

  /**
   * @brief Handles a request from the host to upload a file to the SD card.
   * 
   * The upload protocol consists of:
   * 1. Receiving a 4-byte file size (Big Endian).
   * 2. Receiving the file data in chunks.
   * 3. Receiving a 1-byte CRC-8 checksum from the host.
   * 4. Sending a 1-byte result code (0x00 for success, 0x01 for failure).
   * 
   * @param path The destination path on the SD card FAT filesystem.
   * @param response Buffer to write the response string into (used for errors).
   * @param responseLen Reference to the length of the written response string.
   * @return The status of the operation, typically Handled as the response is manual.
   */
  SRL::DevCart::HostIo::Status HandleUpload(const char *path, char *response,
                                            size_t &responseLen)
  {
    if (IsSdFsPath(path))
    {
      if (!EnsureSgclibReady())
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoUpload] SGCLIB init failed\n");
        return SRL::DevCart::HostIo::Status::Error;
      }

      FIL file;
      const FRESULT openRes = f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS);
      if (openRes != FR_OK)
      {
        APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                   "[DoUpload] Can't open file '%s' for writing\n", path);
        return SRL::DevCart::HostIo::Status::Error;
      }

      // Tell host we are ready
      SRL::DevCart::HostIo::SendResponse(SRL::DevCart::HostIo::Status::Ok, nullptr, 0);

      // Now wait for raw data from host
      // 1. Read 4 bytes file size (Big Endian)
      uint8_t size_buf[4];
      if (!SRL::DevCart::HostIo::ReadAll(size_buf, 4))
      {
        f_close(&file);
        return SRL::DevCart::HostIo::Status::Handled; // skip auto-response
      }
      uint32_t file_size = (static_cast<uint32_t>(size_buf[0]) << 24) |
                           (static_cast<uint32_t>(size_buf[1]) << 16) |
                           (static_cast<uint32_t>(size_buf[2]) << 8) |
                           static_cast<uint32_t>(size_buf[3]);

      // 2. Read File Data & Calculate CRC
      uint8_t buffer[4096];
      uint32_t received = 0;
      uint8_t checksum = 0;
      while (received < file_size)
      {
        uint32_t chunk = file_size - received;
        if (chunk > sizeof(buffer))
        {
          chunk = sizeof(buffer);
        }
        if (!SRL::DevCart::HostIo::ReadAll(buffer, chunk))
        {
          break;
        }

        checksum = Crc8Update(checksum, buffer, chunk);
        UINT bw = 0;
        f_write(&file, buffer, chunk, &bw);
        received += chunk;
      }

      f_close(&file);

      // 3. Read Checksum
      uint8_t host_checksum;
      if (!SRL::DevCart::HostIo::ReadAll(&host_checksum, 1))
      {
        return SRL::DevCart::HostIo::Status::Handled;
      }

      // 4. Send Result
      uint8_t result = (host_checksum == checksum) ? 0x00 : 0x01;
      SRL::DevCart::HostIo::WriteAll(&result, 1);

      return SRL::DevCart::HostIo::Status::Handled;
    }

    APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
               "[DoUpload] Unsupported path: %s\n", path);
    return SRL::DevCart::HostIo::Status::Unsupported;
  }

  const int kShellStartY = 2;
  const int kShellMaxY = 27;
  int g_shellY = kShellStartY;

  /**
   * @brief Prints text to a scrolling shell-like view on the screen.
   * 
   * Wraps long lines and handles scrolling when the text reaches the bottom of the screen.
   * 
   * @param text The text to print.
   */
  void PrintShell(const char* text)
  {
      if (!text) return;
      const char* p = text;
      while (*p)
      {
          char lineBuf[45] = {0};
          int len = 0;
          while (*p && *p != '\n' && len < 44)
          {
              lineBuf[len++] = *p++;
          }
          if (*p == '\n') p++;

          SRL::Debug::PrintClearLine(g_shellY);
          SRL::Debug::Print(1, g_shellY, lineBuf);
          g_shellY++;

          if (g_shellY > kShellMaxY)
          {
              g_shellY = kShellStartY;
              for (int y = kShellStartY; y <= kShellMaxY; ++y)
              {
                  SRL::Debug::PrintClearLine(y);
              }
          }
      }
      SRL::Debug::PrintClearLine(g_shellY);
  }

  /**
   * @brief Reads and processes an incoming Host I/O request.
   * 
   * Decodes the request command, routes it to the appropriate handler (e.g., HandleList,
   * HandleUpload), and sends the response back to the host via the DevCart interface.
   * Also echoes the command and response to the on-screen shell.
   */
  void HandleHostIoRequest()
  {
    uint8_t requestPayload[kMaxRequestBytes + 1];
    size_t requestLen = 0;
    SRL::DevCart::HostIo::Command command = SRL::DevCart::HostIo::Command::List;

    if (!SRL::DevCart::HostIo::TryReadRequest(command, requestPayload,
                                              kMaxRequestBytes, requestLen))
    {
      const char msg[] = "[HostIo] Bad request frame\n";
      SRL::DevCart::HostIo::SendResponse(
          SRL::DevCart::HostIo::Status::BadRequest,
          reinterpret_cast<const uint8_t *>(msg), sizeof(msg) - 1);
      return;
    }

    requestPayload[requestLen] = '\0';
    const char *path = reinterpret_cast<const char *>(requestPayload);

    const char *commandName = "UNKNOWN";
    switch (command)
    {
    case SRL::DevCart::HostIo::Command::List:
      commandName = "LS";
      break;
    case SRL::DevCart::HostIo::Command::Remove:
      commandName = "RM";
      break;
    case SRL::DevCart::HostIo::Command::Crc:
      commandName = "CRC";
      break;
    case SRL::DevCart::HostIo::Command::Upload:
      commandName = "UPLOAD";
      break;
    case SRL::DevCart::HostIo::Command::Mkdir:
      commandName = "MKDIR";
      break;
    case SRL::DevCart::HostIo::Command::Rmdir:
      commandName = "RMDIR";
      break;
    default:
      break;
    }

    // Keep latest incoming host command visible during request handling.
    char cmdBuf[80];
    snprintf(cmdBuf, sizeof(cmdBuf), "> %s %s\n", commandName, path);
    PrintShell(cmdBuf);

    char response[kMaxResponseBytes + 1] = {0};
    size_t responseLen = 0;
    SRL::DevCart::HostIo::Status status = SRL::DevCart::HostIo::Status::Error;

    switch (command)
    {
    case SRL::DevCart::HostIo::Command::List:
      status = HandleList(path, response, responseLen);
      break;
    case SRL::DevCart::HostIo::Command::Remove:
      status = HandleRemove(path, response, responseLen);
      break;
    case SRL::DevCart::HostIo::Command::Crc:
      status = HandleCrc(path, response, responseLen);
      break;
    case SRL::DevCart::HostIo::Command::Upload:
      status = HandleUpload(path, response, responseLen);
      break;
    case SRL::DevCart::HostIo::Command::Mkdir:
      status = HandleMkdir(path, response, responseLen);
      break;
    case SRL::DevCart::HostIo::Command::Rmdir:
      status = HandleRmdir(path, response, responseLen);
      break;
    default:
      APPEND_FMT(response, kMaxResponseBytes + 1, responseLen,
                 "[HostIo] Unsupported command: %u\n",
                 static_cast<unsigned int>(command));
      status = SRL::DevCart::HostIo::Status::Unsupported;
      break;
    }

    if (status != SRL::DevCart::HostIo::Status::Handled)
    {
      SRL::DevCart::HostIo::SendResponse(
          status, reinterpret_cast<const uint8_t *>(response), responseLen);

      // Send explicit end-of-listing sentinel for List command
      if (command == SRL::DevCart::HostIo::Command::List && status == SRL::DevCart::HostIo::Status::Ok)
      {
        SRL::DevCart::HostIo::SendResponse(SRL::DevCart::HostIo::Status::Ok, nullptr, 0);
      }
    }
    if (responseLen > 0)
    {
      PrintShell(response);
    }
  }
}

#undef APPEND_FMT

int main()
{
  SRL::Core::Initialize(HighColor::Colors::Black);
  SRL::Cd::Initialize();

  SRL::Debug::Print(1, 0, "File Transfer - FTX");

  while (true)
  {
    if (!SRL::DevCart::CS0::isRXFEmpty())
    {
      HandleHostIoRequest();
    }

    SRL::Core::Synchronize();
  }

  return 0;
}
