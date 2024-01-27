#pragma once

#include "../srl_debug.hpp"
#include "srl_bitmap.hpp"
#include "../srl_cd.hpp"

namespace SRL::Bitmap
{
	/** @brief TGA image handling
	 */
	struct TGA : IBitmap
	{
	private:

		enum class TgaType : Uint8
		{
			/** @brief No data present
			 */
			NoData = 0,

			/** @brief Raw paletted image
			 */
			UncompressedPaletted = 1,

			/** @brief Raw RGB image
			 */
			UncompressedRgb = 2,

			/** @brief Raw B/W image
			 */
			UncompressedBw = 3,

			/** @brief Runlength compressed paletted image
			 */
			RlePalleted = 9,

			/** @brief Runlength compressed RGB image
			 */
			RleRgb = 10,

			/** @brief Compressed b/W image
			 */
			CompressedBw = 11,

			/** @brief Compressed paletted image
			 */
			CompressedPaletted = 32,

			/** @brief Compressed palleted image using 4 pass quadtree-type process
			 */
			CompressedPaletted4Pass = 33
		};

		/** @brief Image bit depth
		 */
		enum class BitDepth : Uint8
		{
			/** @brief 8-bit color depth
			 */
			Depth8Bit = 8,

			/** @brief 15-bit color depth
			 */
			Depth15Bit = 15,

			/** @brief 16-bit color depth
			 */
			Depth16Bit = 16,

			/** @brief 24-bit color depth
			 */
			Depth24Bit = 24,

			/** @brief 32-bit color depth
			 */
			Depth32bit = 32
		};

		/** @brief TGA data point
		 */
		struct TgaPoint
		{
			/** @brief X coordinate
			 */
			Uint16 X;

			/** @brief Y coordinate
			 */
			Uint16 Y;
		};

		struct TgaHeader
		{
			/** @brief Image identifier
			 */
			Uint8 ImageId;

			/** @brief Color mode type
			 */
			Uint8 ColorMode;

			/** @brief Image type
			 */
			TgaType ImageType;

			/** @brief Start of the palette data
			 */
			Uint16 PaletteStart;

			/** @brief End of the palette data
			 */
			Uint16 PaletteEnd;

			/** @brief Size of a palette color entry
			 */
			Uint8 PaletteEntrySize;

			/** @brief Image origin point
			 */
			TgaPoint Origin;

			/** @brief Image size
			 */
			TgaPoint Size;

			/** @brief Color bit depth
			 */
			BitDepth ColorDepth;

			/** @brief Per color attribute size
			 */
			Uint8 AttributeSize;
		};


		/** @brief Height of the bitmap
		 */
		Uint16 height;

		/** @brief Image data
		 */
		char* imageData;

		/** @brief Color palette
		 */
		Bitmap::Palette* palette;

		/** @brief Width of the bitmap
		 */
		Uint16 width;

	public:

		/** @brief Construct RGB555 TGA image from file
		 * @param filename TGA file name
		 */
		TGA(char* filename) : imageData(nullptr), palette(nullptr)
		{
			Cd::File tgaFile = Cd::File(filename);
			
			// Open file
			if (tgaFile.Open())
			{


				// Close the file
				tgaFile.Close();
			}
		}

		/** @brief Destroy the TGA image
		 */
		~TGA()
		{
			if (this->imageData != nullptr)
			{
				delete this->imageData;
			}

			if (this->palette != nullptr)
			{
				delete this->palette->Colors;
				delete this->palette;
			}
		}

		/** @brief Get image data
		 * @return Pointer to image data
		 */
		void* GetData() override
		{
			return this->imageData;
		}

		/** @brief Get image info
		 * @return image info
		 */
		BitmapInfo GetInfo() override
		{
			if (this->palette != nullptr)
			{
				return BitmapInfo(this->width, this->height, this->palette);
			}

			return BitmapInfo(this->width, this->height);
		}
	};
}