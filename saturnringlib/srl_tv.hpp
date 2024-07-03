#pragma once

#include "srl_base.hpp"

/** @brief Television control
 */
namespace SRL
{
	class TV
	{
	public:
			
		/** @brief Available TV resolutions
		 */
		enum class Resolutions
		{
			Normal320x224 = 0,
			Normal320x240 = 1,
			Normal320x256 = 2,
			Normal352x224 = 4,
			Normal352x240 = 5,
			Interlaced640x224 = 8,
			Interlaced640x240 = 9,
			Interlaced704x224 = 12,
			Interlaced704x240 = 13,
			Normal320x448i = 16,
			Normal320x480i = 17,
			Normal352x448 = 20,
			Normal352x480 = 21,
			Interlaced640x448i = 24,
			Interlaced640x480i = 25,
			Interlaced704x448 = 28,
			Interlaced704x480 = 29
		};

#ifdef SRL_MODE_PAL
		/** @brief Screen width
		 */
		inline static const uint16_t Width = 320;

		/** @brief Screen height
		 */
		inline static const uint16_t Height = 256;

		/** @brief Screen resolution mode
		 */
		inline static const TV::Resolutions Resolution = TV::Resolutions::Normal320x256;
#elif SRL_MODE_NTSC
	#ifdef SRL_HIGH_RES
		/** @brief Screen width
		 */
		inline static const uint16_t Width = 704;

		/** @brief Screen height
		 */
		inline static const uint16_t Height = 480;

		/** @brief Screen resolution mode
		 */
		inline static const TV::Resolutions Resolution = TV::Resolutions::Interlaced704x480;
	#else
		/** @brief Screen width
		 */
		inline static const uint16_t Width = 320;

		/** @brief Screen height
		 */
		inline static const uint16_t Height = 240;

		/** @brief Screen resolution mode
		 */
		inline static const TV::Resolutions Resolution = TV::Resolutions::Normal320x240;
	#endif
#elif DOXYGEN
		/** @brief Screen width
		 * @note Differs based on makefile setting SRL_MODE = (PAL | NTSC) and whether SRL_HIGH_RES is set
		 */
		inline static const uint16_t Width;

		/** @brief Screen height
		 * @note Differs based on makefile setting SRL_MODE = (PAL | NTSC) and whether SRL_HIGH_RES is set
		 */
		inline static const uint16_t Height;

		/** @brief Screen resolution mode
		 * @note Differs based on makefile setting SRL_MODE = (PAL | NTSC) and whether SRL_HIGH_RES is set
		 */
		inline static const TV::Resolutions Resolution;
#endif

	};
};
