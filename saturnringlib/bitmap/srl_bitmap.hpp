#pragma once

#include "../srl_memory.hpp"
#include "../types/srl_color.hpp"

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
		 * @param count Number of colors in the palette
		 */
		Palette(size_t count) : Colors(nullptr), Count(count)
		{
			this->Colors = new SRL::Types::SaturnColor[count];
		}

		/** @brief Construct a new Palette object
		 * @param colors Palette colors
		 * @param count Number of colors in the palette
		 */
		Palette(SRL::Types::SaturnColor* colors, size_t count) : Colors(colors), Count(count)
		{
			// Do nothing
		}

		/** @brief Destroy the Palette object
		 */
		~Palette()
		{
			delete this->Colors;
		}
	};

	/** @brief Bitmap information
	 */
	struct BitmapInfo
	{
		/** @brief Texture color mode
		 */
		enum class TextureColorMode : Uint16
		{
			/** @brief 16 color image
			 */
			Paletted16 = 3,

			/** @brief 64 color image
			 */
			Paletted = 2,

			/** @brief 32k color image
			 */
			RGB555 = 1
		};

		/** @brief Image width
		 */
		Uint16 Width;

		/** @brief Image height
		 */
		Uint16 Height;

		/** @brief Image color mode
		 */
		BitmapInfo::TextureColorMode ColorMode;

		/** @brief Used palette (not in use for RGB555 images)
		 */
		Bitmap::Palette* Palette;

		/** @brief Construct a new Bitmap info
		 * @param width Image width
		 * @param height Image height
		 */
		BitmapInfo(Uint16 width, Uint16 height) : Width(width), Height(height), ColorMode(BitmapInfo::TextureColorMode::RGB555), Palette(nullptr)
		{
			// Do nothing
		}

		/** @brief Construct a new Bitmap info
		 * @param width Image width
		 * @param height Image height
		 * @param palette Color palette
		 */
		BitmapInfo(Uint16 width, Uint16 height, Bitmap::Palette* palette) : Width(width), Height(height), ColorMode(BitmapInfo::TextureColorMode::Paletted), Palette(palette)
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
		virtual Uint8* GetData()
		{
			return nullptr;
		}
		
		/** @brief Get bitmap info
		 * @return Bitmap info
		 */
		virtual BitmapInfo GetInfo()
		{
			return BitmapInfo(0, 0);
		}
	};
}
