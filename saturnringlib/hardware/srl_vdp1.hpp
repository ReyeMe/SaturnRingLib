#pragma once
/**
 * @file srl_vdp1.hpp
 * @brief VDP1 (Video Display Processor 1) Hardware Interface
 * 
 * This file provides direct access to the Sega Saturn VDP1 hardware registers.
 * The VDP1 is responsible for sprite and polygon rendering on the Saturn.
 * 
 * @note This provides a low-level direct hardware access alternative to SGL functions
 */

#include "srl_hardware_common.hpp"

namespace SRL::Hardware::VDP1
{
    using namespace MemoryRegions;

    /**
     * @brief VDP1 hardware registers for sprite and polygon rendering
     * 
     * The VDP1 (Video Display Processor 1) is primarily responsible for sprites,
     * polygons, and other graphical objects on the Saturn hardware.
     */
    
    // Display and frame buffer control registers
    static inline constexpr Reg16 TvMode { Vdp1Base, 0x0000UL };            // TV Mode (TVMR)
    static inline constexpr Reg16 FrameBufferChange { Vdp1Base, 0x0002UL }; // Frame Buffer Change (FBCR)
    static inline constexpr Reg16 PlotTrigger { Vdp1Base, 0x0004UL };       // Plot Trigger (PTMR)
    static inline constexpr Reg16 EraseWriteData { Vdp1Base, 0x0006UL };    // Erase/Write Data (EWDR)
    static inline constexpr Reg16 EraseWriteLower { Vdp1Base, 0x0008UL };   // Erase/Write Lower (EWLR)
    static inline constexpr Reg16 EraseWriteUpper { Vdp1Base, 0x000AUL };   // Erase/Write Upper (EWRR)
    static inline constexpr Reg16 EndStatus { Vdp1Base, 0x000CUL };         // End Status (ENDR)

    // Transfer and command status registers
    static inline constexpr Reg16 TransferEndStatus { Vdp1Base, 0x0010UL }; // Transfer End Status (EDSR)
    static inline constexpr Reg16 LastOpCmdAddr { Vdp1Base, 0x0012UL };     // Last Operation Command Address (LOPR)
    static inline constexpr Reg16 CurrentOpCmdAddr { Vdp1Base, 0x0014UL };  // Current Operation Command Address (COPR)
    static inline constexpr Reg16 ModeStatus { Vdp1Base, 0x0016UL };        // Mode Status (MODR)

    /**
     * @brief TV Mode register bit definitions
     */
    namespace TVMode
    {
        static inline constexpr std::uint16_t Normal = 0x0000;           // Normal display mode
        static inline constexpr std::uint16_t Rotate = 0x0008;           // Rotated display mode
        static inline constexpr std::uint16_t VerticalBlankErase = 0x0008; // Vertical blank erase
    }

    /**
     * @brief Frame Buffer Control register bit definitions
     */
    namespace FrameBufferControl
    {
        static inline constexpr std::uint16_t None = 0x0000;                // No operation
        static inline constexpr std::uint16_t FrameChangeType = 0x0001;     // Setting prohibited
        static inline constexpr std::uint16_t FrameChangeMode = 0x0002;     // Erase in the next field
        static inline constexpr std::uint16_t FrameChangeComplete = 0x0003; // Change in next field
        static inline constexpr std::uint16_t DisplayLines = 0x0004;        // Display interlaced lines
        static inline constexpr std::uint16_t DisplayEnable = 0x0008;       // Display enable
        static inline constexpr std::uint16_t EndOfTransfer = 0x0010;       // End of transfer status
    }

    /**
     * @brief Command table structure for VDP1 drawing commands
     * 
     * This structure defines the memory layout of a VDP1 command table entry.
     * Each command consists of 32 bytes (16 words) that define a drawing operation.
     */
    struct CommandTable
    {
        std::uint16_t cmdCtrl;     // Command Control Word
        std::uint16_t cmdLink;     // Link/Jump Command
        std::uint16_t cmdPmod;     // Draw Mode Word
        std::uint16_t cmdColr;     // Color Control Word
        std::uint16_t cmdSrca;     // Character Address
        std::uint16_t cmdSize;     // Character Size
        std::int16_t  cmdXa;       // Vertex A X Coordinate
        std::int16_t  cmdYa;       // Vertex A Y Coordinate
        std::int16_t  cmdXb;       // Vertex B X Coordinate
        std::int16_t  cmdYb;       // Vertex B Y Coordinate
        std::int16_t  cmdXc;       // Vertex C X Coordinate
        std::int16_t  cmdYc;       // Vertex C Y Coordinate
        std::int16_t  cmdXd;       // Vertex D X Coordinate
        std::int16_t  cmdYd;       // Vertex D Y Coordinate
        std::uint16_t cmdGrda;     // Gradient Data
        std::uint16_t reserved;    // Reserved
    };

    /**
     * @brief Command Control Word bit definitions
     */
    namespace CommandControl
    {
        // Command types
        static inline constexpr std::uint16_t CmdNormalSprite = 0x0000;     // Normal Sprite
        static inline constexpr std::uint16_t CmdScaledSprite = 0x0001;     // Scaled Sprite
        static inline constexpr std::uint16_t CmdDistortedSprite = 0x0002;  // Distorted Sprite
        static inline constexpr std::uint16_t CmdPolygon = 0x0004;          // Polygon
        static inline constexpr std::uint16_t CmdPolyline = 0x0005;         // Polyline
        static inline constexpr std::uint16_t CmdLine = 0x0006;             // Line
        static inline constexpr std::uint16_t CmdUserClipping = 0x0008;     // User Clipping
        static inline constexpr std::uint16_t CmdSystemClipping = 0x0009;   // System Clipping
        static inline constexpr std::uint16_t CmdLocalCoordinate = 0x000A;  // Local Coordinate
        static inline constexpr std::uint16_t CmdDrawEnd = 0x000B;          // Draw End

        // End bit flags
        static inline constexpr std::uint16_t EndCommandList = 0x8000;      // End of command list
        static inline constexpr std::uint16_t SkipNextCommand = 0x4000;     // Skip next command
    }

    /**
     * @brief Draw Mode Word bit definitions
     */
    namespace DrawMode
    {
        // Texture settings
        static inline constexpr std::uint16_t TextureBit = 0x0100;          // Use texture
        static inline constexpr std::uint16_t TransparentBit = 0x1000;      // Enable transparency
        static inline constexpr std::uint16_t EndCodeDisable = 0x0200;      // Disable end code detection

        // Color mode
        static inline constexpr std::uint16_t ColorMode16Bit = 0x0000;      // 16-bit direct color
        static inline constexpr std::uint16_t ColorMode16BitLookup = 0x0004; // 16-bit lookup table
        static inline constexpr std::uint16_t ColorMode8Bit = 0x0008;       // 8-bit color bank
        static inline constexpr std::uint16_t ColorMode4Bit = 0x000C;       // 4-bit color bank

        // Gouraud shading
        static inline constexpr std::uint16_t GouraudShading = 0x0004;      // Enable Gouraud shading
    }
}
