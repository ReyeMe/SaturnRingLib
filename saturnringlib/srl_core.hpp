#pragma once

#include "srl_memory.hpp"
#include "srl_event.hpp"
#include "srl_tv.hpp"
#include "srl_color.hpp"
#include "srl_cd.hpp"
#include "srl_vdp1.hpp"
#include "srl_vdp2.hpp"
#include "srl_input.hpp"
#include "srl_slave.hpp"

#if SRL_USE_SGL_SOUND_DRIVER == 1
    #include "srl_sound.hpp"
#endif

namespace SRL
{
    /** @brief Core functions of the library
     */
    class Core
    {
    public:
        /** @brief Event triggered every v-blank
         */
        inline static SRL::Types::Event<> OnVblank;

    private:

        /** @brief Handle V-Blank events
         */
        inline static void VblankHandling()
        {
            slGetStatus();
            SRL::Input::Management::RefreshPeripherals();
            SRL::Input::Gun::VblankRefresh();
            Core::OnVblank.Invoke();
        }

    public:

        /** @brief Initialize basic environment
         * @param backColor Color of the screen
         */
        inline static void Initialize(const Types::HighColor& backColor)
        {
            SRL::Memory::Initialize();

#if defined(SRL_FRAMERATE) && (SRL_FRAMERATE > 0)
            slInitSystem((uint16_t)SRL::TV::Resolution, SRL::VDP1::Textures->SglPtr(), SRL_FRAMERATE);
#elif defined(SRL_FRAMERATE) && (SRL_FRAMERATE == 0)
            slInitSystem((uint16_t)SRL::TV::Resolution, SRL::VDP1::Textures->SglPtr(), -1);
            slDynamicFrame(ON);
            SynchConst = 1;
#else
            slInitSystem((uint16_t)SRL::TV::Resolution, SRL::VDP1::Textures->SglPtr(), -SRL_FRAMERATE);
            slDynamicFrame(ON);
            SynchConst = (uint8_t)(-SRL_FRAMERATE);
#endif
            // Initialize CD drive
            SRL::Cd::Initialize();

            // Initialize callbacks
            slIntFunction(Core::VblankHandling);

            // Start initializing stuff
            slTVOff();

            // Initialize VDP2
            VDP2::Initialize(backColor);

            // Set window size to 512 (this allows us to have VDP1 render outside of the visible area)
            *(uint16_t*)(SpriteVRAM+20)=511;

            // Set cull depth
            slZdspLevel(3);

            // Load sound driver
#if SRL_USE_SGL_SOUND_DRIVER == 1
            SRL::Sound::Hardware::Initialize();
#endif

            // All was initialized
            slTVOn();
        }

        /** @brief Wait until graphic processing time is reached
         *  @note Graphic processing time is set by 'SRL_FRAMERATE' in makefile
         */
        inline static void Synchronize()
        {
            slSynch();
            SRL::Input::Gun::Synchronize();
        }
    };
};
