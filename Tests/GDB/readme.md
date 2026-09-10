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

`gdb_protocol_tests.py` covers, in isolated per-test sessions (each test
connects fresh and detaches/closes on its own, so failures don't cascade):

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
- **`monitor` diagnostics**: `regs vdp`, `trace`, `nmi`.
- **Timeouts**: an idle-but-connected client (several seconds of silence)
  is still served normally afterward; a `continue` left free-running for
  several seconds unattended can still be reclaimed with Ctrl-C.
- **Unknown / malformed commands**, all in one session: an unsupported
  top-level command, `Z`/`z` with a bad type digit or a missing separator,
  `m`/`M` with a missing `,`/`:`, removing a breakpoint that was never set
  (defined as a no-op success, not an error), and reading/writing the
  SH-2 peripheral register space (rejected by `is_valid_memory_range()`,
  since byte-wise access to much of that space causes real bus errors on
  this hardware) -- each checked against its actual defined error code or
  "not supported" reply, with a final sanity command confirming the stub
  wasn't left wedged by any of them.
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
./run_gdb_tests.sh                  # reuse whatever's already powered on
./run_gdb_tests.sh 192.168.1.50     # power-cycle via a network PSU first,
                                     # same REST API as ../run_tests.bat's
                                     # USBGamers mode (relay_status/on/off/toggle)
```

This builds and uploads the sample, starts the `ftx -g` GDB proxy, then runs
`gdb_protocol_tests.py` against it. Results print to the terminal and are
written to `gdb_uts.log` (`***UT_START***`/`***UT_END***` markers, one
`PASS`/`FAIL`/`SKIP`/`ERROR` line per test, final counts), mirroring the
on-target campaign's log format. Exits non-zero if any test failed or
errored (skips don't count as failures).

You can also run the test suite directly against an already-uploaded,
already-proxied target:

```bash
python3 gdb_protocol_tests.py "../../Samples/Debug - GDB Stub/BuildDrop/Debug_GDBStub.elf" --port 1234
```

## Known flakiness on real hardware

Validated across repeated full runs on real hardware, reliably passing: all
protocol negotiation (`qSupported`/`qXfer`/`qAttached`/`qOffsets`/`qC`/
thread-info/`H`/`T`/`vCont?`/`?`), `g`/`G`/`p`/`P` register access,
aligned/unaligned `M` writes, the CRAM write regression test, a single
software breakpoint, all eight exception vectors, detach/kill + reconnect,
the `monitor` diagnostics, the idle-client and long-running-continue timeout
tests, the unknown/malformed-command test, the dirty-disconnect test, and
the breakpoint-removed-before-hit/mixed-resume-types/monitor-priority
combination tests (each independently confirmed passing in an isolated,
freshly power-cycled session -- see below for what "isolated" rules out).

Two different things cause the remaining flakiness, and they should not be
conflated:

**1. The rig itself degrades under this suite's reconnect rate.** Each test
opens a brand-new `target remote` session (50 of them per run) -- far
heavier, faster churn than this project's normal one-connection-at-a-time
hardware testing. The `ftx` proxy/USB link has repeatedly been observed to
degrade partway through a full run the same way it's documented to degrade
under heavy use elsewhere in this project (see the hardware-rig notes),
recovering only after a full power-cycle. If a run shows a cluster of "no
reply"/empty results, or realistic-looking commands getting a stale reply
left over from an earlier command, concentrated in its second half, that's
this -- power-cycle and rerun. The two-software-breakpoints,
slave-breakpoint, and `k`/dirty-disconnect-reconnect tests are the most
exposed to this, since each depends on either a longer command chain or a
`continue` landing within its timeout window.

**2. Hardware breakpoints and watchpoints (`Z1`-`Z4`, the SH-2 UBC) do not
fire reliably, independent of rig churn.** This was originally assumed to be
another instance of (1) -- it isn't. Isolated, manually-driven sessions
against a freshly power-cycled target (no other test running before or
after, generous 10s+ timeouts, the shared UBC channel confirmed idle
beforehand) still show `watch`/`rwatch`/`awatch`/`hbreak` failing to report
a hit noticeably more often than not, including cases where:
  - `Z1` (`hbreak`) installs successfully (gdb reports "Hardware assisted
    breakpoint N at ..." with no error) but the CPU reaching that exact
    instruction never produces a stop, even though the equivalent `BBRA`
    register configuration (`0x0010`, instruction-fetch) fires reliably when
    programmed directly by this sample's own `UserBreakController()` trigger
    (`monitor crash ubc`, which passes consistently).
  - A plain `watch g_testVariable` + a `monitor touch` that's known to
    execute the write (confirmed separately via memory reads) still doesn't
    produce a watchpoint stop.
  - This affects the write/read/access watchpoint tests, the two
    breakpoint-combination tests that involve a watchpoint or `hbreak`
    (`hardware breakpoint`, `a software breakpoint and a hardware watchpoint
    coexist`), and appears to be the same underlying issue in each case --
    not something specific to any one Z-type or to the software breakpoint
    coexisting alongside it.

  This looks like a genuine gap somewhere in the UBC-based `Z1`-`Z4` path
  (`install_hardware_watchpoint()`/the exception routing that reports a UBC
  stop back to gdb) rather than a test-suite problem, but pinning down
  exactly where is a separate investigation from writing test coverage --
  these tests are being kept (not weakened or skipped) specifically so that
  investigation has a reliable, scripted way to reproduce and verify a fix.
  A `test_watchpoint`/`test_hardware_breakpoint` failure is not itself cause
  to suspect this campaign's own code; a `git blame`/history check on
  `install_hardware_watchpoint()` and its exception-reporting path is the
  more useful next step.

`g_testVariable` is also live, shared target state, not reset between
tests: each test connects fresh, but the *target* keeps running continuously
across the whole suite, so a `monitor touch`/`step` queued by one test that
gets cut short (e.g. by a timeout, before its own cleanup runs) can still
land and nudge `g_testVariable` before the next test reads it. The
single-step test's exact expected value is the one most exposed to this.
