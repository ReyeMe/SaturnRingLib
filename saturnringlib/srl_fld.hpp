#pragma once

#include "srl_base.hpp"
#include "srl_cd.hpp"
#include "srl_interrupt.hpp"
#include "srl_system.hpp"

#include <sega_sys.h>

extern "C"
{
    /** @brief FLD kernel linked into the program (modules/fld/fld_kernel.S), present when SRL_USE_FLD = 1 */
    extern const uint8_t FldKernel[];

    /** @brief End of the FLD kernel */
    extern const uint8_t FldKernelEnd[];
}

namespace SRL
{
    /** @brief File loader (FLD): starts other programs stored on the same disc, like SEGA's "Demo-Demo" discs.
     * @details A menu program calls ExecGame() with the IP file and the directory of a game. The game boots as
     * if it were alone on the disc: its directory is its root directory (GFS reads the DDS block the loader
     * writes), its first CD-DA track is the one the menu gave (SRL::Sound::Cdda takes care of it), and it can
     * go back to the menu with ReturnToMenu() or the Reset button (see ReturnToMenuOnReset()).
     *
     * Disc layout:
     * @code
     * 0.BIN         <- menu program (first file, as usual)
     * GAME1.BIN     <- IP.BIN of game 1
     * GAME1/0.BIN   <- game 1 program, must be the first file of its directory
     * GAME1/...     <- game 1 files
     * @endcode
     *
     * The menu needs `SRL_USE_FLD = 1` in its makefile: it links the FLD kernel into the program
     * (`SRL_FLD_KERNEL` picks the kernel binary). Games only need this header.
     * @note Needs the disc: it does not work with programs uploaded through a dev cartridge.
     */
    class Fld final
    {
    private:
        /** @brief Where the BIOS keeps the IP of the disc it booted, and where the game IP is loaded */
        static constexpr uintptr_t IpAddress = 0x06002000;

        /** @brief Largest game IP: it must end before the menu program, which starts at 0x06004000 */
        static constexpr int32_t IpMaxSize = 0x2000;

        /** @brief Entry of the IP code (after its 0x100 bytes of header) */
        static constexpr uintptr_t IpCodeAddress = 0x06002100;

        /** @brief Stack top the BIOS gives the IP code at boot (default master stack) */
        static constexpr uintptr_t BootStackAddress = 0x06002000;

        /** @brief Vector the IP code calls to load the first program file: the BIOS loader by default */
        static constexpr uintptr_t IpLoaderVector = 0x06002270;

        /** @brief BIOS vector of the reset/exit path (Reset button, SYS_Exit), and where the BIOS keeps the address
         * the IP code copies its hook routine to. The IP points the vector at that routine, which calls the
         * vector it replaced, saved at HookChainOffset in the routine */
        static constexpr uintptr_t BiosExitVector = 0x060002DC;
        static constexpr uintptr_t BiosExitHookAddress = 0x06000260;
        static constexpr uintptr_t HookChainOffset = 0x54;

        /** @brief Where the FLD kernel runs (low work RAM) */
        static constexpr uintptr_t KernelAddress = 0x00200000;

        /** @brief BIOS copy of the system ID (IP header) of the disc */
        static constexpr uintptr_t BiosSystemId = 0x06000C00;

        /** @brief BIOS area copied into the IP header from offset 0xE0 (IP size, stacks, 1st read address and size) */
        static constexpr uintptr_t BiosBootInfo = 0x060002A0;

        /** @brief SH-2 cache control register */
        static constexpr uintptr_t CacheControlRegister = 0xFFFFFE92;

        /** @brief SMPC command register, and its status flag (bit 0 set while a command runs) */
        static constexpr uintptr_t SmpcCommandRegister = 0x2010001F;
        static constexpr uintptr_t SmpcStatusFlag = 0x20100063;

        /** @brief SMPC command SSHOFF: holds the slave CPU in reset */
        static constexpr uint8_t SmpcSlaveOff = 0x03;

        /** @brief Offset of the NMI vector (vector 11, sent by the Reset button) in the vector table */
        static constexpr uintptr_t NmiVectorOffset = 11 * 4;

        /** @brief NMI handler of a game started by the menu: goes back to the menu instead of the BIOS reset */
        [[noreturn]] static void ResetHandler()
        {
            Fld::ReturnToMenu();
        }

        /** @brief SH-2 interrupt priority registers: IPRA (DIVU, DMAC, WDT) and IPRB (SCI, FRT) */
        static constexpr uintptr_t OnChipPriorityA = 0xFFFFFEE2;
        static constexpr uintptr_t OnChipPriorityB = 0xFFFFFE60;

    public:
        Fld() = delete;

        /** @brief Clears the DDS block (SEGA's FLD_INIT_DDS), so the menu sees the whole disc.
         * @details Call it in the menu program before SRL::Core::Initialize(). Games must not call it.
         */
        static void InitializeMenu()
        {
            *reinterpret_cast<volatile uint32_t *>(GFS_DDS_ADDR) = 0x20202020;
        }

        /** @brief Starts a game of the disc.
         * @param ipFile IP file of the game, in the current directory (usually the root directory)
         * @param gameDir Directory of the game, in the current directory: its first file is the game program
         * @param firstAudioTrack Disc track the game's first CD-DA track is on (track 2 when the game is alone)
         * @return false if the IP file or the directory is missing, gameDir is not a directory or the IP is too big;
         * on success it does not return
         */
        static bool ExecGame(const char *ipFile, const char *gameDir, const uint8_t firstAudioTrack)
        {
            const int32_t ipId = GFS_NameToId((int8_t *)ipFile);
            const int32_t dirId = GFS_NameToId((int8_t *)gameDir);
            GfsDirId dirRecord;

            if (ipId < 0 || dirId < 0 || GFS_GetDirInfo(dirId, &dirRecord) < 0 || (GFS_DIR_ATR(&dirRecord) & GFS_ATR_DIR) == 0 ||
                Cd::File(ipFile).Size.Bytes > IpMaxSize || GFS_Load(ipId, 0, reinterpret_cast<void *>(IpAddress), IpMaxSize) < 0)
            {
                return false;
            }

            // The IP header comes from the booted disc, as the BIOS checks it
            memcpy(reinterpret_cast<void *>(IpAddress), reinterpret_cast<const void *>(BiosSystemId), 0x100);
            memcpy(reinterpret_cast<void *>(IpAddress + 0xE0), reinterpret_cast<const void *>(BiosBootInfo), 0x20);

            // DDS block: the game's GFS takes gameDir as its root directory
            *reinterpret_cast<volatile uint8_t *>(GFS_DDS_ADDR) = GFS_DDS_EXEC;
            *reinterpret_cast<volatile uint8_t *>(GFS_DDS_ADDR + 1) = firstAudioTrack;
            *reinterpret_cast<volatile uint16_t *>(GFS_DDS_ADDR + 2) = static_cast<uint16_t>(dirId);

            // From here the menu is gone: nothing may run on top of the kernel, which overwrites the low work RAM heap
            Interrupt::SetMask(Interrupt::Mask::All);

            // The slave CPU runs SGL's idle loop inside the menu program, which the game overwrites: hold it in reset.
            // slSlaveOffWait() left it running (it then ran the game's code and could corrupt the BIOS vectors), so
            // the command goes to the SMPC directly
            volatile uint8_t &smpcStatus = *reinterpret_cast<volatile uint8_t *>(SmpcStatusFlag);
            while ((smpcStatus & 1) != 0) { }
            smpcStatus = 1;
            *reinterpret_cast<volatile uint8_t *>(SmpcCommandRegister) = SmpcSlaveOff;
            while ((smpcStatus & 1) != 0) { }

            // The SCU mask does not stop SH-2 on-chip interrupts (the FRT of SRL::Timer...): priority 0 does
            *reinterpret_cast<volatile uint16_t *>(OnChipPriorityA) = 0;
            *reinterpret_cast<volatile uint16_t *>(OnChipPriorityB) = 0;

            // The menu's IP already hooked the exit vector: running the game's IP again would make the hook call
            // itself, and Reset or ReturnToMenu() would then loop forever. Put back the vector it replaced.
            // shortcut: HookChainOffset is the layout of SEGA's IP code in SRL's IP.BIN, check it for another IP
            const uintptr_t hook = *reinterpret_cast<volatile uint32_t *>(BiosExitHookAddress);
            volatile uint32_t &exitVector = *reinterpret_cast<volatile uint32_t *>(BiosExitVector);

            if (exitVector == hook)
            {
                exitVector = *reinterpret_cast<volatile uint32_t *>(hook + HookChainOffset);
            }

            memcpy(reinterpret_cast<void *>(KernelAddress), FldKernel, FldKernelEnd - FldKernel);
            *reinterpret_cast<volatile uint32_t *>(IpLoaderVector) = KernelAddress;
            *reinterpret_cast<volatile uint8_t *>(CacheControlRegister) |= 0x10; // purge: the IP was written by DMA

            // The game may load up to 0x06100000, over the menu stack (SGL's MasterStack): run the IP code and the
            // kernel on the stack the BIOS uses at boot, below the IP
            __asm__ volatile("mov %0,r15\n\tjmp @%1\n\tnop" : : "r"(BootStackAddress), "r"(IpCodeAddress));
            __builtin_unreachable();
        }

        /** @brief Whether this program was started by an FLD menu
         * @return true when started by Fld::ExecGame()
         */
        static bool IsRunningFromMenu()
        {
            return GFS_IS_DDS();
        }

        /** @brief Disc track of this program's first CD-DA track
         * @return The track the menu gave, or 2 when the program is alone on its disc
         */
        static uint8_t FirstAudioTrack()
        {
            return Fld::IsRunningFromMenu() ? GFS_DDS_TNO() : 2;
        }

        /** @brief Goes back to the menu that started this program (or to the CD player if there is none)
         * @details The BIOS checks the disc again and boots it, which starts the menu.
         */
        [[noreturn]] static void ReturnToMenu()
        {
            System::Exit(0);
        }

        /** @brief Makes the Reset button go back to the menu
         * @details The BIOS answers the Reset button with a full system reset, which boots a cartridge (a dev
         * cartridge menu...) before the disc. This points the NMI the Reset button sends to ReturnToMenu() instead.
         * SRL::Core::Initialize() calls it when the program was started by a menu.
         */
        static void ReturnToMenuOnReset()
        {
            uintptr_t vbr;
            __asm__ volatile("stc vbr,%0" : "=r"(vbr));
            *reinterpret_cast<volatile uintptr_t *>(vbr + NmiVectorOffset) = reinterpret_cast<uintptr_t>(&Fld::ResetHandler);
        }
    };
}
