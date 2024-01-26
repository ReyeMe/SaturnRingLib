#pragma once

#include "..\srl_core.hpp"

/** @brief Bitmap loading
 */
namespace SRL::Bitmap
{
	/** @brief Color palette
	 */
	struct Palette
	{
		/** @brief Number of colors in the palette
		 */
		size_t Count;

		/** @brief Palette colors
		 */
		SRL::Types::SaturnColor* Colors;

		/** @brief Construct a new Palette object
		 * @param colors Palette colors
		 * @param count Number of colors in the palette
		 */
		Palette(SRL::Types::SaturnColor* colors, size_t count) : Colors(colors), Count(count)
		{
			// Do nothing
		}
	};

	/** @brief Bitmap information
	 */
	struct BitmapInfo
	{
		/** @brief Image width
		 */
		Uint16 Width;

		/** @brief Image height
		 */
		Uint16 Height;

		/** @brief Image color mode
		 */
		SRL::VDP1::TextureColorMode ColorMode;

		/** @brief Used palette (not in use for RGB555 images)
		 */
		Bitmap::Palette* Palette;

		/** @brief Construct a new Bitmap info
		 * @param width Image width
		 * @param height Image height
		 */
		BitmapInfo(Uint16 width, Uint16 height) : Width(width), Height(height), ColorMode(SRL::VDP1::TextureColorMode::RGB555), Palette(nullptr)
		{
			// Do nothing
		}

		/** @brief Construct a new Bitmap info
		 * @param width Image width
		 * @param height Image height
		 * @param palette Color palette
		 */
		BitmapInfo(Uint16 width, Uint16 height, Bitmap::Palette* palette) : Width(width), Height(height), ColorMode(SRL::VDP1::TextureColorMode::Paletted), Palette(palette)
		{
			// Do nothing
		}
	};

	/** @brief 
	 */
	struct IBitmap
	{
		/** @brief Get image data
		 * @return Pointer to image data
		 */
		virtual void* GetData() {}
		
		/** @brief Get bitmap info
		 * @return Bitmap info
		 */
		virtual BitmapInfo GetInfo() {}
	};
}
