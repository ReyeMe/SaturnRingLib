#pragma once
/**
 * @file srl_vdp2.hpp
 * @brief VDP2 (Video Display Processor 2) Hardware Interface
 * 
 * This file provides direct access to the Sega Saturn VDP2 hardware registers.
 * The VDP2 is responsible for backgrounds, scrolling planes, and other
 * advanced graphics features of the Saturn hardware.
 * 
 * @note This provides a low-level direct hardware access alternative to SGL functions
 */

#include "srl_hardware_common.hpp"

namespace SRL::Hardware::VDP2
{
    using namespace MemoryRegions;

    /**
     * @brief VDP2 VRAM constants
     */
    namespace VRAM
    {
        static constexpr std::uintptr_t TotalSize = 0x00080000UL;  // 512 KiB total VRAM
        static constexpr std::uintptr_t HalfSize = TotalSize / 2;   // 256 KiB (half of VRAM)
        static constexpr std::uintptr_t QuarterSize = TotalSize / 4; // 128 KiB (quarter of VRAM)
    }

    //----------------------------------------------------------------------
    // Core system registers - Basic display control and status
    //----------------------------------------------------------------------
    static inline constexpr Reg16 TvMode { Vdp2Base, 0x0000UL };          // TV Mode - Screen resolution and interlace settings
    static inline constexpr Reg16 ExternalEnable { Vdp2Base, 0x0002UL };  // External Signal Enable - Video input control
    static inline constexpr Reg16 TvStatus { Vdp2Base, 0x0004UL };        // TV Status - Display status flags (read-only)
    static inline constexpr Reg16 VRamSize { Vdp2Base, 0x0006UL };        // VRAM Size - Configured memory size settings
    static inline constexpr Reg16 HCounter { Vdp2Base, 0x0008UL };        // H-Counter - Horizontal scan position (read-only)
    static inline constexpr Reg16 VCounter { Vdp2Base, 0x000AUL };        // V-Counter - Vertical scan line position (read-only)
    static inline constexpr Reg16 RamControl { Vdp2Base, 0x000EUL };      // RAM Control - VRAM bank access configuration

    //----------------------------------------------------------------------
    // VRAM access pattern registers - Memory access cycle patterns
    //----------------------------------------------------------------------
    static inline constexpr Reg16 CycleA0Lower { Vdp2Base, 0x0010UL };    // VRAM Cycle Pattern A0 Lower Word
    static inline constexpr Reg16 CycleA0Upper { Vdp2Base, 0x0012UL };    // VRAM Cycle Pattern A0 Upper Word
    static inline constexpr Reg16 CycleA1Lower { Vdp2Base, 0x0014UL };    // VRAM Cycle Pattern A1 Lower Word
    static inline constexpr Reg16 CycleA1Upper { Vdp2Base, 0x0016UL };    // VRAM Cycle Pattern A1 Upper Word
    static inline constexpr Reg16 CycleB0Lower { Vdp2Base, 0x0018UL };    // VRAM Cycle Pattern B0 Lower Word
    static inline constexpr Reg16 CycleB0Upper { Vdp2Base, 0x001AUL };    // VRAM Cycle Pattern B0 Upper Word
    static inline constexpr Reg16 CycleB1Lower { Vdp2Base, 0x001CUL };    // VRAM Cycle Pattern B1 Lower Word
    static inline constexpr Reg16 CycleB1Upper { Vdp2Base, 0x001EUL };    // VRAM Cycle Pattern B1 Upper Word

    //----------------------------------------------------------------------
    // Background control registers - Layer and special effect controls
    //----------------------------------------------------------------------
    static inline constexpr Reg16 BgEnable { Vdp2Base, 0x0020UL };        // Background Display Enable - Controls layer visibility
    static inline constexpr Reg16 PriorityNbg0 { Vdp2Base, 0x0022UL };    // NBG0 Priority - Normal BG0 display priority
    static inline constexpr Reg16 PriorityNbg1 { Vdp2Base, 0x0024UL };    // NBG1 Priority - Normal BG1 display priority
    static inline constexpr Reg16 PriorityNbg2 { Vdp2Base, 0x0026UL };    // NBG2 Priority - Normal BG2 display priority
    static inline constexpr Reg16 PriorityNbg3 { Vdp2Base, 0x0028UL };    // NBG3 Priority - Normal BG3 display priority
    static inline constexpr Reg16 PriorityRbg0 { Vdp2Base, 0x002AUL };    // RBG0 Priority - Rotational BG0 display priority
    static inline constexpr Reg16 ColorOffset { Vdp2Base, 0x002CUL };     // Color Offset Enable - Controls color offset processing
    static inline constexpr Reg16 ColorCalcControl { Vdp2Base, 0x002EUL }; // Color Calculation Control - Special color effects

    /**
     * @brief TV Mode register bit definitions
     */
    namespace TVMode
    {
        // Horizontal resolution settings
        static inline constexpr std::uint16_t HRes320 = 0x0000;    // 320 pixel width (normal)
        static inline constexpr std::uint16_t HRes352 = 0x0001;    // 352 pixel width
        static inline constexpr std::uint16_t HRes640 = 0x0002;    // 640 pixel width (hi-res)
        static inline constexpr std::uint16_t HRes704 = 0x0003;    // 704 pixel width
        static inline constexpr std::uint16_t HResMask = 0x0003;   // Mask for horizontal resolution bits
        
        // Vertical resolution and interlace settings
        static inline constexpr std::uint16_t VResNormal = 0x0000; // Normal vertical resolution
        static inline constexpr std::uint16_t VResInterlace = 0x0004; // Interlaced display
        static inline constexpr std::uint16_t VResDoubleDensity = 0x0008; // Double-density interlace
        static inline constexpr std::uint16_t VResMask = 0x000C;   // Mask for vertical resolution bits
        
        // TV system settings
        static inline constexpr std::uint16_t TvNTSC = 0x0000;     // NTSC (60Hz)
        static inline constexpr std::uint16_t TvPAL = 0x0010;      // PAL (50Hz)
        static inline constexpr std::uint16_t TvMask = 0x0010;     // Mask for TV system bit
    }

    /**
     * @brief Background Enable register bit definitions
     */
    namespace BgEnableBits
    {
        static inline constexpr std::uint16_t NBG0 = 0x0001;       // Normal Background 0
        static inline constexpr std::uint16_t NBG1 = 0x0002;       // Normal Background 1
        static inline constexpr std::uint16_t NBG2 = 0x0004;       // Normal Background 2
        static inline constexpr std::uint16_t NBG3 = 0x0008;       // Normal Background 3
        static inline constexpr std::uint16_t RBG0 = 0x0010;       // Rotational Background 0
        static inline constexpr std::uint16_t RotationMode = 0x0020; // Rotation mode (RBG0 or RBG1)
        static inline constexpr std::uint16_t VDP1DisplayEnable = 0x0100; // Enable VDP1 display
        static inline constexpr std::uint16_t VDP1EraseModeNone = 0x0000; // No VDP1 frame erase
        static inline constexpr std::uint16_t VDP1EraseModeAuto = 0x0200; // Automatic VDP1 frame erase
        static inline constexpr std::uint16_t VDP1EraseModeManual = 0x0300; // Manual VDP1 frame erase
    }

    /**
     * @brief Color Calculation Control register bit definitions
     */
    namespace ColorCalc
    {
        // Color calculation target layers
        static inline constexpr std::uint16_t CalcNBG0 = 0x0001;   // Include NBG0 in color calculation
        static inline constexpr std::uint16_t CalcNBG1 = 0x0002;   // Include NBG1 in color calculation
        static inline constexpr std::uint16_t CalcNBG2 = 0x0004;   // Include NBG2 in color calculation
        static inline constexpr std::uint16_t CalcNBG3 = 0x0008;   // Include NBG3 in color calculation
        static inline constexpr std::uint16_t CalcRBG0 = 0x0010;   // Include RBG0 in color calculation
        static inline constexpr std::uint16_t CalcSprite = 0x0020; // Include sprites in color calculation
        
        // Color calculation modes
        static inline constexpr std::uint16_t ModeNone = 0x0000;   // No special color processing
        static inline constexpr std::uint16_t ModeShadow = 0x0100; // Shadow processing
        static inline constexpr std::uint16_t ModeHalfLuminance = 0x0200; // Half-luminance processing
        static inline constexpr std::uint16_t ModeHalfTransparent = 0x0300; // Half-transparent processing
    }
}
