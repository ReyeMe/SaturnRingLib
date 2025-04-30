#pragma once
/**
 * @file srl_cd_block.hpp
 * @brief CD Block Hardware Interface
 * 
 * This file provides direct access to the Sega Saturn CD Block hardware registers.
 * The CD Block handles all CD-ROM operations including audio playback,
 * data reading, and CD-DA (digital audio) access.
 * 
 * @note This provides a low-level direct hardware access alternative to SGL functions
 */

#include "srl_hardware_common.hpp"

namespace SRL::Hardware::CDBlock
{
    using namespace MemoryRegions;

    //----------------------------------------------------------------------
    // Communication registers - Host interface to CD subsystem
    //----------------------------------------------------------------------
    static inline constexpr Reg16 Interrupt { CdBlockBase, 0x0008UL };      // Interrupt status - Shows current interrupt conditions
    static inline constexpr Reg16 InterruptMask { CdBlockBase, 0x000CUL };  // Interrupt mask - Controls which interrupts are enabled
    static inline constexpr Reg16 Command { CdBlockBase, 0x0018UL };        // Command register - Sends commands to CD subsystem
    static inline constexpr Reg32 DataTransfer { CdBlockBase, 0x0060UL };    // Data transfer - Main interface for data exchange

    /**
     * @brief CD Block interrupt status bit flags (HIRQ)
     * 
     * These flags indicate various CD subsystem events and statuses.
     * They are read from the Interrupt register and can be masked
     * using the InterruptMask register.
     */
    namespace InterruptBits
    {
        static inline constexpr std::uint16_t CommandComplete = 0x0001U;       // Command has finished processing
        static inline constexpr std::uint16_t DataTransferReady = 0x0002U;     // Data is ready for transfer
        static inline constexpr std::uint16_t SectorTransferComplete = 0x0004U; // Full sector has been transferred
        static inline constexpr std::uint16_t BufferFull = 0x0008U;           // CD buffer is full
        static inline constexpr std::uint16_t PlayEnd = 0x0010U;              // Audio playback has ended
        static inline constexpr std::uint16_t DiscChange = 0x0020U;           // CD media has been changed/ejected
        static inline constexpr std::uint16_t EndOfFile = 0x0040U;            // Reached end of current file
        static inline constexpr std::uint16_t EndOfSectorHeader = 0x0080U;    // Finished reading sector header
        static inline constexpr std::uint16_t EndOfSectorCopy = 0x0100U;      // Finished copying sector data
        static inline constexpr std::uint16_t EndOfFileList = 0x0200U;        // Reached end of file list
        static inline constexpr std::uint16_t SubcodeQDataReady = 0x0400U;    // Subcode Q data is available
        static inline constexpr std::uint16_t MpegEndOfDecode = 0x0800U;      // MPEG decoder has completed
        static inline constexpr std::uint16_t MpegEndOfFrameDecode = 0x1000U; // MPEG frame decoding complete
        static inline constexpr std::uint16_t MpegInterruptTrigger = 0x2000U; // MPEG decoder triggered an interrupt
    }

    /**
     * @brief CD Block command codes
     * 
     * These commands are sent to the CD Block to perform various operations.
     */
    namespace CommandCodes
    {
        // System commands
        static inline constexpr std::uint16_t GetStatus = 0x0000U;        // Get CD Block status
        static inline constexpr std::uint16_t GetHardwareInfo = 0x0001U;  // Get hardware information
        static inline constexpr std::uint16_t GetToc = 0x0002U;           // Get Table of Contents
        static inline constexpr std::uint16_t GetSessionInfo = 0x0003U;   // Get session information
        static inline constexpr std::uint16_t Initialize = 0x0004U;       // Initialize CD Block
        static inline constexpr std::uint16_t OpenTray = 0x0005U;         // Open disc tray
        static inline constexpr std::uint16_t EndDataTransfer = 0x0006U;  // End data transfer
        
        // CD-DA playback commands
        static inline constexpr std::uint16_t PlayDisc = 0x0010U;         // Play audio tracks
        static inline constexpr std::uint16_t SeekDisc = 0x0011U;         // Seek to position on disc
        static inline constexpr std::uint16_t ScanDisc = 0x0012U;         // Scan disc (fast forward/rewind)
        static inline constexpr std::uint16_t GetSubcodeQ = 0x0020U;      // Get subcode Q data
        static inline constexpr std::uint16_t SetCdcMode = 0x0030U;       // Set CD drive mode
        
        // CD-ROM data commands
        static inline constexpr std::uint16_t GetDirInfo = 0x0072U;       // Get directory information
        static inline constexpr std::uint16_t ReadFile = 0x0074U;         // Read file from disc
        static inline constexpr std::uint16_t GetFileSize = 0x0075U;      // Get file size
        static inline constexpr std::uint16_t GetFileInfo = 0x0076U;      // Get file information
    }

    /**
     * @brief CD Block status codes
     * 
     * These status codes are returned by the GetStatus command.
     */
    namespace StatusCodes
    {
        static inline constexpr std::uint16_t Busy = 0x0000U;            // CD Block is busy
        static inline constexpr std::uint16_t Paused = 0x0100U;          // CD playback is paused
        static inline constexpr std::uint16_t Standby = 0x0200U;         // CD drive is in standby mode
        static inline constexpr std::uint16_t Playing = 0x0300U;         // CD is playing
        static inline constexpr std::uint16_t Seeking = 0x0400U;         // CD drive is seeking
        static inline constexpr std::uint16_t Scanning = 0x0500U;        // CD drive is scanning
        static inline constexpr std::uint16_t Open = 0x0600U;            // CD tray is open
        static inline constexpr std::uint16_t NoDisc = 0x0700U;          // No disc in drive
        static inline constexpr std::uint16_t Retry = 0x0800U;           // CD drive is retrying operation
        static inline constexpr std::uint16_t Error = 0x0900U;           // CD drive error
        static inline constexpr std::uint16_t Fatal = 0x0A00U;           // Fatal CD drive error
    }

    /**
     * @brief CD Block sector data format
     * 
     * This structure defines the format of a CD-ROM sector.
     */
    struct SectorData
    {
        std::uint8_t header[16];     // Sector header (sync + address)
        std::uint8_t data[2048];     // Sector data (mode 1)
        std::uint8_t edc[4];         // Error detection code
        std::uint8_t ecc[276];       // Error correction code
    };
}
