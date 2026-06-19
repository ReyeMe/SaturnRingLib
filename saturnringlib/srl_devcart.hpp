// Based on SatCom Library by cafe-alpha, Original:
// http://ppcenter.free.fr/satcom/

#pragma once
#include <cstddef> // For size_t
#include <cstdint> // For uintptr_t, size_t, uint8_t, uint32_t
//#include <cstring>  // For memcpy
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

    /** @brief CS0 area: Flash memory and USB-related registers.
     *
     *  This namespace groups constants and functions for accessing the cartridge's
     * CS0 memory space, which includes flash memory and USB communication registers
     * (likely for a Sega Saturn USB dev cart). Addresses are memory-mapped I/O;
     * accesses should use volatile pointers to prevent optimization issues.
     */
    namespace CS0
    {
      /** @brief Base address of the cartridge in CS0 area.
       *
       *  This is the starting point for flash and USB registers (overlaps with
       * DataCart in srl_cartridge.hpp).
       */
      constexpr static uintptr_t CART_BASE_ADR =
          0x22000000UL; // Base address of the cartridge in CS0 area



      constexpr static uintptr_t CART_PCNTR =
          CART_BASE_ADR + 0x1FFFFF0UL; // Wasca Prepare counter.

      constexpr static uintptr_t CART_STATUS =
          CART_BASE_ADR + 0x1FFFFF2UL; // Wasca Status register.

      constexpr static uintptr_t CART_HWVER =
          CART_BASE_ADR +
          0x1FFFFF6UL; // wasca hardware version, major and minor 0x050C = v5.12

      constexpr static uintptr_t CART_SIGNATURE =
          CART_BASE_ADR +
          0x1FFFFFAUL; // Signature: “wasca “ in ASCII (0x7761 0x7363 0x6120)

      /** @brief Base address of the flash memory (1MB region).
       */
      constexpr static uintptr_t FLASH_MEMORY_BASE =
          CART_BASE_ADR + 0x0; // Base address of the flash memory (1MB)

      /** @brief Address of the USB flags register (8-bit Read/Write).
       *
       *  This register holds status flags for USB FIFO operations (RXF, TXE, PWREN).
       */
      constexpr static uintptr_t USB_FLAGS =
          CART_BASE_ADR +
          0x200001UL; // Address of the USB flags register (Read/Write)

      /** @brief Address of the USB FIFO data register (8-bit Read/Write).
       *
       *  Used for sending/receiving bytes over USB.
       */
      constexpr static uintptr_t USB_FIFO =
          CART_BASE_ADR +
          0x100001; // Address of the USB FIFO data register (Read/Write)
      // 0x223x to 0x227x unused  // Reserved/unused address range in hardware

      /**
       * @brief Registers for controlling the SD card on the development cartridge.
       *
       * These registers are mapped in the CS0 memory space.
       */
      namespace SDCardRegisters
      {
        constexpr static uintptr_t CART_CID =
            CART_BASE_ADR + 0x1FF0200UL; // Card Identification Number Register

        constexpr static uintptr_t CART_CSD =
            CART_BASE_ADR + 0x1FF0210UL; // Card Specific Data Register

        constexpr static uintptr_t CART_OCR =
            CART_BASE_ADR + 0x1FF0220UL; // Operation Condition Register

        constexpr static uintptr_t CART_SR =
            CART_BASE_ADR + 0x1FF0224UL; // SD Card Status Register

        constexpr static uintptr_t CART_RC =
            CART_BASE_ADR + 0x1FF0228UL; // Relative Card Address Register

        constexpr static uintptr_t CART_CMD_ARG =
            CART_BASE_ADR + 0x1FF022CUL; // Command Argument Register

        constexpr static uintptr_t CART_CMD =
            CART_BASE_ADR + 0x1FF0230UL; // Command Register

        constexpr static uintptr_t CART_ASR =
            CART_BASE_ADR + 0x1FF0234UL; // Auxiliary Status Register

        constexpr static uintptr_t CART_RR1 =
            CART_BASE_ADR + 0x1FF0238UL; // Response R1

        constexpr static uintptr_t CART_WSSCR =
            CART_BASE_ADR + 0x1FF0FFEUL; // wasca Specific SD Control Register

      } // namespace SDCardRegisters

      /** @brief Maximum length allowed for firmware uploads (matches flash size).
       */
      constexpr static size_t FIRM_MAXLEN =
          1024 * 1024; // Maximum length allowed for firmware (1MB)

      /**
       * @brief Class representing the USB flags register bits.
       *
       * This class provides a type-safe way to manipulate the bits in the USB flags
       * register (RXF, TXE, PWREN). It supports bitwise operations and flag checking.
       *
       * Note: Only bits 0,1,7 are defined; others are ignored/reserved.
       */
      class USBFlags
      {
      public:
        // Bit position constants (accessible as USBFlags::TXE, etc.)
        enum : uint8_t
        {
          RXF = 1 << 0,  // RXF: Receive FIFO Full (data available to read)
          TXE = 1 << 1,  // TXE: Transmit FIFO Empty (ready to accept data)
          PWREN = 1 << 7 // PWREN: Power Enable (USB power control)
        };

        /** @brief Mask for all defined flags (bits 0,1,7). */
        static constexpr uint8_t ALL_FLAGS =
            (RXF | TXE | PWREN); // Mask for all defined flags

        /** @brief Inverted mask for all defined flags (for clearing/checking
         * undefined bits). */
        static constexpr uint8_t NOT_ALL_FLAGS =
            static_cast<uint8_t>(~ALL_FLAGS); // Mask for not all defined flags

      private:
        uint8_t bits_; // Raw bit storage (8-bit value read/written to hardware)

      public:
        /** @brief Default constructor: Initializes with no flags set. */
        USBFlags() : bits_(0) {}

        /** @brief Constructor: Initialize with raw bit value. */
        explicit USBFlags(uint8_t bits) : bits_(bits) {}

        /** @brief Constructor: Initialize by OR-ing a list of flag constants.
         *  @param flags Initializer list of flag enums (e.g., {USBFlags::RXF,
         * USBFlags::TXE}).
         */
        USBFlags(std::initializer_list<uint8_t> flags) : bits_(0)
        {
          for (auto f : flags)
            bits_ |= f; // Set each provided flag
        }

        /** @brief Conversion to bool: True if any flag is set. */
        explicit operator bool() const { return bits_ != 0; }

        /** @brief Bitwise OR: Combine with another USBFlags. */
        USBFlags operator|(USBFlags other) const
        {
          return USBFlags(bits_ | other.bits_);
        }

        /** @brief Bitwise OR assignment: Add flags from another. */
        USBFlags &operator|=(USBFlags other)
        {
          bits_ |= other.bits_;
          return *this;
        }

        /** @brief Bitwise AND: Keep only common flags. */
        USBFlags operator&(USBFlags other) const
        {
          return USBFlags(bits_ & other.bits_);
        }

        /** @brief Bitwise AND assignment: Retain common flags. */
        USBFlags &operator&=(USBFlags other)
        {
          bits_ &= other.bits_;
          return *this;
        }

        /** @brief Bitwise NOT: Invert all bits (careful: affects undefined bits too).
         */
        USBFlags operator~() const { return USBFlags(static_cast<uint8_t>(~bits_)); }

        /** @brief Check if a specific flag is set.
         *  @param flag The flag constant to test (e.g., USBFlags::TXE).
         *  @return True if set.
         */
        bool has(uint8_t flag) const { return (bits_ & flag) != 0; }

        /** @brief Get the raw bit value (for writing to hardware). */
        uint8_t bits() const { return bits_; }
      };

      /**
       * @brief Checks if the Transmit FIFO Empty (TXE) flag is set.
       *
       * Reads the USB_FLAGS register and tests the TXE bit. When the TXE bit is set,
       * the transmit FIFO is full and cannot accept new data. The function name
       * `isTXEFull` is accurate in this context, though `TXE` often means "Transmit
       * Empty" in other hardware.
       *
       * @return true If TXE is set (FIFO is full), false otherwise.
       */
      static inline bool isTXEFull()
      {
        return ((*(volatile uint8_t *)(USB_FLAGS)) & USBFlags::TXE) !=
               0; // Added volatile for MMIO safety
      }

      /**
       * @brief Reads the raw USB_FLAGS register value.
       */
      static inline uint8_t readFlags() { return *(volatile uint8_t *)(USB_FLAGS); }

      /**
       * @brief Waits until the Transmit FIFO is ready (TXE cleared?).
       * This function polls `isTXEFull()` until it returns false, which indicates
       * that the transmit FIFO is no longer full and can accept data.
       *
       * Warning: Infinite loop if hardware never clears—consider adding timeout in
       * production code.
       *
       */
      static inline void waitTXE()
      {
        // Bad design, no timeout! TODO: Add optional timeout parameter or counter
        while (isTXEFull())
          ; // Busy-wait
      }

      /**
       * @brief Waits until the Transmit FIFO is ready, with timeout.
       *
       * Polls `isTXEFull()` until it returns false. If `maxPolls` reaches zero first,
       * the function returns false to signal timeout.
       *
       * @param maxPolls Maximum number of polling iterations while FIFO is full.
       * @return true if FIFO became ready before timeout, false otherwise.
       */
      static inline bool waitTXE(uint32_t maxPolls)
      {
        while (isTXEFull())
        {
          if (maxPolls == 0)
          {
            return false;
          }
          --maxPolls;
        }
        return true;
      }

      /**
       * @brief Checks if the Receive FIFO (RXF) is empty.
       *
       * Reads the USB_FLAGS register and checks the RXF bit.
       * The FIFO is considered empty while RXF is set.
       *
       * @return true If FIFO is empty, false otherwise.
       */
      static inline bool isRXFEmpty()
      {
        return ((*(volatile uint8_t *)(USB_FLAGS)) & USBFlags::RXF) !=
               0; // Added volatile
      }

      /**
       * @brief Waits until data is available in Receive FIFO.
       *
       * This function polls `isRXFEmpty()` until it returns false, indicating data is
       * ready to be read.
       *
       * Warning: Infinite loop possible—add timeout if needed.
       */
      static inline void waitRXF()
      {
        // Bad design, no timeout !
        while (isRXFEmpty())
          ; // Busy-wait
      }

      /**
       * @brief Waits until data is available in Receive FIFO, with timeout.
       *
       * Polls `isRXFEmpty()` until it returns false. If `maxPolls` reaches zero
       * first, the function returns false to signal timeout.
       *
       * @param maxPolls Maximum number of polling iterations while FIFO is empty.
       * @return true if data became available before timeout, false otherwise.
       */
      static inline bool waitRXF(uint32_t maxPolls)
      {
        while (isRXFEmpty())
        {
          if (maxPolls == 0)
          {
            return false;
          }
          --maxPolls;
        }
        return true;
      }

      /**
       * @brief Writes a single byte to the USB FIFO.
       *
       * This function waits until the transmit FIFO is not full (`waitTXE()`) and
       * then writes a single byte.
       *
       * @param c Pointer to the byte to write.
       * @return size_t 1 on success.
       */
      static inline size_t write(const uint8_t *c)
      {
        size_t counter = 0;

        waitTXE();
        *(volatile uint8_t *)(USB_FIFO) = *c; // Volatile for MMIO
        ++counter;
        return counter;
      }

      /**
       * @brief Writes a buffer to the USB FIFO.
       *
       * This function writes a buffer of a given size to the USB FIFO by writing one
       * byte at a time, waiting for the FIFO to be ready for each byte.
       *
       * @param c Pointer to the buffer.
       * @param size Number of bytes to write.
       * @return size_t Number of bytes written.
       */
      static inline size_t write(const uint8_t *c, size_t size)
      {
        size_t counter = 0;
        for (size_t i = 0; i < size; i++)
        {
          counter += write(c + i);
        }
        return counter;
      }

      /**
       * @brief Reads a single byte from the USB FIFO.
       *
       * This function waits until data is available in the receive FIFO (`waitRXF()`)
       * and then reads a single byte.
       *
       * @return uint8_t The byte read.
       */
      static inline uint8_t read()
      {
        waitRXF();
        return *(volatile uint8_t *)(USB_FIFO); // Volatile for MMIO
      }

      /**
       * @brief Checks if the USB device is connected and ready.
       *
       * This function checks the `USB_FLAGS` register. It assumes the device is
       * connected if the reserved bits (those not in `ALL_FLAGS`) are all zero. This
       * is a common way to detect hardware presence on embedded systems.
       *
       * @return true If connected, false otherwise.
       */
      static inline bool isConnected()
      {
        const uint8_t flags = readFlags();
        // SatCom-compatible test: bits 7..2 must be low when FTDI is USB powered.
        return (flags & 0xFCU) == 0;
      }

      /**
       * @brief Returns true when USB dev cart flag register pattern looks valid.
       */
      static inline bool isPortAvailable()
      {
        const uint8_t flags = readFlags();
        // SatCom-compatible availability test: reserved bits 6..2 should stay low.
        return (flags & 0x7CU) == 0;
      }

    } // namespace CS0

    /** @brief CS1 area: CPLD registers.
     *
     *  This namespace groups constants for accessing the CPLD (Complex Programmable
     * Logic Device) registers, which are used to control features like LEDs, the SD
     * card interface, and general-purpose I/O.
     */
    namespace CS1
    {
      /** @brief Base address for CPLD registers in CS1 space. */
      constexpr static uint32_t CPLD_BASE_ADDR =
          0x24000000L; // Base address for CPLD registers (note: L suffix for long)

      /**
       * @brief Enumeration of CPLD register addresses.
       *
       * These are offsets from `CPLD_BASE_ADDR`. The values `0x55` and `0xAA` are
       * likely part of a handshake or initialization sequence. Access to these
       * registers is typically 8-bit or 16-bit; refer to the hardware documentation
       * for specifics.
       */
      enum class Register : uint32_t
      {
        CPLD_55 =
            CPLD_BASE_ADDR + 0x01, // Register CPLD_55 (possibly init/write 0x55)
        CPLD_AA =
            CPLD_BASE_ADDR + 0x03,             // Register CPLD_AA (possibly init/write 0xAA)
        CART_CPLD_VER = CPLD_BASE_ADDR + 0x05, // Register: CPLD version (read-only?)
        CART_BETA_ID = CPLD_BASE_ADDR + 0x07,  // Register: Beta/ID identifier
        CPLD_IO = CPLD_BASE_ADDR + 0x09,       // Register: General I/O control
        SDIN_BITS = CPLD_BASE_ADDR + 0x0B,     // Register: SD input bits
        LED_SETTING = CPLD_BASE_ADDR +
                      0x0D,                 // Register: LED settings (bitfield for colors/modes)
        SD_CLK_SET = CPLD_BASE_ADDR + 0x0F, // Register: SD clock configuration
        REG_STDOUT_BIT =
            CPLD_BASE_ADDR + 0x11, // Register: Stdout bit (debug/output?)
        REG_SD_IO_0 = CPLD_BASE_ADDR +
                      0x11,                  // Register: SD I/O port 0 (shared address with above?)
        REG_SD_IO_1 = CPLD_BASE_ADDR + 0x13, // Register: SD I/O port 1
        REG_SD_IO_2 = CPLD_BASE_ADDR + 0x15, // Register: SD I/O port 2
        REG_SD_IO_3 = CPLD_BASE_ADDR + 0x17, // Register: SD I/O port 3
        REG_SD_REINSERT =
            CPLD_BASE_ADDR + 0x19, // Register: SD reinsert/eject command
        REG_SD_WRITE_PROTECT =
            CPLD_BASE_ADDR +
            0x1B // USB Gamer's cart SD write-protect / SD present status
      };

      /**
       * @brief Reads an 8-bit CS1 register value from the DevCart CPLD space.
       */
      static inline uint8_t ReadRegister(const Register reg)
      {
        return *(volatile uint8_t *)(static_cast<uint32_t>(reg));
      }

      /**
       * @brief Returns true when the expected CPLD identification bytes are present.
       */
      static inline bool HasWascaSignature()
      {
        return ReadRegister(Register::CPLD_55) == 0x55 &&
               ReadRegister(Register::CPLD_AA) == 0xAA;
      }

      /**
       * @brief Returns true when cartridge reports USB Gamer's CPLD version.
       */
      static inline bool IsUsbGamersCartridge()
      {
        return ReadRegister(Register::CART_CPLD_VER) == 0x19;
      }



      /**
       * @note `REG_STDOUT_BIT` and `REG_SD_IO_0` share the same address.
       * This suggests they might be bit aliases or their function is mode-dependent.
       * Care should be taken to avoid conflicts when using them.
       */



    } // namespace CS1

    /**
     * @brief Minimal framed protocol for host commands over DevCart USB FIFO.
     *
     * This protocol is used by host tools (such as ftx) to send filesystem-like
     * requests (ls/rm/crc) through FTDI, where Saturn-side code can parse and
     * handle them.
     *
     * Request frame:
     *   - 4 bytes magic: "SRL1"
     *   - 1 byte command
     *   - 2 bytes payload length (big-endian)
     *   - N bytes payload
     *
     * Response frame:
     *   - 4 bytes magic: "SRL1"
     *   - 1 byte status
     *   - 2 bytes payload length (big-endian)
     *   - N bytes payload
     */
    namespace HostIo
    {
      enum class Command : uint8_t
      {
        List = 1,
        Remove = 2,
        Crc = 3
      };

      enum class Status : uint8_t
      {
        Ok = 0,
        Error = 1,
        Unsupported = 2,
        BadRequest = 3
      };

      constexpr static uint8_t MAGIC_0 = 'S';
      constexpr static uint8_t MAGIC_1 = 'R';
      constexpr static uint8_t MAGIC_2 = 'L';
      constexpr static uint8_t MAGIC_3 = '1';
      constexpr static size_t HEADER_SIZE = 7;

      static inline bool WriteAll(const uint8_t *data, size_t size)
      {
        return CS0::write(data, size) == size;
      }

      static inline bool ReadAll(uint8_t *data, size_t size)
      {
        for (size_t i = 0; i < size; ++i)
        {
          data[i] = CS0::read();
        }
        return true;
      }

      static inline uint16_t DecodeU16BE(const uint8_t hi, const uint8_t lo)
      {
        return static_cast<uint16_t>((static_cast<uint16_t>(hi) << 8) |
                                     static_cast<uint16_t>(lo));
      }

      static inline bool TryReadRequest(Command &command,
                                        uint8_t *payloadBuffer,
                                        size_t payloadCapacity,
                                        size_t &payloadSize)
      {
        payloadSize = 0;
        uint8_t header[HEADER_SIZE];
        if (!ReadAll(header, HEADER_SIZE))
        {
          return false;
        }

        if (header[0] != MAGIC_0 || header[1] != MAGIC_1 ||
            header[2] != MAGIC_2 || header[3] != MAGIC_3)
        {
          return false;
        }

        command = static_cast<Command>(header[4]);
        const uint16_t payloadLen = DecodeU16BE(header[5], header[6]);

        if (payloadLen > payloadCapacity)
        {
          uint8_t sink = 0;
          for (uint16_t i = 0; i < payloadLen; ++i)
          {
            sink = CS0::read();
          }
          (void)sink;
          return false;
        }

        if (payloadLen > 0)
        {
          ReadAll(payloadBuffer, payloadLen);
          payloadSize = payloadLen;
        }

        return true;
      }

      static inline bool SendResponse(Status status,
                                      const uint8_t *payload,
                                      size_t payloadSize)
      {
        if (payloadSize > 0xFFFFU)
        {
          return false;
        }

        uint8_t header[HEADER_SIZE] = {
            MAGIC_0,
            MAGIC_1,
            MAGIC_2,
            MAGIC_3,
            static_cast<uint8_t>(status),
            static_cast<uint8_t>((payloadSize >> 8) & 0xFFU),
            static_cast<uint8_t>(payloadSize & 0xFFU)};

        if (!WriteAll(header, HEADER_SIZE))
        {
          return false;
        }

        if (payloadSize == 0)
        {
          return true;
        }

        return WriteAll(payload, payloadSize);
      }
    } // namespace HostIo

    namespace SD
    {
      /* Local SGCLIB/FatFs compatibility layer.
       * This keeps the sample self-contained inside SRL and avoids direct includes
       * on the external beta headers.
       */
      constexpr int SGC_FR_OK = 0;
      constexpr int FA_READ   = 0x01;
      constexpr uint8_t AM_DIR = 0x10;

      typedef struct
      {
        uint32_t size;
        uint16_t date, time;
        uint8_t attrib;
        char name[13];
      } __attribute__((packed)) sgc_stat_t;

      typedef uint32_t DWORD;
      typedef uint16_t WORD;
      typedef uint8_t BYTE;
      typedef const char TCHAR;

      typedef struct
      {
        BYTE fs_type;
        BYTE pdrv;
        BYTE n_fats;
        BYTE wflag;
        BYTE fsi_flag;
        WORD id;
        WORD n_rootdir;
        WORD csize;
        WORD ssize;
        DWORD n_fatent;
        DWORD fsize;
        uint32_t volbase;
        uint32_t fatbase;
        uint32_t dirbase;
        uint32_t database;
        uint32_t winsect;
        BYTE win[512];
      } FATFS;

      typedef struct
      {
        FATFS *fs;
        WORD id;
        BYTE attr;
        BYTE stat;
        DWORD sclust;
        uint32_t objsize;
        DWORD cltbl;
      } FFOBJID;

      typedef struct
      {
        FFOBJID obj;
        uint32_t dptr;
        DWORD clust;
        uint32_t sect;
        BYTE *dir;
        BYTE fn[12];
        DWORD blk_ofs;
        const TCHAR *pat;
      } DIR;

      typedef struct
      {
        uint32_t fsize;
        WORD fdate;
        WORD ftime;
        BYTE fattrib;
        TCHAR fname[12 + 1];
      } FILINFO;

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

      /** @brief Pointer to the SGCLIB API struct at its fixed load address. */
      static inline sgclib_api_t *const SGCLIB_API =
          reinterpret_cast<sgclib_api_t *>(0x060BA000);

      /** @brief Returns the SGCLIB API pointer. */
      static inline sgclib_api_t *sgclib_api()
      {
        return SGCLIB_API;
      }

      /** @brief Loads the SGCLIB firmware stub into its fixed memory location. */
      static inline void fs_load_stub(const void *stubPtr, size_t stubSize)
      {
        memcpy(SGCLIB_API, stubPtr, stubSize);
      }

      /** @brief Initializes the SGCLIB FAT driver. Returns SGC_FR_OK on success. */
      static inline int fs_init()
      {
        return SGCLIB_API->init();
      }

      /** @brief Gets current working directory. */
      static inline void fs_getcwd(char *buf, int len)
      {
        SGCLIB_API->getcwd(buf, len);
      }

      /** @brief Changes current working directory. */
      static inline void fs_chdir(const char *path)
      {
        SGCLIB_API->chdir(path);
      }

      /** @brief Opens a file. Returns file descriptor or -1 on error. */
      static inline int fs_open(const char *path, int flags)
      {
        return SGCLIB_API->open(path, flags);
      }

      /** @brief Closes a file descriptor. */
      static inline void fs_close(int fd)
      {
        SGCLIB_API->close(fd);
      }

      /** @brief Reads from an open file. Returns bytes read, 0 at EOF, <0 on error. */
      static inline int fs_read(int fd, void *buf, int len)
      {
        return SGCLIB_API->read(fd, buf, len);
      }

      /** @brief Gets file/directory metadata. Returns SGC_FR_OK on success. */
      static inline int fs_stat(const char *path, sgc_stat_t *stat)
      {
        return SGCLIB_API->stat(path, stat, static_cast<int>(sizeof(sgc_stat_t)));
      }

      /** @brief Opens a directory for listing. Returns SGC_FR_OK on success. */
      static inline int fs_opendir(const char *path)
      {
        return SGCLIB_API->opendir(path);
      }

      /** @brief Removes a file. Returns SGC_FR_OK on success. */
      static inline int fs_unlink(const char *path)
      {
        return SGCLIB_API->unlink(path);
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
            int digit = (base == 16) ? HexDigitValue(*cursor) :
                                       ((*cursor >= '0' && *cursor <= '9') ? (*cursor - '0') : -1);
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