# GDB Stub Sample

This sample demonstrates how to use the Sega Saturn GDB stub (`srl_gdbstub.hpp`) to debug your game running on real hardware.

> [!WARNING]
> **Real Hardware Required:** This sample utilizes the USB interface on a DevCart with USB support. It **cannot** be run in emulators like Mednafen, Kronos, or Yabause, because they do not emulate this custom USB hardware. Attempting to run this in an emulator will likely cause it to hang or crash.

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
- `set variable <var> = <value>`: Write memory. Try `set variable g_testVariable = 99` then `print g_testVariable`.
- `Ctrl-C`: Pause the running game (requires the game to periodically call `SRL::GDBStub::Poll()`, which this sample does via `SRL::Core::Synchronize()` every frame).
- `x/10xh 0x25F80000`: Examine memory directly -- e.g. VDP2 registers (see caveat below).
- `monitor <text>`: Sends the text to the target via `qRcmd`, decoded and dispatched by this sample's `HandleMonitorCommand()`. Lets you trigger any of the test paths below without a gamepad -- handy for scripted/headless testing over a raw `ftx -g` connection:
  - `monitor crash illegal|addr|reserved|slotillegal|slotreserved|genillegal|dma|ubc|trapa3` -- same as the B/A/C/X/Y/Z/L/START buttons.
  - `monitor step` -- same as D-Pad Up (`SteppableFunction()`).
  - `monitor touch` -- same as D-Pad Down (increments `g_testVariable`, useful with `watch`).
  - The dispatch only happens once the target resumes (`continue`/detach), and only the *last* `monitor` command sent while stopped takes effect -- send one, `continue`, then repeat, rather than queuing several while paused.

### Testing the Slave SH-2 (`SlaveCounterTask`) -- and why `InstallSlaveFreezeHandler()` isn't used here

The sample dispatches a small `SlaveCounterTask` (see `main.cxx`) onto the Slave SH-2 five times at startup via `SRL::Slave::ExecuteOnSlave()`, purely to demonstrate that API working -- it does, reliably, on real hardware (`Slave jobs done` reaches `5` and stays there).

`srl_gdbstub.hpp` also ships `SRL::GDBStub::InstallSlaveFreezeHandler()` (and a convenience `InstallSlaveFreezeTask` to dispatch it), meant to freeze the Slave SH-2 for the duration of every master-side debug stop via the SH-2's on-chip FRT Input Capture Interrupt. Its own doc comment already warned this was "likely but unverified" to conflict with SGL's own `SRL::Slave::ExecuteOnSlave` (`slSlaveFunc`), which almost certainly uses the same FRT-ICI mechanism for its own slave job dispatch.

**This sample does not call `InstallSlaveFreezeHandler()`, because real-hardware testing confirmed the conflict is real and found no working combination:**

- **Redispatching a slave task every frame** (an earlier version of this sample) alongside the freeze handler: `SRL::GDBStub::g_slave_ici_count` -- which should tick up by exactly one per debug stop -- incremented only for the first stop or two after boot, then went permanently silent for every stop after that.
- **Dispatching a slave task a fixed number of times at startup only, then installing the freeze handler last and never touching `SRL::Slave` again**: this doesn't fix it either. On hardware, `g_slave_ici_count` read exactly `5` (matching the number of startup dispatches, not any debug stop) immediately after boot, then stayed at `5` across 8 further real `Ctrl-C`-triggered debug stops in the same session.

In both cases the counter tracks past `SRL::Slave` job-dispatch activity, not live freeze pulses -- once dispatch activity stops, the freeze mechanism goes dead and doesn't respond to further debug stops. The working theory: SGL's `slSlaveFunc` leaves the slave's own on-chip `TIER.ICIE` (interrupt enable) bit disabled once it has no more queued work. That bit lives in the slave's private on-chip peripheral space, which the master cannot write directly across the bus -- the only sanctioned way to run code on the slave that could re-arm it is `SRL::Slave::ExecuteOnSlave()` itself, which reopens the same conflict. No ordering or dispatch-count workaround was found.

**Bottom line:** if your project uses `SRL::Slave::ExecuteOnSlave()` at all, treat `InstallSlaveFreezeHandler()` as non-functional. It has not been tested in a program that never touches `SRL::Slave`.

### Pseudo-registers (VDP1/VDP2/slave SH-2) -- known limitation

`srl_gdbstub.hpp` advertises VDP1, VDP2, and slave-CPU register state as named pseudo-registers (`$vdp2_bgon`, `$slave_pc`, ...) via `qXfer:features:read`. In practice, **neither `gdb-multiarch` nor `sh-elf-gdb` honor this for the SH architecture** -- both hard-code a fixed register layout and print `warning: Target-supplied registers are not supported by the current architecture`, ignoring the extra names entirely. This isn't fixable from the target side; it's a limitation of GDB's (largely unmaintained) SH port.

The reliable, cross-client way to inspect this state is **ordinary memory examination** at the real hardware address, e.g.:
```gdb
(gdb) x/1xh 0x25F80020    # VDP2 BGON
(gdb) x/1xh 0x25D00000    # VDP1 TVMR
```
See the `ExtraRegs[]` array in `srl_gdbstub.hpp` for the full address list.
