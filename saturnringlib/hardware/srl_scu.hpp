#pragma once
/**
 * @file srl_scu.hpp
 * @brief SCU (System Control Unit) Hardware Interface
 * 
 * This file provides direct access to the Sega Saturn SCU hardware registers.
 * The SCU handles DMA transfers, interrupts, and system control functions.
 * 
 * @note This provides a low-level direct hardware access alternative to SGL functions
 */

#include "srl_hardware_common.hpp"

namespace SRL::Hardware::SCU
{
    using namespace MemoryRegions;

    //----------------------------------------------------------------------
    // DMA registers - Direct Memory Access control
    //----------------------------------------------------------------------
    static inline constexpr Reg32 Dma0Control { ScuBase, 0x0100UL };      // DMA0 Control - Level 0 DMA control register
    static inline constexpr Reg32 Dma0Source { ScuBase, 0x0104UL };       // DMA0 Source - Level 0 DMA source address
    static inline constexpr Reg32 Dma0Dest { ScuBase, 0x0108UL };         // DMA0 Destination - Level 0 DMA destination address
    static inline constexpr Reg32 Dma0Size { ScuBase, 0x010CUL };         // DMA0 Size - Level 0 DMA transfer size
    static inline constexpr Reg32 Dma1Control { ScuBase, 0x0110UL };      // DMA1 Control - Level 1 DMA control register
    static inline constexpr Reg32 Dma1Source { ScuBase, 0x0114UL };       // DMA1 Source - Level 1 DMA source address
    static inline constexpr Reg32 Dma1Dest { ScuBase, 0x0118UL };         // DMA1 Destination - Level 1 DMA destination address
    static inline constexpr Reg32 Dma1Size { ScuBase, 0x011CUL };         // DMA1 Size - Level 1 DMA transfer size
    static inline constexpr Reg32 Dma2Control { ScuBase, 0x0120UL };      // DMA2 Control - Level 2 DMA control register
    static inline constexpr Reg32 Dma2Source { ScuBase, 0x0124UL };       // DMA2 Source - Level 2 DMA source address
    static inline constexpr Reg32 Dma2Dest { ScuBase, 0x0128UL };         // DMA2 Destination - Level 2 DMA destination address
    static inline constexpr Reg32 Dma2Size { ScuBase, 0x012CUL };         // DMA2 Size - Level 2 DMA transfer size
    static inline constexpr Reg32 DmaStatus { ScuBase, 0x0130UL };        // DMA Status - DMA operation status flags
    static inline constexpr Reg32 DmaIllegal { ScuBase, 0x0134UL };       // DMA Illegal - DMA illegal address detection
    static inline constexpr Reg32 DmaMaskClear { ScuBase, 0x0138UL };     // DMA Mask Clear - DMA mask clear register
    static inline constexpr Reg32 DmaEnable { ScuBase, 0x013CUL };        // DMA Enable - DMA enable control register

    //----------------------------------------------------------------------
    // DSP registers - DSP control and communication
    //----------------------------------------------------------------------
    static inline constexpr Reg32 DspProgram { ScuBase, 0x0140UL };       // DSP Program Control - DSP program control register
    static inline constexpr Reg32 DspProgRam { ScuBase, 0x0144UL };       // DSP Program RAM - DSP program RAM access
    static inline constexpr Reg32 DspDataRam { ScuBase, 0x0148UL };       // DSP Data RAM - DSP data RAM access
    static inline constexpr Reg32 DspDataRamRead { ScuBase, 0x014CUL };   // DSP Data RAM Read - DSP data RAM read access
    static inline constexpr Reg32 DspExec { ScuBase, 0x0150UL };          // DSP Execution Control - DSP execution control register
    static inline constexpr Reg32 DspTimer { ScuBase, 0x0154UL };         // DSP Timer - DSP timer register
    static inline constexpr Reg32 DspControl { ScuBase, 0x0158UL };       // DSP Control - DSP control register
    static inline constexpr Reg32 DspVersion { ScuBase, 0x015CUL };       // DSP Version - DSP version register

    //----------------------------------------------------------------------
    // Interrupt registers - Interrupt control and status
    //----------------------------------------------------------------------
    static inline constexpr Reg32 InterruptMask { ScuBase, 0x0340UL };    // Interrupt Mask - Interrupt mask register
    static inline constexpr Reg32 InterruptStatus { ScuBase, 0x0344UL };  // Interrupt Status - Interrupt status register
    static inline constexpr Reg32 InterruptClear { ScuBase, 0x0348UL };   // Interrupt Clear - Interrupt clear register
    static inline constexpr Reg32 InterruptLevel { ScuBase, 0x034CUL };   // Interrupt Level - Interrupt level register
    static inline constexpr Reg32 InterruptMask2 { ScuBase, 0x0350UL };   // Interrupt Mask 2 - Additional interrupt mask register
    static inline constexpr Reg32 InterruptStatus2 { ScuBase, 0x0354UL }; // Interrupt Status 2 - Additional interrupt status register
    static inline constexpr Reg32 InterruptClear2 { ScuBase, 0x0358UL };  // Interrupt Clear 2 - Additional interrupt clear register
    static inline constexpr Reg32 InterruptLevel2 { ScuBase, 0x035CUL };  // Interrupt Level 2 - Additional interrupt level register

    //----------------------------------------------------------------------
    // A-Bus registers - A-Bus control and configuration
    //----------------------------------------------------------------------
    static inline constexpr Reg32 AbusRefresh { ScuBase, 0x0380UL };      // A-Bus Refresh - A-Bus refresh control register
    static inline constexpr Reg32 AddrSpaceCtrl0 { ScuBase, 0x03C0UL };   // Address Space Control 0 - Address space control register 0
    static inline constexpr Reg32 AddrSpaceCtrl1 { ScuBase, 0x03C4UL };   // Address Space Control 1 - Address space control register 1
    static inline constexpr Reg32 AddrSpaceCtrl2 { ScuBase, 0x03C8UL };   // Address Space Control 2 - Address space control register 2
    static inline constexpr Reg32 AddrSpaceCtrl3 { ScuBase, 0x03CCUL };   // Address Space Control 3 - Address space control register 3

    /**
     * @brief DMA Control register bit definitions
     */
    namespace DmaControl
    {
        // DMA start/stop control
        static inline constexpr std::uint32_t Start = 0x00000001UL;      // Start DMA transfer
        static inline constexpr std::uint32_t Pause = 0x00000010UL;      // Pause DMA transfer
        
        // Transfer direction
        static inline constexpr std::uint32_t DirRead = 0x00000000UL;    // Read from source to destination
        static inline constexpr std::uint32_t DirWrite = 0x00000002UL;   // Write from destination to source
        
        // Transfer size
        static inline constexpr std::uint32_t Size1Byte = 0x00000000UL;  // 1 byte transfer size
        static inline constexpr std::uint32_t Size2Byte = 0x00000004UL;  // 2 byte transfer size
        static inline constexpr std::uint32_t Size4Byte = 0x00000008UL;  // 4 byte transfer size
        static inline constexpr std::uint32_t Size8Byte = 0x0000000CUL;  // 8 byte transfer size
        static inline constexpr std::uint32_t Size16Byte = 0x00000010UL; // 16 byte transfer size
        static inline constexpr std::uint32_t Size32Byte = 0x00000014UL; // 32 byte transfer size
        
        // Address update mode
        static inline constexpr std::uint32_t SrcFixed = 0x00000100UL;   // Source address fixed
        static inline constexpr std::uint32_t DestFixed = 0x00000200UL;  // Destination address fixed
    }

    /**
     * @brief DMA Status register bit definitions
     */
    namespace DmaStatus
    {
        static inline constexpr std::uint32_t Dma0Busy = 0x00000001UL;   // DMA0 is busy
        static inline constexpr std::uint32_t Dma1Busy = 0x00000002UL;   // DMA1 is busy
        static inline constexpr std::uint32_t Dma2Busy = 0x00000004UL;   // DMA2 is busy
        static inline constexpr std::uint32_t Dma0End = 0x00000010UL;    // DMA0 has ended
        static inline constexpr std::uint32_t Dma1End = 0x00000020UL;    // DMA1 has ended
        static inline constexpr std::uint32_t Dma2End = 0x00000040UL;    // DMA2 has ended
    }

    /**
     * @brief Interrupt bit definitions
     */
    namespace Interrupts
    {
        // Level A interrupts (highest priority)
        static inline constexpr std::uint32_t VBlankIn = 0x00000001UL;   // VBlank-IN interrupt
        static inline constexpr std::uint32_t VBlankOut = 0x00000002UL;  // VBlank-OUT interrupt
        static inline constexpr std::uint32_t HBlankIn = 0x00000004UL;   // HBlank-IN interrupt
        static inline constexpr std::uint32_t Timer0 = 0x00000008UL;     // Timer 0 interrupt
        static inline constexpr std::uint32_t Timer1 = 0x00000010UL;     // Timer 1 interrupt
        static inline constexpr std::uint32_t Dsp = 0x00000020UL;        // DSP interrupt
        static inline constexpr std::uint32_t Sound = 0x00000040UL;      // Sound interrupt
        static inline constexpr std::uint32_t Smpc = 0x00000080UL;       // SMPC interrupt
        
        // Level B interrupts (medium priority)
        static inline constexpr std::uint32_t Pad = 0x00000100UL;        // PAD interrupt
        static inline constexpr std::uint32_t Dma0 = 0x00000200UL;       // DMA0 end interrupt
        static inline constexpr std::uint32_t Dma1 = 0x00000400UL;       // DMA1 end interrupt
        static inline constexpr std::uint32_t Dma2 = 0x00000800UL;       // DMA2 end interrupt
        static inline constexpr std::uint32_t Dma0Illegal = 0x00001000UL; // DMA0 illegal address interrupt
        static inline constexpr std::uint32_t Dma1Illegal = 0x00002000UL; // DMA1 illegal address interrupt
        static inline constexpr std::uint32_t Dma2Illegal = 0x00004000UL; // DMA2 illegal address interrupt
        
        // Level C interrupts (lowest priority)
        static inline constexpr std::uint32_t External0 = 0x00010000UL;  // External interrupt 0
        static inline constexpr std::uint32_t External1 = 0x00020000UL;  // External interrupt 1
        static inline constexpr std::uint32_t External2 = 0x00040000UL;  // External interrupt 2
        static inline constexpr std::uint32_t External3 = 0x00080000UL;  // External interrupt 3
        static inline constexpr std::uint32_t External4 = 0x00100000UL;  // External interrupt 4
        static inline constexpr std::uint32_t External5 = 0x00200000UL;  // External interrupt 5
        static inline constexpr std::uint32_t External6 = 0x00400000UL;  // External interrupt 6
        static inline constexpr std::uint32_t External7 = 0x00800000UL;  // External interrupt 7
    }
}
