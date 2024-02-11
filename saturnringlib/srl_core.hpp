#pragma once

#include "srl_memory.hpp"
#include "srl_tv.hpp"
#include "srl_color.hpp"
#include "srl_cd.hpp"
#include "srl_vdp1.hpp"
#include "srl_vdp2.hpp"

/** @brief Core functions of the library
 */
namespace SRL::Core
{
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

		// Start initializing stuff
		slTVOff();

		// Initialize VDP2
		VDP2::Initialize(backColor);

		// Set cull depth
    	slZdspLevel(3);

		// All was initialized
		slTVOn();
	}

};