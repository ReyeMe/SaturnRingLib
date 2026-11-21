# FLD - SEGA file loader kernel

FLD is the loader of SEGA's "Demo-Demo System" (DDS) discs: a menu program starts other games stored in
sub-directories of the same disc. SRL wraps it in `saturnringlib/srl_fld.hpp` (`SRL::Fld`); a project that calls
`Fld::ExecGame` sets `SRL_USE_FLD = 1`, which links the kernel into the program (`fld_kernel.S`).

## Files

| File | What it is |
|---|---|
| `fld_knl.bin` | The kernel rebuilt from `src/fld_knl.c` (1700 bytes). Used by default (`SRL_FLD_KERNEL`); not committed, `shared.mk` builds it when a project with `SRL_USE_FLD = 1` needs it. |
| `src/fld_knl.c`, `fld_knl.ld`, `makefile` | Its source, linker script (image at `0x00200000`, entry pointer first) and build: `make` in this directory with the sh-elf toolchain. |
| `fld_kernel.S` | Puts the kernel selected by `SRL_FLD_KERNEL` in the program as `FldKernel` / `FldKernelEnd`. |

## How a game starts

1. The menu (`Fld::ExecGame(ip, dir, track)`, SEGA's `FLD_ExecGame`):
   - loads the game IP file to `0x06002000`, then copies the BIOS system ID (`0x06000C00`, 0x100 bytes) over its
     header and the BIOS boot info (`0x060002A0`, 0x20 bytes) to `0x060020E0`;
   - writes the DDS block: `0x06000CCC` = 1, `0x06000CCD` = first CD-DA track, `0x06000CCE` (16 bits) = GFS id of `dir`;
   - stops the slave CPU and every interrupt source the menu used (SCU mask, SH-2 on-chip priorities);
   - copies the kernel to `0x00200000` and sets the IP's loader vector (`0x06002270`) to it;
   - jumps to the IP code at `0x06002100`.
2. The IP code (SEGA logo/security sequence) calls the loader through `[[0x06002270]]` = `[0x00200000]`, the kernel
   entry (`FLD_1stRead`, `0x002001A4` in 1.21).
3. The kernel reads the first file of the DDS directory (`0.BIN`) to the 1st read address and returns; on error it
   starts the BIOS CD player (`[0x0600026C]`). The BIOS loader cannot be used instead: its work area
   (`0x06020000`) is inside the menu program.
4. The IP code finishes its logo and jumps to the game. The game's GFS (SGL `GFS.O`) reads the DDS block, so `dir`
   is its root directory; `SRL::Sound::Cdda` moves track numbers so the game's track 2 is the given track.
5. `Fld::ReturnToMenu()` (`SYS_Exit(0)`) makes the BIOS check and boot the disc again, which starts the menu. The
   BIOS rewrites the DDS block and clears work RAM on the way.

## What the kernel does

SEGA's kernel is FLD plus a cut-down copy of the CDC library (CD block commands); about half of that library code
is never called. `src/fld_knl.c` keeps the code that runs, with the same CD block commands in the same order, the
same timeouts (loop counts) and the same error codes; each function names the address of the original one.

1. Installs a VBlank-in handler (BIOS `SYS_SETUINT`, `0x06000300`) that calls the IP's own handler (`0x060022E4`)
   and keeps the IP logo counter (`0x06002D80`) from going negative.
2. Soft-resets the CD block, goes back to the root directory, enters the DDS directory (`0x06000CCE`) and starts
   reading its file id 2 (the first file after `.` and `..`).
3. Moves the file to the 1st read address the BIOS took from the IP (`0x060002B0`); the file must fit in
   `0x06003000`-`0x06100000` or `0x00200000`-`0x00300000`.
4. On any error, jumps to the BIOS CD player (`[0x0600026C]`).

Its only variable (HIRQ bits seen by the commands) is part of the binary, right after the code.

| Address | Use |
|---|---|
| `0x00200000` | the kernel (word 0: entry pointer) |
| `0x0600026C` | BIOS CD player vector, used on error |
| `0x060002B0` | 1st read address the BIOS took from the IP |
| `0x06000300` | BIOS `SYS_SETUINT` |
| `0x06000CCE` | DDS directory id |
| `0x060022E4`, `0x06002D80` | VBlank-in handler and logo counter of SEGA's IP security code |
| `0x25890008` - `0x25898000` | CD block registers (HIRQ, CR1-CR4, data) and `0x25818000` (32-bit data port) |

