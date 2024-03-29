#pragma once

#include "srl_base.hpp"

/** @brief Input handling
 */
namespace SRL::Input
{
	/** @brief Peripheral type family
	 */
	enum class PeripheralFamily : Uint8
	{
		/** @brief Digital controllers (control pad, fighting stick, etc...)
		 */
		Digital = 0x00,

		/** @brief Analog controllers (3d pad, racing wheel, train controller, etc...)
		 */
		Analog = 0x10,

		/** @brief Mouse controllers (Saturn mouse, Gun)
		 */
		Pointing = 0x20,

		/** @brief Keyboard controllers (NetLink keyboard)
		 */
		KeyBoard = 0x30,

		/** @brief Megadrive controllers (MD mouse, MD gamepads)
		 */
		MegaDrive = 0xe0,

		/** @brief Controller not connected, Unknown type
		 */
		Unknown = 0xf0,
	};
	
	/** @brief Peripheral type
	 */
	enum class PeripheralType : Uint8
	{
		/** @brief Generic gamepad
		 */
		Gamepad = 0x02,

		/** @brief Racing wheel
		 */
		Racing = 0x13,

		/** @brief Mission stick
		 */
		AnalogPad = 0x15,

		/** @brief 3D analog pad
		 */
		Analog3dPad = 0x16,

		/** @brief NetLink/XBand mouse
		 */
		Mouse = 0x23,

		/** @brief Light gun
		 */
		Gun = 0x25,

		/** @brief NetLink/XBand keyboard
		 */
		Keyboard = 0x34,

		/** @brief 3 button Megadrive controller
		 */
		MD3ButtonPad = 0xe1,

		/** @brief 6 button Megadrive controller
		 */
		MD6ButtonPad = 0xe2,

		/** @brief Megadrive mouse
		 */
		ShuttleMouse = 0xe3,

		/** @brief Controller not connected
		 */
		NotConnected = 0xff
	};

	/** @brief Connected device control
	 */
	struct IPeripheral
	{
	public:

		/** @brief Maximal number of connected devices
		 */
		inline static const Uint8 MaxPeripherals = 12;

	protected:

		/** @brief Connected devices from previous update
		 */
		inline static PerDigital PeripheralsPreviousState[IPeripheral::MaxPeripherals] = { 0xff, 0, 0, 0, 0, 0 };

		/** @brief Connected devices in current frame
		 */
		inline static PerDigital Peripherals[IPeripheral::MaxPeripherals] = { 0xff, 0, 0, 0, 0, 0 };

	public:
        /** @brief Get the Raw port data
         * @param port Port index
         * @return Port data
         */
        static PerDigital* GetRawData(const Uint8& port)
        {
            return &IPeripheral::Peripherals[port];
        }

		/** @brief Checks whether any peripheral is connected to specified port
		 * @param port Peripheral port
		 * @return true if peripheral is present
		 */
		inline static bool IsConnected(const Uint8& port)
		{
			return port < IPeripheral::MaxPeripherals && IPeripheral::Peripherals[port].id != (Uint8)PeripheralType::NotConnected;
		}

		/** @brief Gets connected peripheral type to specified port
		 * @param port Peripheral port
		 * @return Peripheral type
		 */
		inline static PeripheralType GetType(const Uint8& port)
		{
			if (IPeripheral::IsConnected(port))
			{
				return (PeripheralType)IPeripheral::Peripherals[port].id;
			}

			return PeripheralType::NotConnected;
		}

		/** @brief Find poirt number of nth connected peripheral
		 * @param index Index of the peripheral
		 * @return Port number of connected peripheral of 0xff
		 */
		inline static Uint8 FindNthConnectedPeripheral(const Uint8& index)
		{
			Uint8 counter = 0;

			for (Uint8 port = 0; port < IPeripheral::MaxPeripherals; port++)
			{
				if (IPeripheral::IsConnected(port))
				{
					if (counter == index)
					{
						return port;
					}

					counter++;
				}
			}

			return 0xff;
		}

		/** @brief Refresh peripherals
		 */
		static void RefreshPeripherals()
		{
			Uint8* destination = reinterpret_cast<Uint8*>(IPeripheral::Peripherals);
			Uint8* source = reinterpret_cast<Uint8*>(Smpc_Peripheral);
			Uint32 batchSize = sizeof(PerDigital) * (IPeripheral::MaxPeripherals >> 1);

			// Copy first half
			for (Uint32 byte = 0; byte < batchSize; byte++)
			{
				*destination++ = *source++;
			}
			
			// Offset for second multitap (see https://github.com/johannes-fetz/joengine/issues/23)
			source += sizeof(PerDigital) * 9;

			// Copy second half
			for (Uint32 byte = 0; byte < batchSize; byte++)
			{
				*destination++ = *source++;
			}
		}
	
	private:

		/** @brief Construct an empty peripheral
		 */
		IPeripheral() : Port(0) { }

	protected:
	
		/** @brief Construct a new peripheral handle
		 * @param port Peripheral port
		 */
		IPeripheral(const Uint8& port) : Port(port) { }

	public:

		/** @brief Port of the current peripheral
		 */
		Uint8 Port;

		/** @brief Gets connected peripheral type family to specified port
		 * @param port Peripheral port
		 * @return Peripheral type family
		 */
		PeripheralFamily GetFamily(const Uint8& port)
		{
			if (IPeripheral::IsConnected(port))
			{
				return (PeripheralFamily)(IPeripheral::Peripherals[port].id & 0xf0);
			}

			return PeripheralFamily::Unknown;
		}

		/** @brief Indicates whether peripheral is connected or not
		 * @return true if connected
		 */
		constexpr virtual bool IsConnected()
		{
			return IPeripheral::IsConnected(this->Port);
		}

		/** @brief Gets connected peripheral type
		 *  @return Peripheral type
		 */
		PeripheralType GetType()
		{
			return IPeripheral::GetType(this->Port);
		}

		/** @brief Gets connected peripheral type family
		 *  @return Peripheral type family
		 */
		PeripheralFamily GetFamily()
		{
			return IPeripheral::GetFamily(this->Port);
		}
	};
	
	/** @brief Generic gamepad
	 */
	struct Gamepad : public IPeripheral
	{
		/** @brief Digital gamepad buttons
		 */
		enum class Button : Uint16
		{
			/** @brief D-Pad right direction
			 */
			Right = 1 << 15,

			/** @brief D-Pad left direction
			 */
			Left = 1 << 14,

			/** @brief D-Pad down direction
			 */
			Down = 1 << 13,

			/** @brief D-Pad up direction
			 */
			Up = 1 << 12,

			/** @brief Start button
			 */
			START = 1 << 11,

			/** @brief A button
			 */
			A = 1 << 10,

			/** @brief B button
			 */
			B = 1 << 8,

			/** @brief C button
			 */
			C = 1 << 9,

			/** @brief X button
			 */
			X = 1 << 6,

			/** @brief Y button
			 */
			Y = 1 << 5,

			/** @brief Z button
			 */
			Z = 1 << 4,

			/** @brief Right trigger
			 */
			R = 1 << 7,

			/** @brief Left trigger
			 */
			L = 1 << 3
		};

		/** @brief Construct a new Gamepad handler
		 * @param port Gamepad peripheral port
		 */
		Gamepad(const Uint8& port) : IPeripheral(port) { }

		/** @brief Indicates whether peripheral is connected or not
		 * @return true if connected
		 */
		constexpr bool IsConnected() override
		{
			return IPeripheral::IsConnected(this->Port) && IPeripheral::Peripherals[this->Port].id == (Uint8)PeripheralType::Gamepad;
		}

		/** @brief Check if user is holding down a button
		 * @param peripheral Connected peripheral
		 * @param button Button to check
		 * @return true Button was pressed
		 * @return false Button was not pressed
		 */
		constexpr bool virtual IsHeld(const Button& button)
		{
			return this->IsConnected() && ((IPeripheral::Peripherals[this->Port].data & ((Uint16)button)) == 0);
		}

		/** @brief Check if user pressed a button
		 * @param peripheral Connected peripheral
		 * @param button Button to check
		 * @return true Button is being held down
		 * @return false Button is not held down
		 */
		constexpr bool virtual IsDown(const Button& button)
		{
			if (this->IsConnected())
			{
				Uint16 current = IPeripheral::Peripherals[this->Port].data;
				Uint16 last = IPeripheral::PeripheralsPreviousState[this->Port].data;
				return (((current ^ last) & current) & ((Uint16)button)) != 0;
			}

			return false;
		}

		/** @brief Check if user released a button
		 * @param peripheral Connected peripheral
		 * @param button Button to check
		 * @return true Button was released
		 * @return false Button was not released
		 */
		constexpr bool virtual IsUp(const Button& button)
		{
			if (this->IsConnected())
			{
				Uint16 current = IPeripheral::Peripherals[this->Port].data;
				Uint16 last = IPeripheral::PeripheralsPreviousState[this->Port].data;
				return (((current ^ last) & last) & ((Uint16)button)) != 0;
			}

			return false;
		}
	};
	
}