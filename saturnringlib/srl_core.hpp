#pragma once

#include "srl_memory.hpp"
#include "srl_tv.hpp"
#include "srl_color.hpp"
#include "srl_cd.hpp"
#include "srl_vdp1.hpp"
#include "srl_vdp2.hpp"
#include "srl_input.hpp"
#include "srl_event.hpp"

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
		
		/** @brief Haandle V-Blank events
		 */
		inline static void VblankHandling()
		{
			SRL::Input::IPeripheral::RefreshPeripherals();
			Core::OnVblank.Invoke();
		}

	public:

		/** @brief Initialize basic environment
		 * @param backColor Color of the screen
		 */
		inline static void Initialize(const Types::SaturnColor& backColor)
		{
			SRL::Memory::Initialize();

#if SRL_FRAMERATE > 0
			slInitSystem((Uint16)SRL::TV::Reslotution, SRL::VDP1::Textures->SglPtr(), SRL_FRAMERATE);
#else
			slInitSystem((Uint16)SRL::TV::Reslotution, SRL::VDP1::Textures->SglPtr(), 1);
			slDynamicFrame(1);
#endif
			// Initialize CD drive
			SRL:Cd::Initialize();

			// Initialize callbacks
			slIntFunction(Core::VblankHandling);

			// Start initializing stuff
			slTVOff();

			// Initialize VDP2
			VDP2::Initialize(backColor);

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
		}
	};
};