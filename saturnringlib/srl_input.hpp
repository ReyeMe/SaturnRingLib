#pragma once

#include "srl_vector.hpp"

/** @brief Input handling
 */
namespace SRL::Input
{
    /** @brief Peripheral type family
     */
    enum class PeripheralFamily : uint8_t
    {
        /** @brief Digital controllers (control pad, fighting stick, etc...)
         */
        Digital = 0x00,

        /** @brief Analog controllers (3d pad, racing wheel, train controller, etc...)
         */
        Analog = 0x10,

        /** @brief Pointer controllers (Saturn mouse, Gun)
         */
        Pointing = 0x20,

        /** @brief Keyboard controllers (NetLink keyboard)
         */
        KeyBoard = 0x30,

        /** @brief Extended/Megadrive controllers (shuttle mouse, MD gamepad)
         */
        Extended = 0xe0,

        /** @brief Controller not connected, Unknown type
         */
        Unknown = 0xf0,
    };
    
    /** @brief Peripheral type
     */
    enum class PeripheralType : uint8_t
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

        /** @brief Shuttle mouse
         */
        ShuttleMouse = 0xe3,

        /** @brief Controller not connected
         */
        NotConnected = 0xff
    };

    /** @brief Peripheral management
     */
    class Management
    {
        /** @brief Allow SRL::Input::PeripheralGeneric struct to access private members of this class
         */
        friend struct PeripheralGeneric;

    public:

        /** @brief Maximal number of connected devices
         */
        inline static const uint8_t MaxPeripherals = 12;

    private:

        /** @brief Connected devices from previous update
         */
        inline static PerDigital PeripheralsPreviousState[Management::MaxPeripherals] = { 0xff, 0, 0, 0, 0, 0 };

        /** @brief Connected devices in current frame
         */
        inline static PerDigital Peripherals[Management::MaxPeripherals] = { 0xff, 0, 0, 0, 0, 0 };

        /** @brief Disabled constructor
         */
        Management() = delete;

        /** @brief Disable destructor
         */
        ~Management() = delete;

    public:
    
        /** @brief Find port number of nth connected peripheral
         * @param index Index of the peripheral
         * @return Port number of connected peripheral or 0xff if none
         */
        inline static uint8_t FindNthConnectedPeripheral(const uint8_t& index)
        {
            uint8_t counter = 0;

            for (uint8_t port = 0; port < Management::MaxPeripherals; port++)
            {
                if (Management::IsConnected(port))
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

        /** @brief Get the Raw port data
         * @param port Port index
         * @return Port data
         */
        inline static PerDigital* GetRawData(const uint8_t& port)
        {
            return &Management::Peripherals[port];
        }

        /** @brief Gets connected peripheral type to specified port
         * @param port Peripheral port
         * @return Peripheral type
         */
        inline static PeripheralType GetType(const uint8_t& port)
        {
            if (Management::IsConnected(port))
            {
                return (PeripheralType)Management::Peripherals[port].id;
            }

            return PeripheralType::NotConnected;
        }

        /** @brief Gets connected peripheral type family to specified port
         * @param port Peripheral port
         * @return Peripheral type family
         */
        inline static PeripheralFamily GetFamily(const uint8_t& port)
        {
            return (PeripheralFamily)((uint8_t)Management::GetType(port) & 0xf0);
        }

        /** @brief Checks whether any peripheral is connected to specified port
         * @param port Peripheral port
         * @return true if peripheral is present
         */
        inline static bool IsConnected(const uint8_t& port)
        {
            return port < Management::MaxPeripherals && Management::Peripherals[port].id != (uint8_t)PeripheralType::NotConnected;
        }

        /** @brief Refresh peripherals
         * @note For internal use only
         */
        static void RefreshPeripherals()
        {
            // Copy current state to previous state
            slDMACopy(Management::Peripherals, Management::PeripheralsPreviousState, sizeof(Management::PeripheralsPreviousState));

            // Copy new state in
            uint8_t* destination = reinterpret_cast<uint8_t*>(Management::Peripherals);
            uint8_t* source = reinterpret_cast<uint8_t*>(Smpc_Peripheral);
            uint32_t batchSize = sizeof(PerDigital) * (Management::MaxPeripherals >> 1);

            // Copy first half
            for (uint32_t byte = 0; byte < batchSize; byte++)
            {
                *destination++ = *source++;
            }
            
            // Offset for second multi-tap (see https://github.com/johannes-fetz/joengine/issues/23)
            source += sizeof(PerDigital) * 9;

            // Copy second half
            for (uint32_t byte = 0; byte < batchSize; byte++)
            {
                *destination++ = *source++;
            }
        }

    };
    
    /** @brief Generic peripheral base
     */
    struct PeripheralGeneric
    {
    public:
    
        /** @brief Port number peripheral is connected to
         * @note valid number is 0 to 11, see SRL::Input::MaxPeripherals
         */
        uint32_t Port = 0;

    protected:

        /** @brief Generic peripheral
         * @param port Peripheral port
         */
        PeripheralGeneric(uint32_t port) : Port(port) { }

        /** @brief Get state of the peripheral in the previous frame
         * @return PerDigital* Peripheral data
         */
        PerDigital* GetPreviousFrameState()
        {
            return &Management::PeripheralsPreviousState[this->Port];
        }

        /** @brief Get state of the peripheral in the current frame
         * @return PerDigital* Peripheral data
         */
        PerDigital* GetCurrentFrameState()
        {
            return &Management::Peripherals[this->Port];
        }

    public:

        /** @brief Indicates whether peripheral is connected or not
         * @return true if connected
         */
        virtual bool IsConnected()
        {
            return Management::IsConnected(this->Port);
        }

        /** @brief Gets connected peripheral type
         *  @return Peripheral type
         */
        PeripheralType GetType()
        {
            return Management::GetType(this->Port);
        }

        /** @brief Gets connected peripheral type family
         *  @return Peripheral type family
         */
        PeripheralFamily GetFamily()
        {
            return Management::GetFamily(this->Port);
        }
    };

    /** @brief Generic pointing (mouse) peripheral
     */
    struct Pointer : public PeripheralGeneric
    {
        /** @brief Pointer device buttons
         */
        enum Button
        {
            /** @brief Left pointer button
             */
            Left = 1 << 10,

            /** @brief Right pointer button
             */
            Right = 1 << 9,

            /** @brief Middle pointer button
             */
            Middle = 1 << 8,

            /** @brief Start button
             */
            Start = 1 << 11,
        };

        /** @brief Construct a new pointing peripheral handler
         * @param port Pointing peripheral port
         */
        Pointer(const uint8_t& port) : PeripheralGeneric(port) { }

        /** @brief Indicates whether peripheral is connected or not
         * @return true if connected
         */
        bool IsConnected() override
        {
            Input::PeripheralType type = this->GetType();
            return type == Input::PeripheralType::Mouse ||
                type == Input::PeripheralType::ShuttleMouse;
        }

        /** @brief Check if user is holding down a button
         * @param peripheral Connected peripheral
         * @param button Button to check
         * @return true Button was pressed
         * @return false Button was not pressed
         */
        bool virtual IsHeld(const Button& button)
        {
            return (this->GetCurrentFrameState()->data & ((uint16_t)button)) == 0;
        }

        /** @brief Check if user released a button
         * @param peripheral Connected peripheral
         * @param button Button to check
         * @return true Button was released
         * @return false Button was not released
         */
        bool virtual WasReleased(const Button& button)
        {
            uint16_t current = this->GetCurrentFrameState()->data;
            uint16_t last = this->GetPreviousFrameState()->data;
            return (((current ^ last) & current) & ((uint16_t)button)) != 0;
        }

        /** @brief Check if user pressed a button
         * @param peripheral Connected peripheral
         * @param button Button to check
         * @return true Button was pressed
         * @return false Button was not pressed
         */
        bool virtual WasPressed(const Button& button)
        {
            uint16_t current = this->GetCurrentFrameState()->data;
            uint16_t last = this->GetPreviousFrameState()->data;
            return (((current ^ last) & last) & ((uint16_t)button)) != 0;
        }

        /** @brief Get the pointer position
         *  @return Vector2D Pointer position
         */
        SRL::Types::Vector2D virtual GetPosition()
        {
            PerPoint* data = (PerPoint*)this->GetCurrentFrameState();
            return SRL::Types::Vector2D(SRL::Types::Fxp::FromInt(data->x), SRL::Types::Fxp::FromInt(data->y));
        }
    };

    /** @brief Generic digital peripheral
     */
    struct Digital : public PeripheralGeneric
        {
            /** @brief Digital gamepad buttons
             */
            enum class Button : uint16_t
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
            Digital(const uint8_t& port) : PeripheralGeneric(port) { }

            /** @brief Indicates whether peripheral is connected or not
             * @return true if connected
             */
            bool IsConnected() override
            {
                return this->GetFamily() == Input::PeripheralFamily::Digital;
            }

            /** @brief Check if user is holding down a button
             * @param peripheral Connected peripheral
             * @param button Button to check
             * @return true Button was pressed
             * @return false Button was not pressed
             */
            bool virtual IsHeld(const Button& button)
            {
                return (this->GetCurrentFrameState()->data & ((uint16_t)button)) == 0;
            }

            /** @brief Check if user released a button
             * @param peripheral Connected peripheral
             * @param button Button to check
             * @return true Button was released
             * @return false Button was not released
             */
            bool virtual WasReleased(const Button& button)
            {
                uint16_t current = this->GetCurrentFrameState()->data;
                uint16_t last = this->GetPreviousFrameState()->data;
                return (((current ^ last) & current) & ((uint16_t)button)) != 0;
            }

            /** @brief Check if user pressed a button
             * @param peripheral Connected peripheral
             * @param button Button to check
             * @return true Button was pressed
             * @return false Button was not pressed
             */
            bool virtual WasPressed(const Button& button)
            {
                uint16_t current = this->GetCurrentFrameState()->data;
                uint16_t last = this->GetPreviousFrameState()->data;
                return (((current ^ last) & last) & ((uint16_t)button)) != 0;
            }
        };

    /** @brief Generic analog peripheral
     */
    struct Analog : public Digital
    {
        /** @brief Analog axis
         */
        enum Axis
        {
            /** @brief First axis
             */
            Axis1 = 0,

            /** @brief Second axis
             */
            Axis2,

            /** @brief Third axis
             */
            Axis3,

            /** @brief Fourth axis
             */
            Axis4,

            /** @brief Fifth axis
             */
            Axis5,

            /** @brief Sixth axis
             */
            Axis6,
        };

        /** @brief Construct a new analog peripheral handler
         * @param port Analog peripheral port
         */
        Analog(const uint8_t& port) : Digital(port) { }

        /** @brief Indicates whether peripheral is connected or not
         * @return true if connected
         */
        bool IsConnected() override
        {
            return this->GetFamily() == Input::PeripheralFamily::Analog;
        }

        /** @brief Get current axis value
         * @param axis Analog axis
         * @return uint8_t value in range of 0 to 256
         */
        uint8_t virtual GetAxis(const Axis axis)
        {
            PerAnalog* data = this->Port < Management::MaxPeripherals ? (PerAnalog*)this->GetCurrentFrameState() : nullptr;

            switch (this->GetType())
            {
            
            // Racing wheel data handling
            case Input::PeripheralType::Racing:
                if (axis == Axis::Axis1) return data->x;
                break;

            // Mission-stick data handling
            // TODO: Handle extended data (second stick connected) 
            case Input::PeripheralType::AnalogPad:
                switch (axis)
                {
                case Axis::Axis1:
                    return data->x;      // Stick X axis
                case Axis::Axis2:
                    return data->y;      // Stick Y axis
                case Axis::Axis3:
                    return data->z;      // Throttle
                default:
                    break;
                }
                break;

            // Nights pad data handling
            case Input::PeripheralType::Analog3dPad:
                switch (axis)
                {
                case Axis::Axis1:
                    return data->x;      // Thumb X axis
                case Axis::Axis2:
                    return data->y;      // Thumb Y axis
                case Axis::Axis3:
                    return data->z;      // Right trigger
                case Axis::Axis4:
                    return data->dummy1; // Left trigger
                default:
                    break;
                }
                break;

            // Unknown type
            default:
                break;
            }

            return 0;
        }
    };
}