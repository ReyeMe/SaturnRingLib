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

Once successfully uploaded, the code will boot, initialize the GDB stub, and execute a `trapa #3` instruction. This will immediately halt the Saturn's CPU and wait for GDB to connect.

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
(gdb) set architecture sh
(gdb) set endian big
(gdb) target extended-remote host.docker.internal:1234
```

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

and GDB will show:

```gdb
...
Connected to Saturn.
0x60044444 in main () at src/main.cxx:15
15		SRL::GDBStub::Poll();
(gdb) 
```

### Useful GDB Commands:
- `info registers`: View the state of all SH-2 CPU registers.
- `break <file>:<line>`: Set a software breakpoint.
- `continue` or `c`: Resume game execution.
- `Ctrl-C`: Pause the running game (requires the game to periodically call `SRL::GDBStub::Poll()`, which this sample does in its main loop).
- `x/10x $pc`: Read 10 words of memory at the Program Counter.

### Unsupported Operations (IMPORTANT):
The SH-2 CPU does not have a hardware single-step feature, and the `SaturnRingLib` stub does not emulate it via software instruction decoding. 
Because of this, the following commands are **unsupported and will fail**:
- `step` (Step Into)
- `next` (Step Over)
- `finish` (Step Out)

If you use these commands in GDB (or the corresponding buttons in VS Code), GDB will receive an error reply (`E01`). **You must use breakpoints and `continue` to navigate your code.**
