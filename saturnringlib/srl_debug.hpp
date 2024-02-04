#pragma once

#include "srl_base.hpp"
#include "types/srl_color.hpp"
#include "srl_vdp2.hpp"
#include "srl_memory.hpp"
#include <std/string.h>

namespace SRL
{
	/** @brief Debug helper
	 */
	class Debug
	{
	private:
		/** @brief Current print text color
		 */
		inline static  Uint8 printColor = 0;

		/** @brief previous print text color
		 */
		inline static  Uint8 oldPrintColor = 0;

		/** @brief Default print text buffer
		 */
		inline static  char lineBuffer[SRL_DEBUG_MAX_PRINT_LENGTH];
	
	public:

#ifdef DEBUG
		/** @brief Debug functions are enabled
		 */
    	static constexpr bool Enabled = true;
#else
		/** @brief Debug functions are disabled
		 */
    	static constexpr bool Enabled = false;
#endif

		/** @brief Print text on screen at specific location
		 * @param x Offset from left of the screen
		 * @param y Offset from top of the screen
		 * @param text Text to print
		 */
		inline static void Print(Uint8 x, Uint8 y, const char* text)
		{
			slPrint((char*)text, slLocate(x, y));
		}

		/** @brief Print text on screen at specific location
		 * @param x Offset from left of the screen
		 * @param y Offset from top of the screen
		 * @param fromLeft Where to start next line from on screen
		 * @param fromRight Where to start wrapping to next line
		 * @param text Text to print
		 * @return Number of lines printed
		 */
		inline static Uint8 PrintWithWrap(Uint8 x, Uint8 y, const Uint8 fromLeft, const Uint8 fromRight, const char* text)
		{
			if (fromLeft < fromRight && x < fromRight && text != nullptr)
			{
				char wrapBuffer[2] = { ' ', '\0' };
				Uint16 character = 0;
				Uint8 line = 1;
				Uint16 screenX = x;

				while (text[character] != '\0')
				{
					if (screenX >= fromRight || text[character] == '\n')
					{
						if (text[character] == '\n')
						{
							character++;
						}

						screenX = fromLeft;
						line++;
					}

					wrapBuffer[0] = text[character];
					Debug::Print(screenX, line + y - 1, wrapBuffer);
					character++;
					screenX++;
				}
				
				return line;
			}

			return 0;
		}

		/** @brief Print text on screen at specific location
		 * @param x Offset from left of the screen
		 * @param y Offset from top of the screen
		 * @param text Text to print
		 * @param args... Text arguments
		 */
    	template <typename ...Args>
		inline static void Print(Uint8 x, Uint8 y, const char* text, Args...args)
		{
			Sint32 leftOver = snprintf(Debug::lineBuffer, SRL_DEBUG_MAX_PRINT_LENGTH, text, args ...);

			if (leftOver >= SRL_DEBUG_MAX_PRINT_LENGTH)
			{
				char expandedBuffer[SRL_DEBUG_MAX_PRINT_LENGTH + leftOver + 1];

				if (snprintf(expandedBuffer, SRL_DEBUG_MAX_PRINT_LENGTH + leftOver + 1, text, args ...) >= 0)
				{
					Debug::Print(x, y, expandedBuffer);
				}
			}
			else if (leftOver > 0)
			{
				Debug::Print(x, y, Debug::lineBuffer);
			}
		}

		/** @brief Print text on screen at specific location
		 * @param x Offset from left of the screen
		 * @param y Offset from top of the screen
		 * @param fromLeft Where to start next line from on screen
		 * @param fromRight Where to start wrapping to next line
		 * @param text Text to print
		 * @param args... Text arguments
		 * @return Number of lines printed
		 */
    	template <typename ...Args>
		inline static Uint8 PrintWithWrap(Uint8 x, Uint8 y, const Uint8 fromLeft, const Uint8 fromRight, const char* text, Args...args)
		{
			if (fromLeft < fromRight && x < fromRight && text != nullptr)
			{
				Sint32 leftOver = snprintf(Debug::lineBuffer, SRL_DEBUG_MAX_PRINT_LENGTH, text, args ...);

				if (leftOver >= SRL_DEBUG_MAX_PRINT_LENGTH)
				{
					char expandedBuffer[SRL_DEBUG_MAX_PRINT_LENGTH + leftOver + 1];

					if (snprintf(expandedBuffer, SRL_DEBUG_MAX_PRINT_LENGTH + leftOver + 1, text, args ...) >= 0)
					{
						return Debug::PrintWithWrap(x, y, fromLeft, fromRight, expandedBuffer);
					}
				}
				else if (leftOver > 0)
				{
					return Debug::PrintWithWrap(x, y, fromLeft, fromRight, Debug::lineBuffer);
				}
			}

			return 0;
		}

		/** @brief Color of the cursor
		 * @param color Cursor color index
		 */
		inline static void PrintColorSet(Uint8 color)
		{
			Debug::oldPrintColor = Debug::printColor;
			Debug::printColor = color;
			slCurColor(color);
		}

		/** @brief Restore previous print color
		 */
		inline static void PrintColorRestore()
		{
			Debug::PrintColorSet(Debug::oldPrintColor);
		}

		/** @brief Clear screen line
		 * @param line Line number
		 */
		inline static void PrintClearLine(const Uint8 line)
		{
			for (int x = 0; x < 40; x++)
			{
				Debug::Print(x, line, " ");
			}
		}

		/** @brief Clear whole screen from text
		 */
		inline static void PrintClearScreen()
		{
			for (int y = 0; y < 30; y++)
			{
				Debug::PrintClearLine(y);
			}
		}

		/** @brief Breaks any further execution and shows assert screen
		 * @param message Custom message to show
		 * @param file File the asser happened in
		 * @param function Function that threw the exception
		 * @param args... Text arguments
		 */
    	template <typename ...Args>
		inline static void Assert(const char* message, const char* file, const char* function, Args...args)
		{
			if constexpr (Debug::Enabled)
			{
				// Clear screen
				Debug::PrintClearScreen();

				// Set background to red and fonr to white
				SRL::VDP2::SetBackColor(SRL::Types::Colors::Red);
				Debug::PrintColorSet(0);

				Debug::Print(1,1, "Assert raised");
				Uint8 lines = Debug::PrintWithWrap(2, 2, 2, 39, "at %s\nin %s()", file, function);
				
				Debug::Print(1,lines + 4, "Message:");
				Debug::PrintWithWrap(2, lines + 5, 2, 39, message, args...);
				
				//Debug::Print(1,24, "Free texture memory: %d bytes", SRL::VDP1::GetAvailableMemory());
				Debug::Print(1,25, "Free memory: %d bytes", SRL::Memory::GetAvailableMemory());

				// Small animation so we know it did not crash
				Debug::Print(1,27, "[");
				Debug::Print(38,27, "]");
				Uint8 frame = 0;
				Uint16 frameCountdown = 3;
				bool breakOut = false;
				
				while (!breakOut)
				{
					if (frameCountdown == 0)
					{
						frameCountdown = 3;
						Sint16 clearFrame = (Sint16)frame - 5;
						Sint16 backArrowFrame = (Sint16)frame - 4;

						if (clearFrame < 0)
						{
							clearFrame = 36 + clearFrame;
						}

						if (backArrowFrame < 0)
						{
							backArrowFrame = 36 + backArrowFrame;
						}

						Debug::Print(clearFrame + 2, 27, " ");
						Debug::Print(backArrowFrame + 2, 27, "<");

						Debug::Print(frame + 2, 27, "=");
						frame++;

						if (frame > 35)
						{
							frame = 0;
							breakOut = true;
						}

						Debug::Print(frame + 2, 27, ">");
					}

					frameCountdown--;
					slSynch();
				}

				// Restore print color
				Debug::PrintColorRestore();
			}
		}

#ifdef DEBUG
		/** @brief Breaks any further execution and shows assert screen
		 * @param message Custom message to show
		 */
		#define Assert(message, ...) Assert((char*)message, __FILE__, __FUNCTION__ __VA_OPT__(,) __VA_ARGS__);
#else
		/** @brief Breaks any further execution and shows assert screen
		 * @param message Custom message to show
		 */
		#define Assert(message, ...) ()
#endif
	};
}