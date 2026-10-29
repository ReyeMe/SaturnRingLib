# Profiler Sample

Measure where a program spends its CPU time on a real Saturn, with the SRL function profiler (`srl_profiler.hpp`).

> [!WARNING]
> **Real hardware only.** The profile is read back through the GDB stub, which talks over the USB port of a DevCart.

## How it works

With `SRL_PROFILER = 1` in the makefile, the files in `SRL_PROFILE_SOURCES` are compiled with `-finstrument-functions`: GCC calls a hook on entry and exit of each of their functions. The hooks time every call with the SH-2's free-running timer (FRT, about 4.8 us per tick) and add it to a table in RAM, `g_srl_prof`:

- per function: calls, total time, own time (total minus the functions it called);
- per caller -> callee pair: number of calls.

Each SH-2 has its own tables: code the slave runs (an `ITask` given to `SRL::Slave::ExecuteOnSlave()`) is profiled too. `SRL::Core::Initialize()` sets the slave's timer to the master's rate.

With `SRL_PROFILER_SAMPLE_HZ = N`, a **PC sampler** also counts where the master's program counter is, N times a second. It sees all code, including SGL and files built without hooks, and costs nothing per call.

It also keeps a **timeline**: `SRL_PROF_SCOPE("name")` times the rest of a block, `SRL_PROF_MARK("name")` marks a point in time, and SRL adds a `VBlank` mark every frame and an `slSynch` slice for the wait at the end of each frame. Names must be string literals: only their address is stored, and the PC reads the text from the ELF. Without `SRL_PROFILER`, both macros compile to nothing.

`SRL::Core::Initialize()` starts recording. Two ways to get the data to the PC, then read it with `tools/scripts/srl_prof.py`:

- **GDB**: stop the program, dump the block to a file.
- **Streaming**: the Saturn sends a report over the DevCart every N frames, and `tools/scripts/srl_prof_capture.py` records it. No GDB needed.

| File | What it shows |
|---|---|
| `src/workload.cxx` | Functions that busy-wait a known number of ticks, and a slave task; the only file profiled |
| `src/main.cxx` | Runs the workload every frame inside `SRL_PROF_SCOPE`s, runs the slave task, shows the hook cost on screen |

## Makefile switches

| Setting | Effect |
|---|---|
| `SRL_PROFILER = 1` | Builds the profiler in |
| `SRL_PROFILE_SOURCES = src/a.cxx src/b.cxx` | Files to profile; default: all of `SOURCES` |
| `SRL_PROFILER_REPORT_FRAMES = 60` | Stream a report every 60 frames; `0` (the sample's setting) = GDB only |
| `SRL_PROFILER_SAMPLE_HZ = 8000` | PC sampler on the master; `0` (the sample's setting) = off |
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
wrote 73892 bytes, 651 frames
```

4. Read the profile:

```bash
python3 ../../tools/scripts/srl_prof.py BuildDrop/Debug_Profiler.elf prof.bin
```

```
651 frames, 13115.9 ms; tick 4.80 us; hook pair 15.06 us
hook overhead, both CPUs: 98.2 ms (0.7% of elapsed time)
warning: dropped_events = 1772

master SH-2
 own %  own ms/frame  total ms/frame     calls   us/call  function
   2.4         0.492           0.492      1304     245.5  InnerA()
   2.4         0.483           0.483       652     482.5  Calibrate100()
   0.7         0.143           0.782       652     780.8  Outer()
   0.5         0.102           0.102       652     101.8  InnerB()
   0.1         0.014           0.014       652      13.8  WarmUp()
   0.0         0.007           1.331       652    1329.1  RunWorkload()

slave SH-2
 own %  own ms/frame  total ms/frame     calls   us/call  function
   1.0         0.202           0.202       652     201.4  SlaveWork()
   0.2         0.039           0.292       652     291.4  SlaveWorkTask::Do()
   0.1         0.021           0.021       652      20.6  WarmUp()
```

The flat profile has one table per CPU (the slave's when it ran profiled code) and, with the sampler on, a table of PC samples per function. `--format folded` prints call stacks weighted by own time, for [flamegraph.pl](https://github.com/brendangregg/FlameGraph) or [speedscope](https://www.speedscope.app/). `--format perfetto -o trace.json` writes the timeline (see below). `--cpu slave` picks the slave for `folded` and `gmon`.

Without streaming, the timeline keeps the first 4096 events after the last `Reset()` (the sample fills it in about 10 s); the function tables keep counting.

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

## Sample the PC, read it with gprof

1. Set `SRL_PROFILER_SAMPLE_HZ = 8000` (and `SRL_PROFILER_REPORT_FRAMES = 60` to stream) in the makefile, build, run, and capture or dump as above.
2. Write a `gmon.out` and read it with the toolchain's gprof:

```bash
python3 ../../tools/scripts/srl_prof.py BuildDrop/Debug_Profiler.elf capture.bin --format gmon -o gmon.out
sh-elf-gprof -b BuildDrop/Debug_Profiler.elf gmon.out
```

gprof's time column comes from the PC samples and its call counts from the hooks. Without the sampler, `--format gmon` puts each function's hook-measured own time in the histogram instead.

- Leave the sampler off when timing functions exactly: its interrupt runs inside whatever it interrupts, so its time adds to that function's.
- Samples that land while interrupts are masked (inside the hooks, for example) are taken right after, so very short functions get a few more samples than their share.
- SGL's library has few symbols: its internal code is counted under the nearest exported name (`slInitInterrupt`, for example).
- Read the FRT through `SRL::Timer::Capture()`, not its registers: the sampler reads the FRT too, and a read split around it gets a wrong low byte. `Capture()` masks interrupts across its two reads for that reason.
- With streaming, reports get bigger (8 bytes per non-zero histogram bin): the sample's report grows from 4.8 KB to 9 KB, about 60 ms to send.

To profile only part of the program, call `SRL::Profiler::Stop()`, `Start()` and `Reset()`, or set `g_srl_prof.Head.Enabled` from GDB.

## What to keep in mind

- **Each profiled call costs about 15 us** (one enter + exit hook pair, measured at start-up and shown on screen). The script removes it from each caller's own time, but a function called thousands of times per frame still slows the frame down. Leave small, hot functions out of `SRL_PROFILE_SOURCES`, or mark them `__attribute__((no_instrument_function))`.
- **The first profiled call of a frame is slower**: the hook code is out of the cache, which adds about 1.5 ticks. That's why the workload starts with `WarmUp()`.
- **Interrupts count in the function they land in**, and so does time stopped in GDB.
- **The slave**: its hooks keep their state in uncached memory (both CPUs read it), which adds about a tick to each slave call. Its timer has no overflow interrupt, so a slave function is timed right only if profiled code runs on the slave at least every 0.3 s. DEBUG builds purge the slave's cache before each task, hence `WarmUp()` at the start of `SlaveWorkTask::Do()`. `Reset()` clears the slave's tables too: call it while the slave is idle.
- **Timeline and sampler are master only.**
- `main()` and other functions that never return don't appear as rows; they still show as callers.

## Test

`Tests/Profiler/run_profiler_test.py` builds this sample, runs it on the rig and checks every function's time against the ticks it busy-waits (`src/workload.hpp`), on both CPUs (`--mode gdb|stream|sampler|all`). The streamed run also checks the timeline, compares the slave's clock with the master's, and writes `Tests/Profiler/trace.json`. The sampler run checks the sample rate and each function's share of samples, and that `sh-elf-gprof` reads the `gmon.out`. `tools/scripts/test_srl_prof.py` tests the script without hardware.
