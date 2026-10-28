# GDB Stub Sample

Debug a program running on a real Saturn with GDB, using the SRL GDB stub (`srl_gdbstub.hpp`).

> [!WARNING]
> **Real hardware only.** The stub talks to GDB over the USB port of a DevCart. Emulators (Mednafen, Kronos, Yabause) don't emulate it, so the sample hangs there.

## What's in the sample

| File | What it shows |
|---|---|
| `src/main.cxx` | The main loop: status text, gamepad and `monitor` triggers, slave jobs |
| `src/debug_triggers.cxx` | One function per SH-2 exception, a function to step through, a variable to watch |
| `src/slave_counter_task.cxx` | A job that runs on the slave SH-2, for slave breakpoints |
| `src/vdp_demo.cxx` | Moving backgrounds, so the screen isn't static (not needed by the stub) |

The code is split across files on purpose: breakpoints and stepping must work across translation units.

The stub needs no setup call. `SRL::Core::Synchronize()` calls `SRL::GDBStub::Poll()` every frame, and that is when GDB can connect or break in.

## 1. Install the tools

- **gdb-multiarch** (or `sh-elf-gdb`): a GDB that knows the SH-2.
  - Windows: `tools/scripts/getcompiler.ps1` installs it to `tools/bin/win/gdb-multiarch.exe`.
  - macOS (Apple Silicon): `tools/scripts/getcompiler.sh` runs `brew install` for it (it builds from source, which takes a while).
  - Linux: use your package manager, e.g. `apt install gdb-multiarch`.
- **[ftx](https://github.com/willll/ftx) v1.2 or later**: uploads the program and relays GDB to the DevCart (`setup_compiler.bat` installs it). Older versions are about 13x slower.
- **usbreset** (optional): resets the FT245R USB chip. On Windows, `getftx.ps1` installs an equivalent, which asks for Administrator rights once.

## 2. Build

```bash
# From Samples/Debug - GDB Stub/
../../tools/scripts/make.sh
```

The makefile already builds with `-g -O0`. You get `BuildDrop/Debug_GDBStub.elf` (symbols, for GDB) and `BuildDrop/Debug_GDBStub.bin` (the program).

## 3. Run on the Saturn

1. Power-cycle the Saturn and wait about 10 seconds.
2. Reset the USB chip: `usbreset "FT245R USB FIFO"`
3. Check the link: `ftx -c`. If it says `device not found`, start again from step 1.
4. Upload and run: `../../tools/scripts/run.sh USBGamers`

If the upload fails with `usb bulk write failed`, repeat the whole sequence.

The program doesn't wait for GDB. It runs straight away, and you can connect at any time.

## 4. Connect GDB

Start the ftx proxy in one terminal, then GDB in another:

```bash
ftx -g 1234 -v
```

```bash
gdb-multiarch ./BuildDrop/Debug_GDBStub.elf
(gdb) set endian big
(gdb) target remote localhost:1234
(gdb) mem 0x06000000 0x06100000 rw cache
(gdb) mem 0x00200000 0x00300000 rw cache
```

- Use `target remote`, not `target extended-remote`; the stub doesn't support the extended packets.
- The two `mem` lines let GDB read RAM in blocks instead of one value per packet (`x/4096xw` takes about 1 s instead of 12 s). The cache is dropped on every resume, so values never go stale. Don't cache hardware registers.
- From inside Docker, connect to `host.docker.internal:1234`.

GDB first stops inside the stub (`SRL::GDBStub::snapshot_polling_context()`). That is expected: `Poll()` caught the program between frames. Set a breakpoint and continue, for example `break main.cxx:95` (runs every frame) or `break SteppableFunction`.

### VS Code

The `.vscode` folder has two launch configurations (C/C++ extension `ms-vscode.cpptools` required):

- **Debug - GDB Stub (Debug - GDB Stub)**: connects to the program already running on the Saturn.
- **Debug - GDB Stub (Upload + Debug)**: uploads first, then connects.

Both start `ftx -g 1234` if it isn't running, and run the same `mem` commands. Press F5, then use the gutter for breakpoints and F10/F11/Shift+F11 to step. To type a GDB command, use the Debug Console with `-exec`, e.g. `-exec monitor step`. If something won't connect, try the command line first: it shows the error messages VS Code hides.

`tasks.json` also has `Compile [DEBUG]`, `Run on Saturn`, `Upload to Saturn` and `Connect to GDB`.

## 5. Things to try

| Goal | Do this |
|---|---|
| Single-step | `break SteppableFunction`, press D-Pad Up, then `step` / `next` |
| Watch a variable | `watch g_testVariable`, `continue`, press D-Pad Down |
| Write memory | `set variable g_testVariable = 99`, then `print g_testVariable` |
| Pause the program | `Ctrl-C` |
| Read hardware registers | `x/1xh 0x25F80020` (VDP2 BGON), `x/1xh 0x25D00000` (VDP1 TVMR) |
| Catch an exception | Press a button from the on-screen list, or use `monitor crash ...` |
| Stop on the slave CPU | `break SlaveCounterTask::Do`, `continue` |

Notes:
- **Stepping**: the SH-2 has no hardware single-step. The stub decodes the current instruction (including branches and delay slots) and puts a temporary trap on the next one.
- **Hardware watchpoints** use the SH-2's User Break Controller. It has two channels, so you can have two `watch`/`rwatch`/`awatch`/`hbreak` at once. The watched range must be 1, 2, 4, 8... bytes long and aligned to that size.
- **Memory writes** use full 16/32-bit bus writes where possible. VDP RAM needs that: it drops single-byte writes.

### Monitor commands

`monitor <text>` (31 characters max) runs a trigger without a gamepad, which is useful for scripts:

| Command | Same as | Runs |
|---|---|---|
| `monitor crash illegal` | B | `CrashProgram()` (vector 4) |
| `monitor crash addr` | A | `CPUAddressError()` (vector 9, doesn't fault on the test hardware) |
| `monitor crash reserved` | C | `ReservedInstruction()` (vector 5) |
| `monitor crash slotillegal` | X | `SlotIllegalInstruction()` (vector 6) |
| `monitor crash slotreserved` | Y | `SlotReservedInstruction()` (vector 8) |
| `monitor crash genillegal` | Z | `GeneralIllegalInstruction()` (vector 7) |
| `monitor crash dma` | L | `DMAAddressError()` (vector 10) |
| `monitor crash ubc` | R | `UserBreakController()` (vector 12) |
| `monitor crash trapa35` | START | `Trapa35()` (vector 35) |
| `monitor step` | D-Pad Up | `SteppableFunction()` |
| `monitor touch` | D-Pad Down | `g_testVariable++` |
| `monitor stepbranches` | | `StepBranches()`, every SH-2 branch kind, for `stepi` |
| `monitor regs slave` | | Prints the slave CPU's registers |
| `monitor trace` | | Prints the state saved when the program stopped |

The trigger runs when the program resumes (`continue`). Only the last command sent while stopped runs, so send one, `continue`, then send the next.

## 6. Tutorial: change a tile's color

The floor (RBG0) and ceiling (NBG1) are tiles whose pixels are indexes into a palette in VDP2 color RAM (CRAM). Change the palette entry and every tile using it changes color on the next frame, with nothing reloaded.

With no CD mounted (the normal `ftx` run), the textures are the built-in checkerboards: index 0 is transparent, index 1 is the color you see.

**1. Find the palette.** Its address is chosen at run time, so ask the target:

```gdb
(gdb) print SRL::VDP2::RBG0::TilePalette
$1 = {data = 0x25f00040 <...>, paletteMode = Paletted16, id = 2}
```

`data` points at index 0. Each color is 2 bytes, so index 1 is at `data + 2`:

| Screen | Tile color address |
|---|---|
| RBG0 (floor) | `0x25F00042` |
| NBG1 (ceiling) | `0x25F00062` |

These hold for this build. CRAM bank 0 holds the debug font, and `SetupSkyAndFloor()` leaves bank 1 unused, so the floor gets bank 2 and the ceiling bank 3. Print again if you change what loads before them.

**2. Pick a color.** Colors are 16-bit: `0x8000 | (blue << 10) | (green << 5) | red`, each channel 0-31. Pure blue is `0xFC00`.

**3. Write it** as one 16-bit value:

```gdb
(gdb) set *(unsigned short*)0x25F00042 = 0xFC00
(gdb) continue
```

The floor turns blue. Don't write a color as two single bytes: CRAM drops byte writes.

## 7. Debugging the slave SH-2

The main loop gives `SlaveCounterTask` to the slave every 15 frames with `SRL::Slave::ExecuteOnSlave()`, and never waits for it. `Slave jobs` on screen counts the finished jobs.

At startup, `main()` runs `SRL::GDBStub::InstallSlaveExceptionTask` on the slave. A breakpoint hit by the slave then stops the slave and reports it to GDB as **thread 2** (thread 1 is the master):

- GDB shows the slave's source line and backtrace; `info threads` lists both CPUs, and `thread 1` goes back to the master.
- `step`/`next`/`stepi` on thread 2 step the slave while the master stays stopped.
- `continue` always resumes both CPUs, even with `set scheduler-locking on`.
- Thread 2 exists only while the slave is stopped at a breakpoint.
- A slave breakpoint is removed when hit. GDB's `break` puts it back on `continue`; a raw `Z0` packet sent by hand must be sent again.
- The slave keeps running while only the master is stopped.

GDB's SH port has no named slave registers. Switch to thread 2, or use `monitor regs slave`.

## Known limitations

- **Don't unplug the USB cable while the program runs.** The CPU freezes on its next access to the cartridge, and only a power cycle helps (not the Reset button).
- **`monitor crash addr` doesn't fault** on the test hardware; the misaligned read just runs.
- **`monitor crash ubc` takes UBC channel A** directly, replacing any GDB watchpoint or `hbreak` that uses channel A.
