#pragma once

#include "srl_tv.hpp"
#include "types/srl_color.hpp"

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
		static inline void SetBackColor(const Types::SaturnColor& color)
		{
			slBack1ColSet((void*)(VDP2_VRAM_A1 + 0x1fffe), color.ARGB);
		}

		/** @brief Set color for printf
		 * @param index Color index
		 * @param color Color to set
		 */
		static inline void SetPrintfPaletteColor(const Uint16 index, const Types::SaturnColor& color)
		{
			*(((Types::SaturnColor*)VDP2_COLRAM) + index) = color.ARGB;
		}

		/** @brief Initialize VDP2
		 * @param backColor starting back color
		 */
		static inline void Initialize(const Types::SaturnColor& backColor)
		{
			// Initialize color ram mode
			slColRAMMode(CRM16_2048);

			// Initialize printf
			slCharNbg0(COL_TYPE_256, CHAR_SIZE_1x1);
			VDP2::SetPrintfPaletteColor(0, Types::Colors::White);
			VDP2::SetPrintfPaletteColor(1, Types::Colors::Black);
			VDP2::SetPrintfPaletteColor(2, Types::Colors::Red);
			VDP2::SetPrintfPaletteColor(3, Types::Colors::Green);
			VDP2::SetPrintfPaletteColor(4, Types::Colors::Blue);
			VDP2::SetPrintfPaletteColor(5, Types::Colors::Yellow);
			VDP2::SetPrintfPaletteColor(6, Types::Colors::Magenta);
    		slCurColor(1);

			// Setup NBG1
			slBitMapNbg1(COL_TYPE_32768, (Uint16)TV::Reslotution, (void*)VDP2_VRAM_A1);

			// Set back color
			VDP2::SetBackColor(backColor);

			// Set default screens to show
			slScrTransparent(RBG0ON);
			slScrAutoDisp(NBG0ON | NBG1ON |SPRON);
		}
	};
}
