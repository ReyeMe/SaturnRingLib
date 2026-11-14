// Tests/src/testsInput.hpp
// SRL::Input: digital and analog pads and the peripheral table. Nobody presses buttons in CI, so the tests write the
// pad data themselves (Management::GetRawData). The table is only refreshed by Core::Synchronize, which the tests do
// not call; teardown puts the real data back.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_input.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using SRL::Input::Digital;
using SRL::Input::Analog;
using SRL::Input::Management;
using SRL::Input::PeripheralType;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

/** @brief Copy of the real current and previous pad data, restored after each test. */
static PerDigital input_saved_current[Management::MaxPeripherals];
static PerDigital input_saved_previous[Management::MaxPeripherals];

void input_test_setup(void)
{
    for (uint8_t port = 0; port < Management::MaxPeripherals; port++)
    {
        input_saved_current[port] = *Management::GetRawData(port);
        input_saved_previous[port] = *Management::GetPreviousRawData(port);
    }
}

void input_test_teardown(void)
{
    for (uint8_t port = 0; port < Management::MaxPeripherals; port++)
    {
        *Management::GetRawData(port) = input_saved_current[port];
        *Management::GetPreviousRawData(port) = input_saved_previous[port];
    }
}

void input_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        LogInfo("****UT_INPUT_ERROR(S)****");
    }
}

/** @brief Marks every port as having nothing connected (id 0xff), in this frame and the previous one. */
static void input_disconnect_all()
{
    for (uint8_t port = 0; port < Management::MaxPeripherals; port++)
    {
        Management::GetRawData(port)->id = static_cast<uint8_t>(PeripheralType::NotConnected);
        Management::GetPreviousRawData(port)->id = static_cast<uint8_t>(PeripheralType::NotConnected);
    }
}

/** @brief Pad data word with the given buttons held: the Saturn reports a held button as a 0 bit. */
static uint16_t input_held(uint16_t buttons) { return static_cast<uint16_t>(~buttons); }

/** @brief Writes a pad of the given type on a port: current and previous button data. */
static void input_set_pad(uint8_t port, PeripheralType type, uint16_t previous, uint16_t current)
{
    Management::GetRawData(port)->id = static_cast<uint8_t>(type);
    Management::GetRawData(port)->data = current;
    Management::GetPreviousRawData(port)->id = static_cast<uint8_t>(type);
    Management::GetPreviousRawData(port)->data = previous;
}

/** @brief IsHeld is true exactly for the buttons whose bit is 0. */
MU_TEST(input_digital_is_held)
{
    const uint16_t a = static_cast<uint16_t>(Digital::Button::A);
    const uint16_t up = static_cast<uint16_t>(Digital::Button::Up);
    input_set_pad(0, PeripheralType::Gamepad, input_held(0), input_held(a | up));
    Digital pad(0);

    mu_assert(pad.IsHeld(Digital::Button::A) && pad.IsHeld(Digital::Button::Up), "A and Up are held but IsHeld says no");
    mu_assert(!pad.IsHeld(Digital::Button::B) && !pad.IsHeld(Digital::Button::START), "B or Start reported held");
}

/** @brief WasPressed: not held last frame, held now. WasReleased: held last frame, not now. Held both frames: neither. */
MU_TEST(input_digital_pressed_and_released)
{
    const uint16_t a = static_cast<uint16_t>(Digital::Button::A);
    const uint16_t b = static_cast<uint16_t>(Digital::Button::B);
    const uint16_t c = static_cast<uint16_t>(Digital::Button::C);
    input_set_pad(0, PeripheralType::Gamepad, input_held(a | c), input_held(b | c));
    Digital pad(0);

    mu_assert(pad.WasPressed(Digital::Button::B) && !pad.WasReleased(Digital::Button::B), "B was just pressed");
    mu_assert(pad.WasReleased(Digital::Button::A) && !pad.WasPressed(Digital::Button::A), "A was just released");
    mu_assert(!pad.WasPressed(Digital::Button::C) && !pad.WasReleased(Digital::Button::C), "C is held in both frames");
    mu_assert(!pad.WasPressed(Digital::Button::X) && !pad.WasReleased(Digital::Button::X), "X is not touched in either frame");
}

/** @brief Connection, type and family come from the peripheral id; ports past the table are not connected. */
MU_TEST(input_connection_and_type)
{
    input_disconnect_all();
    input_set_pad(0, PeripheralType::Gamepad, input_held(0), input_held(0));
    input_set_pad(1, PeripheralType::Analog3dPad, input_held(0), input_held(0));

    mu_assert(Management::IsConnected(0) && Management::IsConnected(1), "Ports 0 and 1 have a pad but are not connected");
    mu_assert(!Management::IsConnected(2) && !Management::IsConnected(Management::MaxPeripherals), "An empty or invalid port is connected");
    mu_assert(Management::GetType(0) == PeripheralType::Gamepad && Management::GetType(2) == PeripheralType::NotConnected, "Wrong peripheral type");
    mu_assert(Digital(0).IsConnected() && !Analog(0).IsConnected(), "A gamepad is digital, not analog");
    mu_assert(Analog(1).IsConnected() && !Digital(1).IsConnected(), "A 3D pad is analog, not digital");
}

/** @brief FindNthConnectedPeripheral counts connected ports in order, and gives 0xff past the last one. */
MU_TEST(input_find_nth_connected)
{
    input_disconnect_all();
    input_set_pad(0, PeripheralType::Gamepad, input_held(0), input_held(0));
    input_set_pad(3, PeripheralType::Gamepad, input_held(0), input_held(0));
    input_set_pad(7, PeripheralType::Mouse, input_held(0), input_held(0));

    snprintf(buffer, buffer_size, "Connected ports found: %d, %d, %d, then %d (expected 0, 3, 7, then 255)",
        Management::FindNthConnectedPeripheral(0), Management::FindNthConnectedPeripheral(1),
        Management::FindNthConnectedPeripheral(2), Management::FindNthConnectedPeripheral(3));
    mu_assert(Management::FindNthConnectedPeripheral(0) == 0 && Management::FindNthConnectedPeripheral(1) == 3 &&
        Management::FindNthConnectedPeripheral(2) == 7 && Management::FindNthConnectedPeripheral(3) == 0xff, buffer);
}

/** @brief Analog axes come from the x, y, z (and left trigger) bytes, depending on the pad type. */
MU_TEST(input_analog_axes)
{
    input_disconnect_all();
    input_set_pad(2, PeripheralType::Analog3dPad, input_held(0), input_held(0));
    PerAnalog *data = reinterpret_cast<PerAnalog *>(Management::GetRawData(2));
    data->x = 10;
    data->y = 200;
    data->z = 30;
    data->dummy1 = 40;
    Analog pad(2);

    snprintf(buffer, buffer_size, "3D pad axes %d %d %d %d, expected 10 200 30 40", pad.GetAxis(Analog::Axis1),
        pad.GetAxis(Analog::Axis2), pad.GetAxis(Analog::Axis3), pad.GetAxis(Analog::Axis4));
    mu_assert(pad.GetAxis(Analog::Axis1) == 10 && pad.GetAxis(Analog::Axis2) == 200 && pad.GetAxis(Analog::Axis3) == 30 &&
        pad.GetAxis(Analog::Axis4) == 40, buffer);

    Management::GetRawData(2)->id = static_cast<uint8_t>(PeripheralType::Racing);
    mu_assert(pad.GetAxis(Analog::Axis1) == 10 && pad.GetAxis(Analog::Axis2) == 0, "A racing wheel has only axis 1 (the wheel)");
}

/** @brief RefreshPeripherals moves this frame's data into the previous frame before reading the new data. */
MU_TEST(input_refresh_keeps_previous_frame)
{
    const uint16_t start = static_cast<uint16_t>(Digital::Button::START);
    input_set_pad(5, PeripheralType::Gamepad, input_held(0), input_held(start));

    Management::RefreshPeripherals();

    snprintf(buffer, buffer_size, "Previous frame on port 5: id 0x%02x, data 0x%04x, expected 0x%02x, 0x%04x",
        Management::GetPreviousRawData(5)->id, Management::GetPreviousRawData(5)->data,
        static_cast<uint8_t>(PeripheralType::Gamepad), input_held(start));
    mu_assert(Management::GetPreviousRawData(5)->id == static_cast<uint8_t>(PeripheralType::Gamepad) &&
        Management::GetPreviousRawData(5)->data == input_held(start), buffer);
}

MU_TEST_SUITE(input_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&input_test_setup, &input_test_teardown, &input_test_output_header);

    MU_RUN_TEST(input_digital_is_held);
    MU_RUN_TEST(input_digital_pressed_and_released);
    MU_RUN_TEST(input_connection_and_type);
    MU_RUN_TEST(input_find_nth_connected);
    MU_RUN_TEST(input_analog_axes);
    MU_RUN_TEST(input_refresh_keeps_previous_frame);
}
