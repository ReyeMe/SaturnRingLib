# How to execute Tests

## Requirements

Only Linux OS is supported for now

## Kronos installation

Kronos can be downloaded from https://github.com/FCare/Kronos,
either use the Ubuntu binary delivered with each release, or build it from source.

## Kronos configuration

A few basic things are required for Kronos to run:
- Disable Wayland
- In Setting/General/BIOS Settings: a BIOS is required
- In Setting/Cart-Memory/Cartridge: Select Development Extension

Close Kronos, everything shall be setup at that point.

## Test campaigns

Tests are split into campaigns. Each campaign is its own disc image, so a crash or hang in one does not hide the results of the others.

| Campaign | What it tests |
|---|---|
| `math` | Fixed point, angles, math types, strings. No hardware state. |
| `memory_simple` | Allocators and memory zones (HWRam, LWRam, CartRam) with the `SIMPLE` allocator. |
| `memory_tlsf` | The same memory tests with the `TLSF` allocator (the library default). |
| `memory_cpus_simple`, `memory_cpus_tlsf` | Master and slave CPUs allocating at the same time, on a fresh heap, with each allocator. |
| `video` | ASCII, color RAM, bitmaps, SGL. |
| `system` | Interrupts, timers, system registers, SCU DSP. |
| `cd` | CD block and file system. Uses the files in `campaigns/cd/cd/data/`. |

Test headers live in `src/`. Each campaign is a folder in `campaigns/` with a `makefile` and a short `main.cxx` that lists its suites.
Shared build settings are in `campaign.mk`.

To add a suite, include its header and add a `RUN_AND_DISPLAY_SUITE(...)` line in the campaign's `main.cxx`.
To add a campaign, copy a folder in `campaigns/`, change `CD_NAME` in its `makefile`, and add its name to `CAMPAIGNS` in `makefile`.

## Build the tests

Execute `make all` within the `SaturnRingLib/Tests` directory from a terminal to build every campaign, or `make <campaign>` (e.g. `make memory_tlsf`) to build one.
Each campaign's disc image is written to `campaigns/<campaign>/BuildDrop/`.

You can override makefile defaults at build time. For example, to change the log output target:

```bash
make all SRL_LOG_OUTPUT=DEV_CART
```

Other valid values: `EMULATOR`, `DEV_CART`, `NONE`.

Campaigns use the `SIMPLE` allocator, except `memory_tlsf` and `memory_cpus_tlsf`, which use `TLSF`.

## Run the tests

Execute `run_tests.bat <emulator> <campaign>` within the `SaturnRingLib/Tests` directory from a terminal, e.g. `./run_tests.bat mednafen math`.
Tests results will be output in both the terminal and `SaturnRingLib/Tests/uts_<campaign>.log`.
The script exits with an error if any test fails. To run every campaign:

```bash
for c in math memory_simple memory_tlsf memory_cpus_simple memory_cpus_tlsf video system cd; do ./run_tests.bat mednafen $c; done
```

The log looks like this:

````
INFO : ***UT_START***
INFO : ****UT_ANGLE_ERROR(S)****
FATAL : angle_test_operator_addition_wrap_around failed:
	src/testsAngle.hpp:900: Addition wrap-around failed: 9 != 10
FATAL : angle_test_operator_subtraction_wrap_around failed:
	src/testsAngle.hpp:910: Subtraction wrap-around failed: 350 != 350
...
FATAL : memory_test_memory_leaks failed:
	src/testsMemory.hpp:844: Memory allocation failed
INFO : ****UT_bitmap_ERROR(S)****
FATAL : palette_test_initialization failed:
	src/testsBitmap.hpp:77: Palette colors not allocated correctly: 0
INFO : 219 tests, 327 assertions, 23 failures
INFO : Finished in f seconds (real) f seconds (proc)
INFO : ***UT_END***
````

Note : Changing SRL_LOG_LEVEL from INFO to TRACE in the makefile will make an output for every single tests.

## USBGamers (real hardware)

The `USBGamers` mode in `run_tests.bat` pushes the test binary to a USBGamers cartridge using `ftx`.

### Requirements

- USBGamers cartridge connected to the Saturn.
- `ftx` tool available in your PATH.

#### Linux Setup

`usbreset` tool shall be available in your PATH (used to reset the USB device).

#### Windows Driver Setup

On Microsoft Windows, the utility communicates with the Sega Saturn USB cartridge via `libusb` and `libftdi`. Since Windows installs the standard FTDI Serial Port (COM) driver by default, you must replace it with the generic **WinUSB** driver for the utility to detect and access the device.

To install the WinUSB driver:
1. Connect the USB cartridge to your Windows PC.
2. Download and run **Zadig** from [zadig.akeo.ie](https://zadig.akeo.ie/).
3. In Zadig, select **Options** -> **List All Devices** from the top menu.
4. In the main drop-down menu, select your cartridge device (typically shown as `FT245R USB FIFO` or similar).
5. Verify that the USB ID matches the target device (the default is VID `0403` and PID `6001`).
6. Select **WinUSB** as the target driver (to the right of the green arrow).
7. Click **Replace Driver** (or **Install Driver**) and wait for the process to finish.

*(Note: To revert to the standard FTDI virtual COM port drivers, you can uninstall the device from the Windows Device Manager and check the option to delete the driver software for the device, then replug it.)*

### Run

1. Build tests with `make all SRL_LOG_OUTPUT=DEV_CART`.
2. Run `./run_tests.bat USBGamers <campaign> [device-ip]`. The optional `device-ip` is the network power switch used to power-cycle the Saturn.

The script resets the USB device, uploads `campaigns/<campaign>/cd/data/0.bin` to address `0x06004000`, and then starts the capture flow.
The output still goes to `uts_<campaign>.log` like the emulator runs.
The `cd` campaign fails over USB: the program is uploaded without a disc.

