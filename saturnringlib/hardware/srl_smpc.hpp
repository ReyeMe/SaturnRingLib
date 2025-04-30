#pragma once
/**
 * @file srl_smpc.hpp
 * @brief SMPC (System Manager and Peripheral Control) Hardware Interface
 * 
 * This file provides direct access to the Sega Saturn SMPC hardware registers.
 * The SMPC handles system management functions like power control, reset,
 * and peripheral I/O including controllers and memory cards.
 * 
 * @note This provides a low-level direct hardware access alternative to SGL functions
 */

#include "srl_hardware_common.hpp"

namespace SRL::Hardware::SMPC
{
    using namespace MemoryRegions;

    //----------------------------------------------------------------------
    // Command and status registers - System control
    //----------------------------------------------------------------------
    static inline constexpr Reg8 Command { SmpcBase, 0x0001UL };        // Command register - Sends commands to SMPC
    static inline constexpr Reg8 Status { SmpcBase, 0x0003UL };         // Status register - Shows SMPC status
    static inline constexpr Reg8 ServiceRequest { SmpcBase, 0x0005UL }; // Service Request - Triggers SMPC command execution
    static inline constexpr Reg8 Result1 { SmpcBase, 0x0007UL };        // Result 1 - First result byte
    static inline constexpr Reg8 Result2 { SmpcBase, 0x0009UL };        // Result 2 - Second result byte
    static inline constexpr Reg8 Result3 { SmpcBase, 0x000BUL };        // Result 3 - Third result byte
    static inline constexpr Reg8 Result4 { SmpcBase, 0x000DUL };        // Result 4 - Fourth result byte
    static inline constexpr Reg8 Result5 { SmpcBase, 0x000FUL };        // Result 5 - Fifth result byte
    static inline constexpr Reg8 Result6 { SmpcBase, 0x0011UL };        // Result 6 - Sixth result byte
    static inline constexpr Reg8 Result7 { SmpcBase, 0x0013UL };        // Result 7 - Seventh result byte
    static inline constexpr Reg8 Result8 { SmpcBase, 0x0015UL };        // Result 8 - Eighth result byte
    static inline constexpr Reg8 Result9 { SmpcBase, 0x0017UL };        // Result 9 - Ninth result byte
    static inline constexpr Reg8 Result10 { SmpcBase, 0x0019UL };       // Result 10 - Tenth result byte
    static inline constexpr Reg8 Result11 { SmpcBase, 0x001BUL };       // Result 11 - Eleventh result byte
    static inline constexpr Reg8 Result12 { SmpcBase, 0x001DUL };       // Result 12 - Twelfth result byte
    static inline constexpr Reg8 Result13 { SmpcBase, 0x001FUL };       // Result 13 - Thirteenth result byte
    static inline constexpr Reg8 Result14 { SmpcBase, 0x0021UL };       // Result 14 - Fourteenth result byte
    static inline constexpr Reg8 Result15 { SmpcBase, 0x0023UL };       // Result 15 - Fifteenth result byte
    static inline constexpr Reg8 Result16 { SmpcBase, 0x0025UL };       // Result 16 - Sixteenth result byte

    //----------------------------------------------------------------------
    // Peripheral data registers - Controller and device I/O
    //----------------------------------------------------------------------
    static inline constexpr Reg8 PortData1 { SmpcBase, 0x0027UL };      // Port 1 Data - Data from peripheral port 1
    static inline constexpr Reg8 PortData2 { SmpcBase, 0x0029UL };      // Port 2 Data - Data from peripheral port 2
    static inline constexpr Reg8 PortData3 { SmpcBase, 0x002BUL };      // Port 3 Data - Data from peripheral port 3
    static inline constexpr Reg8 PortData4 { SmpcBase, 0x002DUL };      // Port 4 Data - Data from peripheral port 4
    static inline constexpr Reg8 PortData5 { SmpcBase, 0x002FUL };      // Port 5 Data - Data from peripheral port 5
    static inline constexpr Reg8 PortData6 { SmpcBase, 0x0031UL };      // Port 6 Data - Data from peripheral port 6
    static inline constexpr Reg8 PortData7 { SmpcBase, 0x0033UL };      // Port 7 Data - Data from peripheral port 7
    static inline constexpr Reg8 PortData8 { SmpcBase, 0x0035UL };      // Port 8 Data - Data from peripheral port 8
    static inline constexpr Reg8 PortData9 { SmpcBase, 0x0037UL };      // Port 9 Data - Data from peripheral port 9
    static inline constexpr Reg8 PortData10 { SmpcBase, 0x0039UL };     // Port 10 Data - Data from peripheral port 10
    static inline constexpr Reg8 PortData11 { SmpcBase, 0x003BUL };     // Port 11 Data - Data from peripheral port 11
    static inline constexpr Reg8 PortData12 { SmpcBase, 0x003DUL };     // Port 12 Data - Data from peripheral port 12
    static inline constexpr Reg8 PortData13 { SmpcBase, 0x003FUL };     // Port 13 Data - Data from peripheral port 13
    static inline constexpr Reg8 PortData14 { SmpcBase, 0x0041UL };     // Port 14 Data - Data from peripheral port 14
    static inline constexpr Reg8 PortData15 { SmpcBase, 0x0043UL };     // Port 15 Data - Data from peripheral port 15

    /**
     * @brief SMPC command codes - System management operations
     */
    namespace CommandCodes
    {
        static inline constexpr std::uint8_t MasterSh2On = 0x00U;      // Turn on Master SH-2 CPU
        static inline constexpr std::uint8_t SlaveSh2On = 0x02U;       // Turn on Slave SH-2 CPU
        static inline constexpr std::uint8_t SlaveSh2Off = 0x03U;      // Turn off Slave SH-2 CPU
        static inline constexpr std::uint8_t SoundOn = 0x06U;          // Turn on sound system
        static inline constexpr std::uint8_t SoundOff = 0x07U;         // Turn off sound system
        static inline constexpr std::uint8_t SystemReset = 0x0DU;      // Reset the entire system
        static inline constexpr std::uint8_t NmiRequest = 0x18U;       // Request NMI (Non-Maskable Interrupt)
        static inline constexpr std::uint8_t ReadStatus = 0x1AU;       // Read system status
        static inline constexpr std::uint8_t ReadOrbitraceData = 0x1DU; // Read Orbitrace data (clock/calendar)
        static inline constexpr std::uint8_t ReadPeripheralData = 0x1FU; // Read data from peripherals
    }

    /**
     * @brief SMPC status register bit definitions
     */
    namespace StatusBits
    {
        static inline constexpr std::uint8_t Busy = 0x01U;             // SMPC is busy processing a command
        static inline constexpr std::uint8_t PdiReady = 0x02U;         // Peripheral data is ready
        static inline constexpr std::uint8_t OreReady = 0x04U;         // Orbitrace data is ready
        static inline constexpr std::uint8_t SteReady = 0x08U;         // System status is ready
    }

    /**
     * @brief Controller data structure
     * 
     * This structure defines the format of data returned from a standard Saturn controller.
     */
    struct ControllerData
    {
        std::uint8_t id;             // Peripheral ID (0x02 for digital pad, 0x16 for analog pad)
        std::uint8_t size;           // Data size (0x02 for digital pad, 0x06 for analog pad)
        std::uint8_t data[6];        // Controller data (buttons, analog values)
        
        // Digital pad button masks
        static inline constexpr std::uint8_t ButtonRight = 0x01;       // D-Pad Right
        static inline constexpr std::uint8_t ButtonLeft = 0x02;        // D-Pad Left
        static inline constexpr std::uint8_t ButtonDown = 0x04;        // D-Pad Down
        static inline constexpr std::uint8_t ButtonUp = 0x08;          // D-Pad Up
        static inline constexpr std::uint8_t ButtonStart = 0x10;       // Start button
        static inline constexpr std::uint8_t ButtonA = 0x01;           // A button
        static inline constexpr std::uint8_t ButtonB = 0x02;           // B button
        static inline constexpr std::uint8_t ButtonC = 0x04;           // C button
        static inline constexpr std::uint8_t ButtonX = 0x08;           // X button
        static inline constexpr std::uint8_t ButtonY = 0x10;           // Y button
        static inline constexpr std::uint8_t ButtonZ = 0x20;           // Z button
        static inline constexpr std::uint8_t ButtonR = 0x40;           // R trigger
        static inline constexpr std::uint8_t ButtonL = 0x80;           // L trigger
    };

    /**
     * @brief Orbitrace (RTC) data structure
     * 
     * This structure defines the format of data returned from the Orbitrace (Real-Time Clock).
     */
    struct OrbitraceData
    {
        std::uint8_t year;           // Year (last two digits, BCD format)
        std::uint8_t month;          // Month (BCD format)
        std::uint8_t day;            // Day (BCD format)
        std::uint8_t dayOfWeek;      // Day of week (0-6, 0=Sunday)
        std::uint8_t hour;           // Hour (BCD format, 24-hour)
        std::uint8_t minute;         // Minute (BCD format)
        std::uint8_t second;         // Second (BCD format)
    };
}
