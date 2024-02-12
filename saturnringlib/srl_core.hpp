#pragma once

#include "srl_memory.hpp"
#include "srl_tv.hpp"
#include "srl_color.hpp"
#include "srl_cd.hpp"
#include "srl_vdp1.hpp"
#include "srl_vdp2.hpp"
#include "srl_input.hpp"
#include "std/vector.h"

namespace SRL
{
	/** @brief Core functions of the library
	 */
	class Core
	{
	private:
		
		/** @brief Contains all user functions to be called when V-Blank event fires
		 */
		inline static std::vector<void (*)()> VblankCallbacks = std::vector<void (*)()>();

		/** @brief Haandle V-Blank events
		 */
		static void VblankHandling()
		{
			SRL::Input::IPeripheral::RefreshPeripherals();

			for (auto event : Core::VblankCallbacks)
			{
				if (event != nullptr)
				{
					event();
				}
			}
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

			// All was initialized
			slTVOn();
		}

		/** @brief Register a function to be called when V-Blank hits
		 * @param function Function to register
		 */
		inline static void RegisterVblankCallback(void (*function)())
		{
			Core::VblankCallbacks.push_back(function);
		}

		/** @brief Un-register a function from being called when V-Blank hits
		 * @param function Function to register
		 */
		inline static void UnRegisterVblankCallback(void (*function)())
		{
			auto it = std::find(Core::VblankCallbacks.begin(), Core::VblankCallbacks.end(), function);

			if (it != Core::VblankCallbacks.end())
			{
				Core::VblankCallbacks.erase(it);
			}
		}
	};
};