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

## Build the tests

Execute `make all` within the `SaturnRingLib/Tests` directory from a terminal.

## Run the tests

Execute `run_tests.bat` within the `SaturnRingLib/Tests` directory from a terminal, tests results will be output in both the terminal and `SaturnRingLib/Tests/uts.log`, such as:

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