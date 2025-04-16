#pragma once

#include "srl_debug.hpp"

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

        /** @brief Megadrive light gun
         */
        MDGun = (MEGA_ID_StnShooting | 0xf0),

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

        /** @brief Allow SRL::Input::Gun struct to access private members of this class
         */
        friend struct Gun;

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
            slDMAWait();

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
        uint8_t Port = 0;

    protected:

        /** @brief Generic peripheral
         * @param port Peripheral port
         */
        PeripheralGeneric(uint8_t port) : Port(port) { }

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
         * @param button Button to check
         * @return true Button was pressed
         * @return false Button was not pressed
         */
        bool virtual IsHeld(const Button& button)
        {
            return (this->GetCurrentFrameState()->data & ((uint16_t)button)) == 0;
        }

        /** @brief Check if user released a button
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
        SRL::Math::Types::Vector2D virtual GetPosition()
        {
            PerPoint* data = (PerPoint*)this->GetCurrentFrameState();
            return SRL::Math::Types::Vector2D((int16_t)data->x, (int16_t)data->y);
        }
    };

    /** @brief Light gun peripheral
     * @note Light gun only works if SRL_FRAMERATE is set to 2 or higher (>30fps)
     * @note Light gun can **NOT** be connected through multi-tap! It must be connected directly to the console. Only valid ports are port 0 and port 6.
     * @note Light gun should be calibrated in game, to ensure correct positional data. Calibration should be done by user and result preferably stored to backup memory.
     */
    struct Gun : public PeripheralGeneric
    {
    private:

        /** @brief VDP2 external signal enable latch
         */
        static inline const uint16_t ExternalSignalEnable = 0x0200;

        /** @brief Light gun mega ID
         */
        static inline const uint8_t GunMegaId = (MEGA_ID_StnShooting | 0xf0);

        /** @brief Player 1 gun port
         */
        static inline const uint8_t GunPlayer1Port = 0;

        /** @brief Player 2 gun port
         */
        static inline const uint8_t GunPlayer2Port = 6;

        /** @brief Indicates whether peripheral control for player 1 light gun is enabled
         */
        static inline bool Player1MegadriveGunControl = false;

        /** @brief Indicates whether peripheral control for player 2 light gun is enabled
         */
        static inline bool Player2MegadriveGunControl = false;

        /** @brief Get the Mega Drive light gun ID
         * @param pd1 In direction port data
         * @param pd0 Out direction port data
         * @return uint8_t Mega drive light gun ID
         */
        static inline uint8_t GetMegaDriveId(uint8_t pd1, uint8_t pd0)
        {
            uint8_t mid;
            pd1  = (pd1 & 0x0a) | ((pd1 & 0x05) << 1);
            mid  = (pd1 & 0x08) | ((pd1 & 0x02) << 1);
            pd0  = (pd0 & 0x05) | ((pd0 & 0x0a) >> 1);
            mid |= (pd0 & 0x01) | ((pd0 & 0x04) >> 1);
            return mid;
        }

        /** @brief Check if light gun device is connected
         * @param port Peripheral port
         * @return true if connected
         */
        static inline bool IsDeviceConnected(uint8_t port)
        {
            Input::PeripheralType type = Management::GetType(port);
            return type == Input::PeripheralType::Gun;
        }

        /** @brief Construct a new light gun peripheral handler
         * @note Light gun can **NOT** be connected through multi-tap! It must be connected directly to the console. Only valid ports for this constructor are port 0 and port 6.
         * @param port Light gun peripheral port
         */
        Gun(const uint8_t& port) : PeripheralGeneric(port)
        {
            
#ifdef DEBUG
            if (port != GunPlayer1Port && port != GunPlayer2Port)
            {
                SRL::Debug::Assert("Light gun can be present only on port %d or %d and must be connected directly to the console!", GunPlayer1Port, GunPlayer2Port);
            }
#endif
        }

    public:

        /** @brief Pointer device buttons
         */
        enum Button
        {
            /** @brief Left pointer button
             */
            Trigger = 1 << 10,

            /** @brief Start button
             */
            Start = 1 << 11,
        };

        /** @brief Player port
         */
        enum Player
        {
            /** @brief First player on port 0
             */
            Player1 = 0,

            /** @brief Second player on port 6
             */
            Player2 = 6
        };

        /** @brief Construct a new light gun peripheral handler
         * @note Light gun can **NOT** be connected through multi-tap! It must be connected directly to the console.
         * @param player Light gun player
         */
        Gun(const Player& player) : PeripheralGeneric((uint8_t)player) { }

        /** @brief Indicates whether peripheral is connected or not
         * @return true if connected
         */
        bool IsConnected() override
        {
            return Gun::IsDeviceConnected(this->Port);
        }

        /** @brief Check if user is holding down a button
         * @param button Button to check
         * @return true Button was pressed
         * @return false Button was not pressed
         */
        bool virtual IsHeld(const Button& button)
        {
            return (this->GetCurrentFrameState()->data & ((uint16_t)button)) == 0;
        }

        /** @brief Check if user released a button
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

        /** @brief Get the hit position
         *  @return Vector2D hit position
         */
        SRL::Math::Types::Vector2D virtual GetPosition()
        {
            PerPoint* data = (PerPoint*)this->GetCurrentFrameState();
            return SRL::Math::Types::Vector2D((int16_t)data->x, (int16_t)data->y);
        }

        /**
         * @name Internal function
         * @{
         */

        /** @brief Trigger data synchronization
         * @details This should be called at the beginning of or at the end of each logic frame
         * @note Used internally
         */
        static inline void Synchronize()
        {
            static uint8_t firstPlayerId = PER_ID_StnShooting;
            static uint8_t secondPlayerId = PER_ID_StnShooting;
            bool refreshStatus = false;

            // Handle gun re-initialization
            // Re-initialize external signal register
            if (VDP2_EXTEN != ExternalSignalEnable &&
                ((Management::Peripherals[GunPlayer1Port].id == GunMegaId ||
                  Gun::IsDeviceConnected(GunPlayer1Port)) ||
                ((Management::Peripherals[GunPlayer2Port].id == GunMegaId ||
                  Gun::IsDeviceConnected(GunPlayer2Port)))))
            {
                VDP2_EXTEN = ExternalSignalEnable;
            }

            // Re-initialize SMPC
            if ((firstPlayerId != PER_ID_StnShooting && firstPlayerId != GunMegaId) &&
                Management::Peripherals[GunPlayer1Port].id == GunMegaId)
            {
                firstPlayerId = PER_ID_StnShooting;
                slSetPortSelect1(SMPC_SH2_DIRECT);
                slSetPortExt1(SMPC_EXL_ENA);
                slGetStatus();
                return;
            }

            if ((secondPlayerId != PER_ID_StnShooting && secondPlayerId != GunMegaId) &&
                Management::Peripherals[GunPlayer2Port].id == GunMegaId)
            {
                secondPlayerId = PER_ID_StnShooting;
                slSetPortSelect2(SMPC_SH2_DIRECT);
                slSetPortExt2(SMPC_EXL_ENA);
                slGetStatus();
                return;
            }

            // Handle light gun
            if (!slCheckIntBackSet())
            {
                if (Gun::Player1MegadriveGunControl)
                {
                    slSetPortDir1(0x00);
                    slSetPortData1(0x7f);
                    slSetPortSelect1(SMPC_SH2_DIRECT);
                }
                else
                {
                    slSetPortSelect1(SMPC_CONTROL);
                    slSetPortExt1(SMPC_EXL_DIS);
                }

                if (Gun::Player2MegadriveGunControl)
                {
                    slSetPortDir2(0x00);
                    slSetPortData2(0x7f);
                    slSetPortSelect2(SMPC_SH2_DIRECT);
                }
                else
                {
                    slSetPortSelect2(SMPC_CONTROL);
                    slSetPortExt2(SMPC_EXL_DIS);
                }

                slGetStatus();
                return;
            }

            uint8_t firstPlayerIdTemp = Management::Peripherals[GunPlayer1Port].id;

            if (firstPlayerId != PER_ID_NotConnect && firstPlayerIdTemp == GunMegaId)
            {
                Gun::Player1MegadriveGunControl = true;
                slIntBackCancel();
            }

            uint8_t secondPlayerIdTemp = Management::Peripherals[GunPlayer2Port].id;

            if (secondPlayerId != PER_ID_NotConnect && secondPlayerIdTemp == GunMegaId)
            {
                Gun::Player2MegadriveGunControl = true;
                slIntBackCancel();
            }

            firstPlayerId = firstPlayerIdTemp;
            secondPlayerId = secondPlayerIdTemp;
        }

        /** @brief Refresh light gun trigger and ID data
         * @details This should be called at the beginning of every v-blank
         * @note Used internally
         */
        static inline void VblankRefresh()
        {
            // Handle light gun
            if (SynchCount)
            {
                return;
            }

            if (Gun::Player1MegadriveGunControl && Management::Peripherals[GunPlayer1Port].id == (uint8_t)PeripheralType::Gun)
            {
                uint8_t pd1, pd0, mid;
                slSetPortDir1(0x40);
                pd1 = slGetPortData1();
                slSetPortDir1(0x00);
                pd0 = slGetPortData1();
                mid = Gun::GetMegaDriveId(pd1, pd0);

                if (mid != MEGA_ID_StnShooting)
                {
                    Gun::Player1MegadriveGunControl = false;
                    slIntBackCancel();
                }
            }

            if (Gun::Player2MegadriveGunControl && Management::Peripherals[GunPlayer2Port].id == (uint8_t)PeripheralType::Gun)
            {
                uint8_t pd1, pd0, mid;
                slSetPortDir2(0x40);
                pd1 = slGetPortData2();
                slSetPortDir2(0x00);
                pd0 = slGetPortData2();
                mid = Gun::GetMegaDriveId(pd1, pd0);

                if (mid != MEGA_ID_StnShooting)
                {
                    Gun::Player2MegadriveGunControl = false;
                    slIntBackCancel();
                }
            }
        }

        /** @} */
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
         * @param button Button to check
         * @return true Button was pressed
         * @return false Button was not pressed
         */
        bool virtual IsHeld(const Button& button)
        {
            return (this->GetCurrentFrameState()->data & ((uint16_t)button)) == 0;
        }

        /** @brief Check if user released a button
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
