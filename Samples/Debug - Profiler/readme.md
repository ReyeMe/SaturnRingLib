# Profiler Sample

Measure where a program spends its CPU time on a real Saturn, with the SRL function profiler (`srl_profiler.hpp`).

> [!WARNING]
> **Real hardware only.** The profile is read back through the GDB stub, which talks over the USB port of a DevCart.

## How it works

With `SRL_PROFILER = 1` in the makefile, the files in `SRL_PROFILE_SOURCES` are compiled with `-finstrument-functions`: GCC calls a hook on entry and exit of each of their functions. The hooks time every call with the SH-2's free-running timer (FRT, about 4.8 us per tick) and add it to a table in RAM, `g_srl_prof`:

- per function: calls, total time, own time (total minus the functions it called);
- per caller -> callee pair: number of calls.

It also keeps a **timeline**: `SRL_PROF_SCOPE("name")` times the rest of a block, `SRL_PROF_MARK("name")` marks a point in time, and SRL adds a `VBlank` mark every frame and an `slSynch` slice for the wait at the end of each frame. Names must be string literals: only their address is stored, and the PC reads the text from the ELF. Without `SRL_PROFILER`, both macros compile to nothing.

`SRL::Core::Initialize()` starts recording. Two ways to get the data to the PC, then read it with `tools/scripts/srl_prof.py`:

- **GDB**: stop the program, dump the block to a file.
- **Streaming**: the Saturn sends a report over the DevCart every N frames, and `tools/scripts/srl_prof_capture.py` records it. No GDB needed.

| File | What it shows |
|---|---|
| `src/workload.cxx` | Functions that busy-wait a known number of ticks; the only file profiled |
| `src/main.cxx` | Runs the workload every frame inside `SRL_PROF_SCOPE`s and shows the hook cost on screen |

## Makefile switches

| Setting | Effect |
|---|---|
| `SRL_PROFILER = 1` | Builds the profiler in |
| `SRL_PROFILE_SOURCES = src/a.cxx src/b.cxx` | Files to profile; default: all of `SOURCES` |
| `SRL_PROFILER_REPORT_FRAMES = 60` | Stream a report every 60 frames; `0` (the sample's setting) = GDB only |
| `DEBUG = 1` | Builds the GDB stub in, needed to read the profile |
| `OPT_FLAGS = -O2` | Profile optimised code: `DEBUG` alone means `-O0`, which gives misleading profiles |

SRL, SGL and C++ library headers are never profiled; their inline code counts as the caller's own time. Run `make clean` after changing these settings.

## Read it with GDB

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

`--format folded` prints call stacks weighted by own time, for [flamegraph.pl](https://github.com/brendangregg/FlameGraph) or [speedscope](https://www.speedscope.app/). `--format perfetto -o trace.json` writes the timeline (see below).

## Stream it, without GDB

1. Set `SRL_PROFILER_REPORT_FRAMES = 60` in the makefile, then build: `../../tools/scripts/make.sh`
2. Upload as in the GDB Stub sample, then start the relay: `ftx -g 1234`.
3. Record a few seconds, then read the capture like a dump:

```bash
python3 ../../tools/scripts/srl_prof_capture.py capture.bin --seconds 10
python3 ../../tools/scripts/srl_prof.py BuildDrop/Debug_Profiler.elf capture.bin
python3 ../../tools/scripts/srl_prof.py BuildDrop/Debug_Profiler.elf capture.bin --format perfetto -o trace.json
```

Open `trace.json` in [ui.perfetto.dev](https://ui.perfetto.dev): one row for the master SH-2 with the `Workload`, `Status text`, `slSynch` and `SRL profiler report` slices, and a `VBlank` mark per frame. The flat profile uses the function tables of the last report.

- **A report costs time.** The DevCart takes about 256 KB/s: the sample's 3.7 KB report takes 14 ms, most of a frame. The report shows on the timeline, and the flat profile prints its size and time. Bigger programs send bigger tables (16 bytes per function, 12 per caller -> callee pair), so raise `SRL_PROFILER_REPORT_FRAMES` if the hitch matters.
- **GDB and streaming don't mix.** `ftx -g` takes one client: GDB or the capture. The Saturn stops streaming while GDB is connected, but reports sent before GDB connects would reach it, so keep `SRL_PROFILER_REPORT_FRAMES = 0` when you use GDB.
- If nobody reads the DevCart for about 10 ms, the report is dropped (`dropped_reports` in the output) and the program carries on.
- Log output on the DevCart (`SRL_LOG_OUTPUT = DEV_CART`) can share the link: the script skips bytes outside report packets.

To profile only part of the program, call `SRL::Profiler::Stop()`, `Start()` and `Reset()`, or set `g_srl_prof.Head.Enabled` from GDB.

## What to keep in mind

- **Each profiled call costs about 14 us** (one enter + exit hook pair, measured at start-up and shown on screen). The script removes it from each caller's own time, but a function called thousands of times per frame still slows the frame down. Leave small, hot functions out of `SRL_PROFILE_SOURCES`, or mark them `__attribute__((no_instrument_function))`.
- **The first profiled call of a frame is slower**: the hook code is out of the cache, which adds about 1.5 ticks. That's why the workload starts with `WarmUp()`.
- **Interrupts count in the function they land in**, and so does time stopped in GDB.
- **Master CPU only.** Calls made on the slave SH-2 are ignored.
- `main()` and other functions that never return don't appear as rows; they still show as callers.

## Test

`Tests/Profiler/run_profiler_test.py` builds this sample, runs it on the rig and checks every function's time against the ticks it busy-waits (`src/workload.hpp`), once through GDB and once streamed (`--mode gdb|stream|all`). The streamed run also checks the timeline and writes `Tests/Profiler/trace.json`. `tools/scripts/test_srl_prof.py` tests the script without hardware.
