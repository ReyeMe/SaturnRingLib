# Profiler Sample

Measure where a program spends its CPU time on a real Saturn, with the SRL function profiler (`srl_profiler.hpp`).

> [!WARNING]
> **Real hardware only.** The profile is read back through the GDB stub, which talks over the USB port of a DevCart.

## How it works

With `SRL_PROFILER = 1` in the makefile, the files in `SRL_PROFILE_SOURCES` are compiled with `-finstrument-functions`: GCC calls a hook on entry and exit of each of their functions. The hooks time every call with the SH-2's free-running timer (FRT, about 4.8 us per tick) and add it to a table in RAM, `g_srl_prof`:

- per function: calls, total time, own time (total minus the functions it called);
- per caller -> callee pair: number of calls.

`SRL::Core::Initialize()` starts recording. Nothing leaves the Saturn by itself: you stop the program with GDB, dump the table to a file, and read it on the PC with `tools/scripts/srl_prof.py`.

| File | What it shows |
|---|---|
| `src/workload.cxx` | Functions that busy-wait a known number of ticks; the only file profiled |
| `src/main.cxx` | Runs the workload every frame and shows the hook cost on screen |

## Makefile switches

| Setting | Effect |
|---|---|
| `SRL_PROFILER = 1` | Builds the profiler in |
| `SRL_PROFILE_SOURCES = src/a.cxx src/b.cxx` | Files to profile; default: all of `SOURCES` |
| `DEBUG = 1` | Builds the GDB stub in, needed to read the profile |
| `OPT_FLAGS = -O2` | Profile optimised code: `DEBUG` alone means `-O0`, which gives misleading profiles |

SRL, SGL and C++ library headers are never profiled; their inline code counts as the caller's own time. Run `make clean` after changing these settings.

## Run it

1. Build: `../../tools/scripts/make.sh`
2. Upload, start `ftx -g 1234 -v` and connect GDB as in the [GDB Stub sample](../Debug%20-%20GDB%20Stub/readme.md) (steps 3 and 4).
3. Let the program run a few seconds, then:

```
(gdb) continue
^C
(gdb) source ../../tools/scripts/srl_prof.gdb
(gdb) srl-prof-dump prof.bin
wrote 20544 bytes, 471 frames
```

4. Read the profile:

```bash
python3 ../../tools/scripts/srl_prof.py BuildDrop/Debug_Profiler.elf prof.bin
```

```
471 frames, 10071.8 ms; tick 4.80 us; hook pair 14.16 us
hook overhead: 46.7 ms (0.5% of elapsed time)

 own %  own ms/frame  total ms/frame     calls   us/call  function
   2.3         0.486           0.486       942     242.6  InnerA()
   2.3         0.485           0.485       471     484.4  Calibrate100()
   0.7         0.148           0.776       471     774.3  Outer()
   0.5         0.099           0.099       471      98.6  InnerB()
   0.1         0.014           0.014       471      13.9  WarmUp()
   0.0         0.006           1.324       471    1321.6  RunWorkload()
```

`--format folded` prints call stacks weighted by own time, for [flamegraph.pl](https://github.com/brendangregg/FlameGraph) or [speedscope](https://www.speedscope.app/).

To profile only part of the program, call `SRL::Profiler::Stop()`, `Start()` and `Reset()`, or set `g_srl_prof.Head.Enabled` from GDB.

## What to keep in mind

- **Each profiled call costs about 14 us** (one enter + exit hook pair, measured at start-up and shown on screen). The script removes it from each caller's own time, but a function called thousands of times per frame still slows the frame down. Leave small, hot functions out of `SRL_PROFILE_SOURCES`, or mark them `__attribute__((no_instrument_function))`.
- **The first profiled call of a frame is slower**: the hook code is out of the cache, which adds about 1.5 ticks. That's why the workload starts with `WarmUp()`.
- **Interrupts count in the function they land in**, and so does time stopped in GDB.
- **Master CPU only.** Calls made on the slave SH-2 are ignored.
- `main()` and other functions that never return don't appear as rows; they still show as callers.

## Test

`Tests/Profiler/run_profiler_test.py` builds this sample, runs it on the rig and checks every function's time against the ticks it busy-waits (`src/workload.hpp`). `tools/scripts/test_srl_prof.py` tests the script without hardware.
