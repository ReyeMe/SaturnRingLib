#pragma once

#include "srl_tv.hpp"
#include "srl_cram.hpp"

namespace SRL
{
	/** @brief VDP2 control
	 */
	class VDP2
	{
	private:

	public:
		/** @brief Set the back color
		 * @param color Color to use
		 */
		inline static  void SetBackColor(const Types::HighColor& color)
		{
			slBack1ColSet((void*)(VDP2_VRAM_A1 + 0x1fffe), (Uint16)color);
		}

		/** @brief Set color for print
		 * @param index Color index
		 * @param color Color to set
		 */
		inline static  void SetPrintPaletteColor(const Uint16 index, const Types::HighColor& color)
		{
			*(((Types::HighColor*)CRAM::BaseAddress) + (1 + (index << 8))) = (Uint16)color;
		}

		/** @brief Initialize VDP2
		 * @param backColor starting back color
		 */
		inline static void Initialize(const Types::HighColor& backColor)
		{
			// Initialize color ram mode
			slColRAMMode(CRM16_2048);

			// Initialize printf
			slCharNbg0(COL_TYPE_256, CHAR_SIZE_1x1);
			VDP2::SetPrintPaletteColor(0, Types::HighColor::Colors::White);
			VDP2::SetPrintPaletteColor(1, Types::HighColor::Colors::Black);
			VDP2::SetPrintPaletteColor(2, Types::HighColor::Colors::Red);
			VDP2::SetPrintPaletteColor(3, Types::HighColor::Colors::Green);
			VDP2::SetPrintPaletteColor(4, Types::HighColor::Colors::Blue);
			VDP2::SetPrintPaletteColor(5, Types::HighColor::Colors::Yellow);
			VDP2::SetPrintPaletteColor(6, Types::HighColor::Colors::Magenta);

			// Set default color to white
    		slCurColor(0);
			
			// Set back color
			VDP2::SetBackColor(backColor);

			// Set default screens to show
			slScrAutoDisp(NBG0ON | SPRON);
		}
	};
}
