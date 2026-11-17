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
| `math` | Fixed point, angles, math types. No hardware state. |
| `memory_simple` | Allocators and memory zones (HWRam, LWRam, CartRam) with the `SIMPLE` allocator. |
| `memory_tlsf` | The same memory tests with the `TLSF` allocator (the library default). |
| `memory_cpus_simple`, `memory_cpus_tlsf` | Master and slave CPUs allocating at the same time, on a fresh heap, with each allocator. |
| `video` | ASCII (text map cells, fonts, palettes and colors), color RAM, bitmaps, SGL. |
| `system` | Interrupts, system registers, BIOS semaphores, SCU DSP. |
| `timer` | Timers: FRT tickstamps, dividers, conversions, overflow. |
| `event` | Events (`Event`, `MemberProxy`) and the Core sync / v-blank events. |
| `datetime` | Date and time (`DateTime`): ordering, leap years, backup unit dates, RTC. |
| `cd` | CD block and file system. Uses the files in `campaigns/cd/cd/data/`. |
| `media` | TGA images, WAV sounds and CubeTile tilemaps loaded from the disc. Its files in `campaigns/media/cd/data/` are written by `campaigns/media/make_data.py`. |
| `slave` | Tasks on the slave CPU (`Slave`, `ITask`). |
| `scene` | Mesh and sprite attributes, `Mesh`/`SmoothMesh`, `Scene3D` projection, visibility and drawing. |
| `string` | `SRL::string` and `snprintfEx` formatting, `SRL::Logger` output. |
| `endian` | Reading little-endian numbers from byte buffers (`SRL::Endian`). |
| `vdp1` | VDP1 texture heap: placement, loading, limits, free memory, reset. Built with `SRL_MAX_TEXTURES = 8`. |
| `vdp2` | VDP2 scroll, sprite and rotation settings (priority, opacity, display, transparency, color offset and calculation, back color, scale), VRAM allocator, `Bmp2Tile`, loading a tilemap and a bitmap into NBG1, `ClearVRAM`, TV resolution. NBG3 is left alone: it shows the results. |
| `input` | Digital and analog pads, from pad data the tests write themselves. |
| `sound` | PCM channels with the SGL sound driver (`SRL_USE_SGL_SOUND_DRIVER = 1`), raw PCM loading, CD audio (`Cdda`) on two CD-DA tracks, Cinepak movies (`CinepakPlayer`). `campaigns/sound/pre.makefile` writes the tracks with `sox` and copies the movie of the `VDP1 - Cinepak` sample. |
| `transform` | `Scene3D` matrix operations: the matrix stack, translate, rotate, scale, multiply, invert, transpose, `LookAt`. |
| `tilemap` | `TilemapInfo`, `SGLTile`, `Bmp2Tile` conversion and map editing, loading tilemaps into NBG1 (1-word and 2-word maps). |
| `draw2d` | `Scene2D` drawing: effects, sprite attributes, and the VDP1 commands lines, polygons, clipping and sprites put in VDP1 VRAM. |
| `fld` | File loader (`SRL::Fld`): the campaign program is a menu that starts `campaigns/fld_game` from a sub-directory of its disc; the game checks its root directory and CD-DA tracks and reports. `fld_game` is built by `campaigns/fld/pre.makefile`, it is not a campaign of its own. |

Test headers live in `src/`. Each campaign is a folder in `campaigns/` with a `makefile` and a short `main.cxx` that lists its suites.
Shared build settings are in `campaign.mk`.

To add a suite, include its header and add a `RUN_AND_DISPLAY_SUITE(...)` line in the campaign's `main.cxx`.

A suite can set things up at two levels:

```cpp
MU_TEST_SUITE(my_test_suite)
{
    MySuiteSetup();                                  // once, before the suite's first test

    MU_SUITE_CONFIGURE(&my_setup, &my_teardown);     // around each test; either can be NULL
    MU_RUN_TEST(my_first_test);
    MU_RUN_TEST(my_second_test);

    MySuiteTeardown();                               // once, after the last test
}
```

A failed check only ends its own test, so the per-test teardown and the suite's last lines still run.
To add a campaign, copy a folder in `campaigns/`, change `CD_NAME` in its `makefile`, add its name to `CAMPAIGNS` in `makefile` and to the CI matrix in `.github/workflows/main.yml`, and give it an entry in `campaigns.json`.

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
The script exits with an error if a test fails that is not listed under the campaign's `known_failures` in `campaigns.json`.
That file holds the settings of every campaign: `known_failures` maps each test that fails because of a known library bug to that bug (remove the entry once it is fixed), and `skip_modes` maps a `run_tests.bat` mode to the reason the campaign is not run in it.
Listed tests that pass are printed as a reminder. A test that would crash or hang the console skips itself and fails, so it can be listed too.
To run every campaign:

```bash
for c in math memory_simple memory_tlsf memory_cpus_simple memory_cpus_tlsf video system timer event datetime cd media slave scene string endian vdp1 vdp2 input sound transform tilemap draw2d fld; do ./run_tests.bat mednafen $c; done
```

The log looks like this:

````
INFO : ***UT_START***
INFO : ****UT_angle_test_suite_ERROR(S)****
FATAL : angle_test_operator_addition_wrap_around failed:
	src/testsAngle.hpp:900: Addition wrap-around failed: 9 != 10
FATAL : angle_test_operator_subtraction_wrap_around failed:
	src/testsAngle.hpp:910: Subtraction wrap-around failed: 350 != 350
...
FATAL : memory_test_memory_leaks failed:
	src/testsMemory.hpp:844: Memory allocation failed
INFO : ****UT_bitmap_test_suite_ERROR(S)****
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
Campaigns with `USBGamers` in their `skip_modes` in `campaigns.json` are skipped in this mode with exit 0. `cd`, `media`, `sound` and `fld` are: the upload sends only the program, not the disc (`sound` loads its driver from it, `fld` its game).

