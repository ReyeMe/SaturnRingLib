# Slave-side GDB breakpoint investigation

Findings from investigating why software breakpoints on code that runs on
the Slave SH-2 (via `SRL::Slave::ExecuteOnSlave()`) were unreliable. Two
distinct bugs were found. One is fixed. One is confirmed, reproducible, and
**not fixed** — this document exists so whoever picks it up next doesn't
have to re-derive the same evidence.

Related code: `srl_gdbstub.hpp` (`install_software_breakpoint()`,
`InstallSlaveExceptionHandler()`, `slave_breakpoint_handler()`), `srl_slave.hpp`
(`SRL::Slave::SlaveTask()`), `Samples/Debug - GDB Stub/src/slave_counter_task.hpp`
(its own `@warning` about the unrelated `InstallSlaveFreezeHandler()` conflict,
which turned out to be relevant background for bug #2).

---

## Bug #1: stale slave instruction cache (FIXED)

**Symptom:** a software breakpoint set on slave-executed code (e.g. `break
SlaveCounterTask::Do`) would silently never fire — no error from gdb, the
`Z0` packet returns `OK`, the memory patch genuinely is written, but the
slave keeps running as if nothing happened.

**Root cause:** `install_software_breakpoint()` patches the `0xFFFF`
breakpoint opcode into shared RAM and then purges the *master's own*
instruction cache. The SH-2 Cache Control Register (`0xFFFFFE92`) is private
on-chip hardware per CPU — there is no bus path for the master to purge the
slave's cache. If the slave has already cached the line containing the
patched instruction (which, for a repeatedly-dispatched task, it almost
always has), it keeps executing its own stale, unpatched copy indefinitely.

**Evidence:** read `srl_gdbstub_slave_bp_count` (the breakpoint hit counter)
via raw memory before and after a 15-second `continue` with a fresh slave
breakpoint installed, on a task known to be dispatched roughly every 0.25s.
The counter never incremented — the illegal instruction genuinely never
executed, not merely "not yet reported."

**Fix:** `srl_slave.hpp`'s `SRL::Slave::SlaveTask()` — the universal wrapper
every `SRL::Slave::ExecuteOnSlave()` dispatch passes through — now purges the
slave's own cache inline, at the top, before running any task. This needs no
cross-CPU dispatch (it's already running on the slave) and is reached
exactly once per task, always before that task's own code executes.

**A fix attempt that didn't work, for the record:** the first attempt fixed
this from `srl_gdbstub.hpp` instead, by dispatching a dedicated cache-purge
task via `SRL::Slave::ExecuteOnSlave()` at breakpoint-install time. Two
problems: (a) it briefly called `ExecuteOnSlave()` from *inside* code already
running on the slave (`remove_software_breakpoint()`'s call from
`slave_breakpoint_handler()`), which is undefined territory; (b) even after
fixing that, the dispatch only reliably reached the slave when it happened
to be idle at that exact instant — a purge task dispatched while the slave
was mid-execution of a different task collided with it and silently never
ran, confirmed via a diagnostic counter that stayed flat across "failed"
attempts. The `SlaveTask()`-level fix has no such race, since it doesn't
need a separate dispatch at all.

**Verification:** confirmed reliable across multiple fresh-boot trials —
the first slave breakpoint hit of a boot session now fires consistently.

---

## Bug #2: dispatch permanently stops after one breakpoint hit (CONFIRMED, NOT FIXED)

**Symptom:** even with bug #1 fixed, a slave breakpoint fires exactly
**once per boot**. After it's released (one-shot removal, master resumes,
slave resumes), `SRL::Slave::ExecuteOnSlave()` never dispatches *any* task
again for the rest of the boot session. Only a power-cycle recovers it.

**What's confirmed, in order of investigation:**

1. **The master-side release handshake is correct.** `g_slave_stopped` /
   `g_slave_resume` read exactly as expected after a hit and release
   (`false` / `true`), whether release happens via `continue`, `step`, or
   the `SlaveReleaseGuard` RAII fallback on other exit paths (`D`, `k`,
   `packet_get()` failure).

2. **This is not a `Do()`-specific hang.** Instrumented `Do()` itself
   (entry/loop-done/return counters) and separately instrumented
   `SlaveTask()` (the dispatch wrapper, before `Do()` is even reached) —
   both freeze completely after a hit. The dispatch *pipeline* stops, not
   just one function's completion.

3. **The SH-2 exception thunk is not obviously broken.** The slave's
   illegal-instruction thunk (`srl_gdbstub_slave_illegal_thunk`) is
   byte-for-byte structurally identical to the master's own thunk
   (`srl_gdbstub_exception_thunk`), which works fine for repeated master-side
   breakpoints. Register save/restore, including the PC/SR round-trip
   through the stack before `rte`, is symmetric between the two.

4. **A targeted fix attempt — re-arming `TIER.ICIE` from the resume path,
   mirroring `slave_ipi_handler()`'s own re-enable — did not help.**
   (`slave_ipi_handler()` is the *freeze handler*'s ICI thunk, a completely
   separate, unused-by-default mechanism — see below for why this was worth
   trying anyway.)

5. **Reverse-engineered `SlaveSHMain`** (the function `InitSlaveSH` installs
   as the slave's permanent boot entry point, from disassembling SGL's
   precompiled `LIBSGL.A` — no source available) via `sh-elf-objdump -b
   coff-sh`: it sets `SR = 0xFFFFFFF0` (**all interrupts permanently
   masked**) and busy-polls the FRT's `FTCSR.ICF` flag directly — it does
   **not** use a real CPU interrupt for dispatch at all. This is why attempt
   #4 didn't help: `TIER.ICIE` genuinely isn't part of this path.

6. **A second hypothesis, from reading `SlaveSHMain`'s loop closely, turned
   out not to apply.** After `jsr`-calling the dispatched function, the loop
   does `mov.l r0,@r2` — storing the callee's return register back into
   `*_SlaveCommand`. Since the dispatched function is `void`, R0 at return
   is compiler-dependent; a breakpoint-interrupted resume path leaving R0=0
   would null out `_SlaveCommand`, and the loop's own null-check would then
   silently skip every future dispatch — a clean match for the symptom.
   **But**: single-stepping a real `slSlaveFunc` call live (`break
   *<slSlaveFunc address>`, then `stepi` through it while dumping registers
   and memory) showed `_SlaveCommand`'s true linked address (`0x060286d8`)
   does not match where `slSlaveFunc` actually writes (`0x260ea000`-based,
   advancing by 12 bytes per call, no wraparound observed). These are two
   *unrelated* memory regions. `SlaveSHMain`'s fixed-address read and
   `slSlaveFunc`'s advancing-queue write don't appear to be the same
   mechanism, which means `SlaveSHMain` is likely not even the function
   actually driving normal dispatch, and this hypothesis doesn't hold up as
   stated.

7. **A confirmed hardware-level collision, independent of the above:**
   `slSlaveFunc`'s doorbell write and `srl_gdbstub`'s own
   `MasterNotifiesSlave` constant are the *exact same address*
   (`0x21000000`). Not "the same vector" — the literal same memory-mapped
   trigger address. This is concrete confirmation of what
   `slave_counter_task.hpp`'s own `@warning` already suspected in general
   terms for the (unrelated, unused-by-default) freeze handler.

**Where the investigation stopped:** finding the actual consumer of the
`0x260ea000`-based advancing queue (there's a candidate — `SlaveControl`, a
3D polygon-pipeline command processor that also reads from `ComWrPtr` /
`(72,gbr)` — but its jump table only dispatches fixed SGL rendering
primitives, not arbitrary callbacks, so it doesn't cleanly fit either) would
need searching further through the ~280 remaining object files in
`LIBSGL.A` and/or tracing `slInitSystem()`'s full boot sequence. This is
reverse-engineering unlabeled 1996-era machine code with no source or
documentation available — a substantially larger undertaking with no
guaranteed answer, so it was set aside rather than pursued further without
new information to justify the cost.

**Practical effect today:** documented at `InstallSlaveExceptionHandler()`'s
doc comment in `srl_gdbstub.hpp` (the fullest writeup) and referenced from
`slave_breakpoint_handler()`. `SRL::Slave::ExecuteOnSlave()` callers that
gate on `!task.IsRunning()` before redispatching (the documented, correct
usage pattern) never dispatch that task again once a slave breakpoint has
fired once, for the rest of the boot session.

---

## SGL reverse-engineering reference notes

Captured here since they took real effort to extract and may be useful
beyond this specific bug. All addresses are from disassembling
`modules/sgl/LIB/LIBSGL.A`'s COFF object members directly:

```bash
# Correct target format is required -- plain `nm`/`objdump` misidentify
# these as "ambiguous" (coff-sh vs coff-sh-small) and refuse to load:
sh-elf-ar x LIBSGL.A                          # extract members
sh-elf-nm --target=coff-sh <member>.o         # symbols
sh-elf-objdump -b coff-sh -d -r <member>.o    # disassembly
```

| Symbol | Object file | Role |
|---|---|---|
| `_slSlaveFunc` | `sglA15.o` | Public dispatch entry point (what `SRL::Slave::ExecuteOnSlave()` calls). Writes a 3-word `{state=0x30, func, param}` record through an *advancing* pointer stored at `*(GBR+72)`, then pulses the doorbell. |
| `_InitSlaveSH` | `sglJ00.o` | One-time slave boot setup. Writes `&SlaveSHMain`'s address to a fixed location (`0x06000250`) the slave's reset vector reads, then releases the slave from reset via `slRequestCommand`. |
| `_SlaveSHMain` | `sglJ00.o` | Installed as the slave's permanent entry point. Masks all interrupts (`SR=0xFFFFFFF0`) and busy-polls `FTCSR.ICF` forever; on a pulse, reads a function pointer from the **fixed** address `_SlaveCommand`, calls it, stores the return value back into `*_SlaveCommand`. Does not appear to consume the advancing queue `slSlaveFunc` writes to (see bug #2, point 6). |
| `_SlaveControl` | `sglJ01.o` | A *different* consumer that does read from the advancing queue (`ComWrPtr`, i.e. `(72,gbr)`) — but its jump table (`comjmp_tbl`) only dispatches fixed 3D primitives (`sbMakeDMATable`, `sbCalcPolygon`, `sbMakeRotSprite`, `sbMakeSprite`, `sbMakePolygon`, `sbSortEntry`), not arbitrary callbacks. Uses the identical `FTCSR.ICF` busy-poll idiom while waiting. |
| `_slCheckSlave` | `sglI06.o` | Not investigated further. |

Key addresses/constants:
- `0x21000000` — the master-to-slave doorbell: a 16-bit write here pulses
  the slave's FRT input-capture pin. **Same address** as
  `srl_gdbstub.hpp`'s own `MasterNotifiesSlave`.
- `0xFFFFFE10` / `0xFFFFFE11` — `FRT_TIER` / `FRT_FTCSR`, bit `0x80` =
  `FRT_ICF` (input-capture flag). Matches `srl_gdbstub.hpp`'s own constants
  of the same name exactly (confirmed both sides of the "shared hardware
  resource" story use the identical register/bit).
- `GBR = 0x060FFC00` (persistent master GBR value throughout normal
  operation — this toolchain uses GBR the way `-mgbr`-style ABIs use a
  small-data base register, for compact `mov.l @(disp,gbr),rN` access to
  SGL's own global workarea). `GBR+72` (`0x060FFC48`) holds `_ComWrPtr`, the
  advancing slave-command queue write pointer.

**Caution for future reverse-engineering in this codebase:** not every
`@(72,gbr)` instance found via a blind grep across object files refers to
this same queue pointer — several SGL functions (e.g. `_slRparaInitSet` /
its internal `rpara_init` helper in `sglB038.o`) temporarily repoint GBR to
an unrelated structure for their own local purposes (saving/restoring the
*caller's* GBR around the call), so the same displacement can coincidentally
land on a completely different piece of data depending on which GBR is
active at the time. Confirm the GBR base is the persistent, global one
before treating a match as relevant.

---

## Suggested next steps, if someone picks this up

- Confirm which function is the *actual* slave-side consumer of the
  `0x260ea000`-based advancing queue by finding what dereferences a pointer
  originating from `*(GBR+72)`'s *initial* value (rather than the fixed
  `_SlaveCommand` symbol, which this investigation showed is a red herring).
  `slInitSystem()`'s own disassembly (not yet examined) is the likely place
  that pointer gets its starting value.
- Once the real consumer is identified, single-step through it the same way
  bug #1 was diagnosed (a master-side breakpoint on the consumer function
  works fine; it's slave-side code that has the caching/exception
  complications) to see exactly what state it expects across a breakpoint
  interruption.
- A from-scratch (non-SGL) slave wake-up path remains the fallback if SGL's
  internal expectations turn out to be fundamentally incompatible with any
  nested exception occurring mid-dispatch — this is the same conclusion
  `slave_counter_task.hpp`'s own `@warning` already reached for the
  unrelated freeze-handler conflict, and this investigation didn't find
  anything to contradict it applying here too.
