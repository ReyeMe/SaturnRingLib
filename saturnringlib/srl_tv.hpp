#pragma once

#include "srl_base.hpp"

/** @brief Television control
 */
namespace SRL::TV
{
	/** @brief Available TV resolutions
	 */
	enum class Reslotutions
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
	static inline const Uint16 Width = 320;

	/** @brief Screen height
	 */
	static inline const Uint16 Height = 256;

	/** @brief Selected resolution
	 */
	static inline const TV::Reslotutions Reslotution = TV::Reslotutions::Normal320x256;
#elif SRL_MODE_NTSC
	#ifdef SRL_HIGH_RES
	/** @brief Screen width
	 */
	static inline const Uint16 Width = 704;

	/** @brief Screen height
	 */
	static inline const Uint16 Height = 480;

	/** @brief Selected resolution
	 */
	static inline const TV::Reslotutions Reslotution = TV::Reslotutions::Interlaced704x480;
	#else
	/** @brief Screen width
	 */
	static inline const Uint16 Width = 320;

	/** @brief Screen height
	 */
	static inline const Uint16 Height = 240;

	/** @brief Selected resolution
	 */
	static inline const TV::Reslotutions Reslotution = TV::Reslotutions::Normal320x240;
	#endif
#endif
};
