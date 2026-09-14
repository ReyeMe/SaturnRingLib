# GDB Stub Sample

This sample demonstrates how to use the Sega Saturn GDB stub (`srl_gdbstub.hpp`) to debug your game running on real hardware.

> [!WARNING]
> **Real Hardware Required:** This sample utilizes the USB interface on a DevCart with USB support. It **cannot** be run in emulators like Mednafen, Kronos, or Yabause, because they do not emulate this custom USB hardware. Attempting to run this in an emulator will likely cause it to hang or crash.


## 0. Get GDB Multiarch

https://static.grumpycoder.net/pixel/gdb-multiarch-windows/


## 1. Building the Sample

To build the sample, open a terminal in this directory and run the provided make script. The `makefile` is already configured to include the `-g` flag (for debug symbols).

```bash
# From inside Samples/Debug - GDB Stub/
../../tools/scripts/make.sh
```

This will produce `./BuildDrop/Debug_GDBStub.elf` (which contains your debug symbols for GDB) and `./BuildDrop/Debug_GDBStub.bin` (which is the executable payload).

## 1.1 VS Code Debug Configuration

This sample includes a `.vscode` folder with `launch.json` and `tasks.json` for debugging in VS Code.

- `launch.json` connects to the remote GDB stub on `localhost:1234` using `gdb-multiarch`.
- It configures `preLaunchTask` (`Start FTX`) to automatically launch the `ftx` GDB proxy in the background on port 1234 if it is not already running.
- It loads `./BuildDrop/Debug_GDBStub.elf` and configures SH-2 architecture and big-endian mode before starting the debug session.
- `sourceFileMap` maps `src` to `${workspaceFolder}/src` so breakpoints resolve correctly.

The `.vscode/tasks.json` file provides these tasks:

- `Compile [DEBUG]` — runs `../../tools/scripts/make.sh` from this sample folder.
- `Run on Saturn` — runs `../../tools/scripts/run.sh USBGamers` from this sample folder.
- `Start FTX` — checks if port 1234 is listening on `localhost` and launches `ftx -g 1234 -v` using `nohup` in the background if not started.
- `Connect to GDB` — checks/starts `ftx` on port 1234 if needed and connects `gdb-multiarch` CLI in the terminal.

### `launch.json` Configuration

```json
{
    "name": "Debug - GDB Stub (Debug - GDB Stub)",
    "type": "cppdbg",
    "request": "launch",
    "program": "${workspaceFolder}/BuildDrop/Debug_GDBStub.elf",
    "stopAtEntry": true,
    "cwd": "${workspaceFolder}",
    "environment": [],
    "externalConsole": false,
    "MIMode": "gdb",
    "miDebuggerPath": "gdb-multiarch",
    "miDebuggerServerAddress": "localhost:1234",
    "miDebuggerArgs": "-q -ex \"set architecture sh2\" -ex \"set endian big\"",
    "preLaunchTask": "Start FTX",
    "sourceFileMap": {
        "src": "${workspaceFolder}/src"
    }
}
```

> Note: If using Docker, set `"miDebuggerServerAddress": "host.docker.internal:1234"`. Use `sh-elf-gdb` instead of `gdb-multiarch` if that is your installed SH-2 GDB executable.

## 1.2 Debugging with VS Code: A Walkthrough

This walks through an actual debug session using the VS Code UI, rather than
the CLI `gdb-multiarch` flow in section 3 below (both talk to the exact same
stub over the exact same `ftx` proxy -- pick whichever fits how you work).

**Prerequisites:** the [C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) (`ms-vscode.cpptools`) installed, and `gdb-multiarch` (or `sh-elf-gdb`) + `ftx` on `PATH` (see the host tool checks in section 2).

1. **Power up and get the console ready.** Follow the hardware test flow in
   section 2 below at least once manually (power cycle, `usbreset`, upload)
   so you know the physical link is healthy before asking VS Code to manage
   it for you.
2. **Open the Run and Debug view** (`Ctrl+Shift+D` / `Cmd+Shift+D`, or the
   icon in the Activity Bar).
3. **Pick a configuration** from the dropdown at the top: `Debug - GDB Stub
   (Debug - GDB Stub)` connects to whatever is already running on the
   console; `Debug - GDB Stub (Upload + Debug)` builds, uploads, and
   connects in one go (its `preLaunchTask` is `Upload and Start FTX`, which
   is why it takes longer to hit the first breakpoint than the other
   configuration).
4. **Press F5** (or the green play arrow). VS Code runs the `preLaunchTask`
   (starting `ftx -g 1234` if it isn't already listening, per
   `tasks.json`), then connects. Because `stopAtEntry` is `true`, the
   session immediately halts -- you'll land inside the stub's own
   `snapshot_polling_context()`, not your own code (see section 3's note on
   why that's expected: `Poll()` caught you between frames, not at a real
   breakpoint).
5. **Set a real breakpoint**: click in the gutter to the left of a line
   number in `main.cxx` or `debug_triggers.cxx` (a red dot appears), then
   press **Continue** (the play icon in the debug toolbar, or `F5` again).
   Try the loop-counter print in `main.cxx`'s main loop for something that
   hits every frame, or `SteppableFunction()` in `debug_triggers.cxx` for a
   clean single-step target.
6. **Step through code** with the toolbar's Step Over / Step Into / Step Out
   (`F10` / `F11` / `Shift+F11`) -- these map directly to the stub's
   `step`/`next` implementation described under *Useful GDB Commands* below.
7. **Inspect variables**: hover over a variable in the editor while stopped,
   or add it to the **Watch** panel (e.g. `g_testVariable`). Watch
   expressions aren't limited to named variables -- `*(unsigned short*)0x25F80020`
   works too, which is exactly how you read VDP1/VDP2 hardware registers --
   the stub doesn't expose those as named registers at all (see the slave
   pseudo-register limitation further down this readme, which covers the
   one register category the stub does try to expose by name).
8. **Run `monitor`/other raw GDB commands via the Debug Console**: open it
   (it's the tab next to Terminal, or auto-opens with the session), and
   prefix any GDB command with `-exec `. This is how you reach everything
   under *Useful GDB Commands* below that isn't a toolbar button, e.g.:
   ```
   -exec monitor step
   -exec monitor touch
   -exec monitor crash illegal
   -exec monitor regs slave
   -exec monitor trace
   -exec watch g_testVariable
   ```
9. **End the session** with the Stop button (`Shift+F5`) -- this sends a
   clean `D` (detach), which the stub is guaranteed to recover from (see the
   `SlaveReleaseGuard` note below) as long as nothing else about the target
   has wedged first.

> [!TIP]
> If a session won't connect or breakpoints silently never hit, don't just
> keep retrying inside VS Code -- fall back to the manual CLI flow in
> section 3 first. A raw `gdb-multiarch` session gives you the actual wire
> log and error text VS Code's UI hides, which is usually the fastest way to
> tell "the stub/hardware is in a bad state, power-cycle" apart from "this
> specific launch.json setting is wrong."

## 2. Running on the Saturn (Hardware Test Flow)

To ensure a stable upload and execution environment on real hardware, it is highly recommended to follow the clean hardware state sequence before uploading your payload:

### Optional Host Tools

Make sure these tools are installed and available in `PATH` before starting:

- [`usbreset`](https://man7.org/linux/man-pages/man1/usbreset.1.html) (reset FT245R endpoint)

### Required Host Tools

Make sure these tools are installed and available in `PATH` before starting:

- [`ftx`](https://github.com/willll/ftx) (USBGamers uploader)
- `gdb-multiarch` or `sh-elf-gdb` (debugger)

Quick sanity checks:

```bash
command -v usbreset
command -v ftx
command -v gdb-multiarch
```

1. **Power Cycle:** Turn your Sega Saturn completely off and then back on.
2. **Reset USB Cartridge:** Reset the state of your USB cartridge on the PC to avoid stale buffer issues:
   ```bash
   usbreset "FT245R USB FIFO"
   ```
3. **Wait:** Give the Saturn a short settle window (approx 10 seconds) after the power cycle.
4. **Probe Cartridge Link:** Verify the USB link is alive before upload:
   ```bash
   ftx -c
   ```
   If this returns `device not found`, repeat the power-cycle + `usbreset` sequence.
5. **Upload:** Use the built-in run script to upload the `.bin` payload directly to the Saturn using `ftx`:
   ```bash
   ../../tools/scripts/run.sh USBGamers
   ```

> [!TIP]
> **Troubleshooting:** If the upload fails with `usb bulk write failed`, rerun the full sequence above (Power Cycle -> `usbreset` -> Wait 10s) before retrying the upload.

Once successfully uploaded, the code boots, initializes the GDB stub, and starts running its main loop immediately -- it does **not** halt itself waiting for GDB. Instead, every frame it calls `SRL::Core::Synchronize()`, which internally calls `SRL::GDBStub::Poll()`; that is what notices an incoming GDB connection (or a `Ctrl-C`) and drops the program into the RSP command loop. In practice this means: just connect GDB at any time (see below) and it will catch the very next `Poll()` call, usually within a frame.

## 3. Attaching GDB

Open a new terminal window on your PC. You need to use the SH-2 cross-compiled version of GDB, and you must pass it the `.elf` file so it knows the memory layout and symbols of your program.

```bash
# Launch GDB with the ELF file
sh-elf-gdb ./BuildDrop/Debug_GDBStub.elf
```

or with multiarch GDB:

```bash
gdb-multiarch ./BuildDrop/Debug_GDBStub.elf
```

Once inside the `(gdb)` prompt, connect to the Saturn over the `ftx` TCP interface.

```sh
ftx -g 1234 -v
```

```gdb
(gdb) set endian big
(gdb) target remote host.docker.internal:1234
```

> Note: `target remote` (not `target extended-remote`) is what's actually been verified against real hardware -- the stub does not implement the extra run-control packets (`vAttach`, `vRun`, `vKill`, ...) that `extended-remote` sessions can rely on. You generally don't need `set architecture` at all; GDB picks up "sh2" from the ELF automatically. If you do set it explicitly, be aware some GDB builds print `warning: Target-supplied registers are not supported by the current architecture` -- this is expected and harmless (see the pseudo-register caveat below).

Ftx will print the communication log :

```ftx
[TCPProxy] listening on port 1234
[TCPProxy] client connected
GDB>+
GDB>$qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-events+;exec-events+;vContSupported+;QThreadEvents+;QThreadOptions+;no-resumed+;memory-tagging+;xmlRegisters=i386;error-message+#14
GDB>$qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-events+;exec-events+;vContSupported+;QThreadEvents+;QThreadOptions+;no-resumed+;memory-tagging+;xmlRegisters=i386;error-message+#14
Target>+
Target>$PacketSize=400;swbreak+;qXfer:features:read+#f4
GDB>+
GDB>$vCont?#49
...
```

and GDB will show something like:

```gdb
...
Remote debugging using localhost:1234
SRL::GDBStub::snapshot_polling_context () at /path/to/saturnringlib/srl_gdbstub.hpp:738
738	            asm volatile("sts pr, %0" : "=r"(pr));
(gdb) 
```

That stop location (inside the stub's own `snapshot_polling_context()`) is expected the first time you connect -- `Poll()` caught you between frames, not at a breakpoint, so the "current line" is a synthetic frame the stub builds for GDB's benefit. Set a real breakpoint and `continue` to get somewhere meaningful, e.g. `break main.cxx:391` (the loop-counter print, which runs every frame).

### Useful GDB Commands:
- `info registers`: View the state of all SH-2 CPU registers.
- `break <file>:<line>`: Set a software breakpoint.
- `continue` or `c`: Resume game execution.
- `step` / `next`: Single-step. The stub implements this by decoding the current instruction (handling branches, delay slots, `TRAPA`) and placing a temporary trap at wherever comes next -- there's no SH-2 hardware single-step. Try it on `SteppableFunction()` (D-Pad Up in this sample) for a clean, branch-free target.
- `watch <var>` / `rwatch` / `awatch`: Hardware watchpoints via the SH-2's UBC. Try `watch g_testVariable`, then `continue`, then press D-Pad Down (or `monitor touch`).
- `set variable <var> = <value>`: Write memory. Try `set variable g_testVariable = 99` then `print g_testVariable`. This also works for arbitrary target addresses, e.g. `set *(unsigned short*)0x25F00042 = 0x1234` to poke a VDP2 CRAM color directly -- **fixed, but worth knowing about**: GDB's `M` packet (which both `set variable` and `set *(T*)addr = val` send) used to be decoded one byte at a time (`hex2mem()`), and VDP RAM (CRAM in particular) does not reliably latch single-byte bus writes -- a 16-bit color write would silently drop its high byte on real hardware (`0xEC63` read back as `0x0063`). The `M` handler now uses `hex2mem_aligned()`, which stores whole 32-/16-bit words where address and length allow, matching the bus cycle width VDP RAM actually needs. Confirmed fixed on real hardware.
- `Ctrl-C`: Pause the running game (requires the game to periodically call `SRL::GDBStub::Poll()`, which this sample does via `SRL::Core::Synchronize()` every frame).
- `x/10xh 0x25F80000`: Examine memory directly -- e.g. VDP2 registers, which the stub doesn't expose as named registers at all (see the slave pseudo-register section below for the one register category it does try to expose by name, and why that still needs memory examination too).
- `monitor <text>`: Sends the text to the target via `qRcmd`, decoded and dispatched by this sample's `HandleMonitorCommand()`. Lets you trigger any of the test paths below without a gamepad -- handy for scripted/headless testing over a raw `ftx -g` connection:
  - `monitor crash illegal|addr|reserved|slotillegal|slotreserved|genillegal|dma|ubc|trapa3` -- same as the B/A/C/X/Y/Z/L/START buttons.
  - `monitor step` -- same as D-Pad Up (`SteppableFunction()`).
  - `monitor touch` -- same as D-Pad Down (increments `g_testVariable`, useful with `watch`).
  - The dispatch only happens once the target resumes (`continue`/detach), and only the *last* `monitor` command sent while stopped takes effect -- send one, `continue`, then repeat, rather than queuing several while paused.

### Tutorial: Changing a Background Tile's Color

This sample's floor (`RBG0`) and ceiling (`NBG1`) are tile-based, paletted
backgrounds (see `SetupSkyAndFloor()` in `vdp_demo.cxx`) -- each tile pixel
is an index into a small VDP2 CRAM palette, not a direct color. So "change
the tile color" means editing that palette entry in CRAM, the same
mechanism as the CRAM caveat already noted above (`set variable` bullet),
just walked through end-to-end with real addresses from this sample.

> Both `FLOOR.TGA`/`CEIL.TGA` are loaded from CD if present, but real-hardware
> testing (see `LoadTextureOrFallback()`'s doc comment in `vdp_demo.cxx`)
> confirms this sample's usual `ftx -x` direct-RAM run has no disc mounted,
> so what's actually on screen is the in-memory `CheckerBitmap` fallback --
> a 2-color palette per plane: index 0 (`colorB`, always hardware-transparent
> for these planes and therefore never actually seen on screen) and index 1
> (`colorA`, the tile color you'll see and are changing below).

**1. Find the palette's address.** Each scroll screen's CRAM palette is
allocated at runtime (`SRL::CRAM::GetFreeBank()` inside `LoadTilemap()`, see
`srl_vdp2.hpp`), not at a fixed compile-time address, so the reliable way to
find it is to ask the live target rather than hardcode a guess:

```gdb
(gdb) print SRL::VDP2::RBG0::TilePalette
$1 = {data = 0x25f00040 <...>, paletteMode = Paletted16, id = 2}
(gdb) print SRL::VDP2::NBG1::TilePalette
$2 = {data = 0x25f00060 <...>, paletteMode = Paletted16, id = 3}
```

`data` is the CRAM address of palette index 0 (`colorB`) for that screen;
index 1 (`colorA`, the actual tile color) is 2 bytes further in, since each
`Types::HighColor` entry is 2 bytes (`srl_cram.hpp`'s `Palette::data` is a
`HighColor*`, so `data[1]` == `data + 0x2` in raw bytes).

For this exact build, that resolves to two addresses -- hardware-confirmed
by literally running the `print` commands above (an earlier draft of this
section guessed `RBG0 -> bank 0, NBG1 -> bank 2` from reading
`SetupSkyAndFloor()`'s own `SetBankUsedState(1, ...)` call in isolation;
live readback caught that guess as wrong before it shipped -- see the
*why* below):

| Screen | `colorA` address (tile color) |
|---|---|
| `RBG0` (floor) | `0x25F00042` |
| `NBG1` (ceiling) | `0x25F00062` |

(`0x25F00042` is the exact address already used as the `M`-packet example
in the *Useful GDB Commands* list above -- this is what it actually points
at: the floor's own tile color.)

*Why bank 0 and 1 are both already spoken for by the time RBG0 loads,
landing it on bank 2 (not 0):* `SRL::VDP2::Initialize()` (`srl_vdp2.hpp`,
runs during `SRL::Core::Initialize()`, well before `main()`'s own
`SetupSkyAndFloor()` call) points `SRL::ASCII`'s debug-text font at bank 0
via `ASCII::SetPalette(0)`, then legitimately claims that same bank through
the tracked allocator (`GetFreeBank()` + `SetBankUsedState()`) -- so bank 0
is correctly off-limits before this sample's own code ever runs.
`SetupSkyAndFloor()` separately reserves bank 1 up front too (its own
comment attributes this to the ASCII font as well, at its *default*
`colorBank = 1 << 12`/bank-1 encoding -- true only before that
`SetPalette(0)` call above overrides it to bank 0; harmless either way,
since reserving an already-unneeded bank just costs one spare slot, not a
collision). With 0 and 1 both unavailable, RBG0 gets the next free bank
(2), then NBG1 gets the one after that (3).

Re-run the `print` commands above after any change to this allocation
order (`SetupSkyAndFloor()`, `VDP2::Initialize()`, or anything else that
loads a Paletted16 tilemap before them) -- bank assignment is
allocation-order-dependent, not a promise these two addresses will always
hold. That's not a hypothetical caveat: it's exactly how the wrong
addresses in an earlier draft of this section got caught.

**2. Pick a color value.** CRAM colors here are `HighColor`
(`srl_color.hpp`), Saturn's native ABGR1555 format: bit 15 = opaque, bits
10-14 = blue (0-31), bits 5-9 = green (0-31), bits 0-4 = red (0-31) --
`value = 0x8000 | (blue << 10) | (green << 5) | red`. The floor's default
red (`HighColor(220, 30, 30)`, i.e. 220>>3=27 red, 30>>3=3 green, 30>>3=3
blue) packs to `0x8C7B`. Full-saturation blue (`red=0, green=0, blue=31`)
packs to `0x8000 | (31 << 10)` = `0xFC00`.

**3. Write it.**

```gdb
(gdb) set *(unsigned short*)0x25F00042 = 0xFC00
(gdb) x/1xh 0x25F00042
0x25f00042:     0xfc00
(gdb) continue
```

The floor tiles turn blue on the next VDP2 frame -- no reload of the
tilemap or a re-upload is needed, since the tiles already reference this
palette slot by index; only the color stored there changed. The same three
steps apply to the ceiling at `0x25F00062`, or to any other CRAM-backed
palette entry you locate via step 1's `print SRL::VDP2::<Screen>::TilePalette`
pattern (`NBG0`/`NBG2`/`NBG3` if your own project uses them).

> [!NOTE]
> This writes through the same `M`-packet path as `set variable`, so it
> needs the `hex2mem_aligned()` fix already covered above -- a single
> 16-bit-aligned write, not two separate byte writes, or the high byte can
> silently drop on real hardware. `set *(unsigned short*)addr = value` (as
> used here) already produces one aligned 2-byte write; avoid `set
> *(unsigned char*)addr = ...` twice in a row for the same color entry.

### Testing the Slave SH-2 (`SlaveCounterTask`) -- and why `InstallSlaveFreezeHandler()` isn't used here

The sample dispatches a small `SlaveCounterTask` (see `main.cxx`) onto the Slave SH-2 once per main-loop iteration via `SRL::Slave::ExecuteOnSlave()`, purely to demonstrate that API working -- it does, reliably, on real hardware (`Slave jobs done` climbs continuously for as long as the sample runs, matching the "Loop counter" field above it).

`srl_gdbstub.hpp` also ships `SRL::GDBStub::InstallSlaveFreezeHandler()` (and a convenience `InstallSlaveFreezeTask` to dispatch it), meant to freeze the Slave SH-2 for the duration of every master-side debug stop via the SH-2's on-chip FRT Input Capture Interrupt. Its own doc comment already warned this was "likely but unverified" to conflict with SGL's own `SRL::Slave::ExecuteOnSlave` (`slSlaveFunc`), which almost certainly uses the same FRT-ICI mechanism for its own slave job dispatch.

**This sample does not call `InstallSlaveFreezeHandler()`, because real-hardware testing confirmed the conflict is real and found no working combination:**

- **Redispatching a slave task every frame** (an earlier version of this sample) alongside the freeze handler: `SRL::GDBStub::g_slave_ici_count` -- which should tick up by exactly one per debug stop -- incremented only for the first stop or two after boot, then went permanently silent for every stop after that.
- **Dispatching a slave task a fixed number of times at startup only, then installing the freeze handler last and never touching `SRL::Slave` again**: this doesn't fix it either. On hardware, `g_slave_ici_count` read exactly `5` (matching the number of startup dispatches, not any debug stop) immediately after boot, then stayed at `5` across 8 further real `Ctrl-C`-triggered debug stops in the same session.

In both cases the counter tracks past `SRL::Slave` job-dispatch activity, not live freeze pulses -- once dispatch activity stops, the freeze mechanism goes dead and doesn't respond to further debug stops. The working theory: SGL's `slSlaveFunc` leaves the slave's own on-chip `TIER.ICIE` (interrupt enable) bit disabled once it has no more queued work. That bit lives in the slave's private on-chip peripheral space, which the master cannot write directly across the bus -- the only sanctioned way to run code on the slave that could re-arm it is `SRL::Slave::ExecuteOnSlave()` itself, which reopens the same conflict. No ordering or dispatch-count workaround was found.

**Bottom line:** if your project uses `SRL::Slave::ExecuteOnSlave()` at all, treat `InstallSlaveFreezeHandler()` as non-functional. It has not been tested in a program that never touches `SRL::Slave`.

**Breakpoints inside code that runs on the slave (e.g. `SlaveCounterTask::Do()`) are now supported**, via a second, independent handler: `SRL::GDBStub::InstallSlaveExceptionHandler()` (and its `InstallSlaveExceptionTask` convenience wrapper, dispatched once at startup in `main.cxx`). This is a completely different mechanism from `InstallSlaveFreezeHandler()` above and does **not** reopen that conflict -- the freeze handler's problem is specifically the FRT Input Capture Interrupt vector (0x64), which SGL's own `slSlaveFunc` dispatch also uses; this instead hooks the Illegal Instruction vector (4), which SGL's dispatch has no reason to touch. A software breakpoint (`Z0`/`break`) is just a `0xFFFF` memory patch and already works regardless of which CPU's code it lands in -- this handler is what lets the slave executing one report back to GDB instead of taking an unhandled exception on its own unconfigured boot-ROM default vector and hanging forever.

How it surfaces to GDB: the slave's thunk saves full context into `g_slave_ctx` (the same struct `monitor regs slave` already reads) and spin-waits on a shared resume flag. `Poll()`, which runs every VBlank on the master independent of whatever the master's own code is doing, notices this and calls the master's own `Break()` -- so a slave breakpoint shows up as a normal GDB stop (reported as SIGINT, the same as Ctrl-C/NMI), and `monitor regs slave` shows the slave's actual halted state. `continue` (or `step`, which doesn't single-step slave code but still releases it) resumes both CPUs together.

**Limitations of this mechanism** (unlike breakpoints in master code, which support the master's full step-over/reinsert machinery):
- **One-shot only:** a slave breakpoint is automatically removed the instant it's hit, so resuming doesn't immediately re-fault on the same patched instruction (there's no software single-step support for the slave to step past it otherwise). Set it again with a fresh `break`/`Z0` if you need it to fire again.
- **No real multi-thread RSP support:** GDB still only sees one CPU. A slave stop halts the whole session the same way Ctrl-C does; you inspect the slave's state via `monitor regs slave` / the slave pseudo-registers, not by switching GDB threads to a "slave thread."
- `main.cxx`'s per-frame `while (slaveTask.IsRunning())` wait is still bounded to a few million iterations as a second line of defense (e.g. if the exception handler was never installed, or GDB was never connected in the first place when a stray breakpoint fires).
- **Installing the handler itself is unreliable via the normal blocking dispatch pattern** -- hardware-confirmed: `InstallSlaveExceptionHandler()` completes correctly on the slave (verified via `g_slave_handlers_installed` reading `true`, and via the handler subsequently catching real breakpoints correctly -- see `InstallSlaveExceptionHandler()`'s own `@warning` for the full writeup), but `installExceptionTask.IsRunning()` was observed to never clear on the master side afterward. Root cause: the slave's VBR was found to already be relocated (~`0x06000400`, not `0`) by the time this runs -- presumably by SGL's own `slInitSystem()`/dual-CPU setup -- so the `if (vbr == 0)` guard (written assuming a boot-ROM-default VBR, same as the master's first run) is skipped, and the illegal-instruction vector gets patched directly into that already-live table instead of a fresh copy. That appears to be what disrupts `slSlaveFunc`'s own dispatch-completion signal back to the master. `main.cxx` bounds this specific wait too (`installWait`), for the same reason as the per-frame wait above -- an unconditional wait here would hang `main()` before it ever reaches its loop at all.
- **Fixed, but worth knowing about**: any RSP exit path from `process_commands()` other than `continue`/`step` (`D` detach, `k` kill, a plain disconnect -- including GDB's own implicit detach at the end of a `-batch` session, which is how this project's own testing scripts often connect) used to leave a slave parked at a breakpoint permanently stuck: the master resumes fine regardless (so `Poll()`/the main loop look completely healthy), but `SlaveCounterTask::Do()` never actually returns, so no further slave jobs are ever dispatched again. `process_commands()` now has a `SlaveReleaseGuard` RAII member that releases a pending slave stop on every exit path, not just `handle_gdb_continue()`/`handle_gdb_step()`. If a slave breakpoint you set once stops firing on later hits, this is very likely why -- power-cycle to confirm, or check `SRL::GDBStub::GetSlaveBreakpointCount()` stays frozen despite further `continue`s.

### Slave pseudo-registers -- known limitation

`srl_gdbstub.hpp` advertises slave-CPU register state as named pseudo-registers (`$slave_pc`, `$slave_r0`, ...) via `qXfer:features:read`. In practice, **neither `gdb-multiarch` nor `sh-elf-gdb` honor this for the SH architecture** -- both hard-code a fixed register layout and print `warning: Target-supplied registers are not supported by the current architecture`, ignoring the extra names entirely. This isn't fixable from the target side; it's a limitation of GDB's (largely unmaintained) SH port. `monitor regs slave` is the practical, cross-client way to see this state instead.

VDP1/VDP2 hardware registers are **not** exposed by the stub at all (no pseudo-registers, no `monitor` command) -- **ordinary memory examination** at the real hardware address is the only way to inspect them, e.g.:
```gdb
(gdb) x/1xh 0x25F80020    # VDP2 BGON
(gdb) x/1xh 0x25D00000    # VDP1 TVMR
```

### Reset button (NMI) landing inside a VDP1 busy-wait -- known limitation, requires power cycle

The Reset button reaches the SH-2 as NMI (see `srl_gdbstub_nmi_thunk`'s doc comment in `srl_gdbstub.hpp`), which this stub reports to GDB as a normal SIGINT stop -- `continue` then resumes exactly where Reset interrupted, just like Ctrl-C. This works reliably for code stopped in ordinary (non-spinning) execution, hardware-confirmed via repeated continue/interrupt/continue cycles.

There is one landing spot where it does not recover: a tiny busy-wait inside SGL's precompiled VDP1 command-submission path (symbolized as `_slInitSprite+0x2a2` in this build, though it is actually a shared low-level "wait for VDP1 command register to clear its busy bit" helper, not `_slInitSprite` itself):

```
602e592: mov  #-112, r3
602e594: mov.l @(12,r3), r0   ; read VDP1 status at 0xFFFFFF9C
602e596: and  #3, r0
602e598: cmp/eq #1, r0
602e59a: bt   602e594          ; spin while busy bit == 1
```

Because this sample submits a VDP1 polygon every frame (`DrawRasterbar()` in `main.cxx`), and that MMIO read carries real wait-states, this spin loop dominates the CPU's per-frame idle time -- making it, empirically, the single most likely place for an asynchronously-timed NMI to land (hardware-confirmed: 4 out of 4 real Reset presses during normal (post-boot, main-loop-running) operation landed on this exact instruction). Resuming from it -- even via a read-only GDB inspection, since this stub's `D` (detach) handler resumes the target the same way `continue` does -- leaves the busy bit permanently set; the target stops responding to the USB link entirely (confirmed via raw wire capture: zero bytes back from the target, not even a packet ACK) and needs a full power cycle to recover. There is no software recovery from this specific case: once resumed, nothing before the exception thunk can fix VDP1's desynced command handshake, and nothing after it (a fresh Poll()) ever runs again.

**Practical implication:** if the screen freezes after a Reset-button press and stays frozen (loop counter no longer advancing, GDB reconnects time out with no target reply), don't keep retrying `continue` -- power-cycle the console and re-upload. This is specific to samples/projects that busy-wait on VDP1 every frame; it is not a bug in the NMI/breakpoint wiring itself, which is otherwise fully recoverable.

### Unplugging the USB cable mid-session hangs the console -- known hardware limitation, requires power cycle

`srl_gdbstub.hpp` guards every *software* polling loop that touches the USB DevCart FIFO (`Poll()`'s Ctrl-C read, the stale-byte drain in `process_commands()`, and `__gdb_wait_rx()`/`__gdb_wait_tx()`) with an `SRL::DevCart::CS0::IsConnected()` check, so none of those loops spin forever on a disconnect -- this is confirmed working for those specific paths.

**Unplugging the physical USB cable while the console is running still hangs it, and hardware-confirmed evidence points to this being unfixable from that C++ layer**: the freeze happens on the *very next instruction cycle* after the cable is pulled, with no intervening frame -- the on-screen "Loop counter" and other fields do not get one more chance to update before freezing (ruling out a software polling loop, which would still be executing right up until it hit that specific loop). This points to the SH-2's Bus State Controller stalling on an external `WAIT` signal from the cartridge's CPLD/FTDI interface (used to time the slower USB FIFO's bus cycles) that never arrives once the FTDI chip loses its USB-side power/reference -- a stall at the CPU's bus-cycle level, before any instruction (including an `IsConnected()` check) gets a chance to execute. No amount of guarding `SRL::DevCart::CS0` call sites in this header can fix a hang that happens *inside* the memory access itself.

The only theoretical fix would be reconfiguring the SH-2's Bus State Controller for the cartridge's CS0 address range to stop waiting on that external signal (a fixed internal wait-state count instead) -- but that's a system-wide bus-timing change, not a local one, and getting the wait-cycle count wrong risks corrupting *normal* (connected) DevCart transfers, which likely genuinely need those wait states. This has not been attempted; it would need real hardware validation of the cart's actual timing requirements to do safely.

**Practical implication:** don't unplug the USB cable while debugging or running this sample -- if you do, power-cycle the console (a soft reset via the Reset button will not help; see the NMI section above) and re-upload. This is a hardware/bus-timing limitation of the DevCart interface itself, not a bug in the GDB stub's connection-tracking logic.
