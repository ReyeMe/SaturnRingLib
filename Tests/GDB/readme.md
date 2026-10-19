# GDB Stub Protocol Test Campaign

Host-side integration tests for `saturnringlib/srl_gdbstub.hpp`, the custom
GDB Remote Serial Protocol (RSP) stub used for Sega Saturn SH-2 debugging.

## Why this campaign is different from `../src`

The rest of `Tests/` is a purely on-target campaign (minunit): the Saturn
itself runs the tests and reports pass/fail over its own log output. That
doesn't work for the GDB stub, because RSP is inherently a two-sided
conversation -- `process_commands()` reads raw bytes directly off the real
serial/DevCart transport and blocks waiting for them. There is no seam to
feed it a synthetic packet without an actual GDB client on the wire.

So this campaign drives a **real `gdb-multiarch` client** against a **real,
running Saturn** and asserts on its behavior from the host side, the same way
a human would validate the stub by hand.

## What it tests against

Rather than duplicating a target ROM, this campaign reuses
`Samples/Debug - GDB Stub` as its system under test: that sample already
ships a hardware-validated set of trigger points -- breakpoint-friendly
functions, all eight SH-2 exception vectors reachable via `monitor crash
<type>`, a watchable global (`g_testVariable`), and a slave-CPU breakpoint
target (`SlaveCounterTask::Do()`) -- built specifically to exercise the stub.
See that sample's own `readme.md` for what each trigger does and why.

## Coverage

`gdb_protocol_tests.py` runs every test on a **freshly booted target**: before
each test the runner power-cycles the console (when a PSU address is given),
uploads the sample and starts a new `ftx -g` proxy. No test can inherit a
crashed CPU, an armed breakpoint, a busy UBC channel or a changed
`g_testVariable` from an earlier one, so each result stands on its own. Tests
check one behavior each; tests named `scenario: ...` chain several debugger
operations on purpose. Coverage:

- **Protocol negotiation**: `qSupported`, `qXfer:features:read` (target.xml),
  `qAttached`, `qOffsets`, `qC`, `qfThreadInfo`/`qsThreadInfo`, `H`, `T`,
  `vCont?`, `?` -- each sent as a raw packet via GDB's `maint packet` and
  checked against the stub's actual implementation.
- **Registers**: `g`/`G` (full set, round-tripped byte-for-byte), `p`/`P`
  (individual register read/write round-trip).
- **Memory**: `m`/`M` aligned and *unaligned* writes (regression coverage for
  `hex2mem_aligned()`), plus a dedicated VDP2 CRAM write regression test --
  this is the exact hardware bug found and fixed in this codebase, where
  byte-wise memory writes silently dropped their high byte because VDP RAM
  doesn't reliably latch single-byte bus writes.
- **Breakpoints**: software breakpoint (`Z0`/`z0`) set/hit/remove, and two
  breakpoints hit in sequence.
- **Watchpoints**: hardware write/read/access watchpoints (`Z2`/`Z3`/`Z4`) on
  a live global.
- **Stepping**: `s`/`S` walked across several sequential statements, checked
  against the expected final computed value.
- **Exception vectors**: all eight `monitor crash <type>` triggers except
  `addr` (see `debug_triggers.hpp` in the sample -- documented as a
  pre-existing, unrelated hardware/config quirk where the CPU address error
  doesn't actually fault on this hardware; skipped rather than asserted
  either way).
- **Slave CPU breakpoints**, including a **dedicated regression test** for
  the `SlaveReleaseGuard` fix: hit a slave breakpoint, detach, then confirm a
  brand new session can re-arm and hit the *same* breakpoint again. Before
  that fix, any non-`continue`/`step` disconnect left the slave permanently
  parked and it would never fire again.
- **Detach (`D`) / kill (`k`)**: both leave the stub healthy for an
  immediate, independent reconnect.
- **`monitor` diagnostics**: `trace`.
- **Timeouts**: an idle-but-connected client (several seconds of silence)
  is still served normally afterward; a `continue` left free-running for
  several seconds unattended can still be reclaimed with Ctrl-C.
- **Unknown / malformed commands**, one test per packet: an unsupported
  top-level command, `Z`/`z` with a bad type digit or a missing separator,
  `m`/`M` with a missing `,`/`:`, removing a breakpoint that was never set
  (defined as a no-op success, not an error), and reading/writing the
  SH-2 peripheral register space (rejected by `is_valid_memory_range()`,
  since byte-wise access to much of that space causes real bus errors on
  this hardware) -- each checked against its actual defined error code or
  "not supported" reply, followed by a sanity command confirming the stub
  wasn't left wedged by it.
- **Disconnections**: beyond the clean `D`/`k` paths above, an *abrupt*
  disconnect -- the gdb process killed outright, no detach, while a
  breakpoint is armed and a `continue` is in flight -- confirmed to still
  leave the stub healthy for an immediate, independent reconnect. This
  exercises `process_commands()`'s `packet_get()`-failure path specifically
  (distinct from `D`/`k`'s explicit cleanup).
- **Command combinations**: a breakpoint deleted before ever being hit
  (never fires); a hardware breakpoint (`Z1`/`hbreak`, the UBC-based
  mechanism distinct from software `Z0`); `continue` → `step` → `continue`
  in one session; only the *last* `monitor` command queued while stopped
  taking effect (per this project's documented dispatch semantics); and a
  software breakpoint and a hardware watchpoint armed at the same time,
  each firing independently and in the right order.
- **Scenarios**: the same breakpoint hit three times in a row; `finish` back
  to the caller; `nexti` through a function's delayed branches (`jsr`/`rts`)
  without a fake SIGILL; conditional breakpoints with a true and a false
  condition; a variable written while stopped being what the resumed program
  sees; Ctrl-C, `bt`, then a breakpoint; recovering from a reserved-instruction
  crash by stepping `$pc` over it (exercises the `C04` resume); a master
  breakpoint after a slave breakpoint stop (regression test for the
  stale-cache wedge); and a breakpoint left armed by a killed GDB not stopping
  the next session.

Not covered: `G` is only exercised as an identity round-trip (fetch then
write back the same register blob) rather than with new values, to avoid
corrupting live target state mid-test-suite; a genuinely new-value `G` write
would need a real use case (like an inferior function call/return) that
carries its own risk of hanging the session -- see this project's notes on
GDB inferior calls breaking down outside a real halt.

## Requirements

Real Saturn hardware + USB DevCart, reachable the same way the rest of this
repo's hardware testing is: `ftx`, `usbreset`, and `gdb-multiarch` on `PATH`,
plus `python3` (no extra pip packages). This does **not** run under Kronos or
mednafen -- the stub's transport is the physical DevCart serial link, which
the emulators don't expose.

## Running

```bash
./run_gdb_tests.sh                       # re-upload before every test, no power-cycle
./run_gdb_tests.sh 192.168.1.50          # power-cycle via a network PSU before every test,
                                          # same REST API as ../run_tests.bat's
                                          # USBGamers mode (relay_status/on/off)
./run_gdb_tests.sh 192.168.1.50 --filter scenario   # extra options go to gdb_protocol_tests.py
```

This builds the sample, then runs `gdb_protocol_tests.py`, which reloads the
target before every test (power-cycle, upload with retries, fresh `ftx -g`
proxy). A full run of all 68 tests takes about 45 minutes with a PSU; use
`--filter` to run a subset. Results print to the terminal and are
written to `gdb_uts.log` (`***UT_START***`/`***UT_END***` markers, one
`PASS`/`FAIL`/`SKIP`/`ERROR` line per test, final counts), mirroring the
on-target campaign's log format. Exits non-zero if any test failed or
errored (skips don't count as failures).

You can also run the test suite directly against an already-uploaded,
already-proxied target, without reloading between tests (tests then share one
target, so a crash trigger takes every later test down with it):

```bash
python3 gdb_protocol_tests.py "../../Samples/Debug - GDB Stub/BuildDrop/Debug_GDBStub.elf" --port 1234
```

## Known issues on real hardware

**The rig.** The first upload after a power-cycle routinely fails ("usb bulk
write failed"); the runner retries after a fresh `usbreset`, and power-cycles
again if that keeps failing. The network PSU itself has dropped off the
network mid-run; the console then loses power and every later test fails
with "monitor command not supported by this target" or FTDI errors in
`ftx_gdb.log` -- check the PSU and rerun.

**Fixed: hardware breakpoints and watchpoints (`Z1`-`Z4`, the SH-2 UBC) never
fired.** Three things were wrong: BRCR was written at `0xFFFFFF60` (that is
BARBH, channel B -- BRCR is `0xFFFFFF78`); every BBRA condition field left at
`00` means "never break", so the CPU-cycle bits (7-6) and, for instruction
fetches, the read bits (3-2) must be set; and a UBC stop was reported as a
plain SIGTRAP, while GDB only recognizes a watchpoint hit when the stop reply
names it (`watch:`/`rwatch:`/`awatch:`/`hwbreak:`). Removing a watchpoint
now clears BBRA, which is what actually disarms the channel.

**A breakpoint left armed by a killed GDB stays patched in.** `D`/`k` clear
breakpoints, but a GDB that dies while the target runs leaves its `0xFFFF`
patches in memory. The next session doesn't notice: the stale breakpoint
still fires, GDB silently resumes from a stop it never asked for, and the
stub re-inserts the patch. The `left armed by a killed GDB` scenario checks
the memory directly and is expected to fail until the stub clears breakpoints
when a new session starts.

**Fixed: a slave breakpoint used to wedge the program.** The two SH-2 caches
don't see each other's writes, and the slave waited for `g_slave.Resume`
through its own cache, so it never saw the master release it -- it stayed
parked inside SGL's slave interrupt, freezing the main loop. The same stale
read made `ITask::IsRunning()` never clear on the master. Both now read these
cross-CPU flags through the cache-through (`0x20000000`) mirror.
