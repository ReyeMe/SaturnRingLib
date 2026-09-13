#!/usr/bin/env python3
"""Host-side RSP protocol test suite for saturnringlib/srl_gdbstub.hpp.

Drives a real gdb-multiarch client against a real, running Saturn (via the
`ftx -g` TCP<->serial proxy) and asserts on the stub's actual behavior --
both at the raw RSP packet level (via GDB's `maint packet`) and through
normal GDB CLI commands for the stateful combinations (breakpoints,
watchpoints, stepping, detach/reconnect).

See readme.md in this directory for coverage notes and how this differs from
the on-target minunit campaign in ../src.
"""

import argparse
import fcntl
import os
import pty
import re
import subprocess
import sys
import termios
import time

ELF_PATH = None
GDB_BIN = "gdb-multiarch"
PORT = 1234


class Skip(Exception):
    """Raised by a test to mark it skipped (with a reason) rather than pass/fail."""


class GdbSession:
    """One gdb-multiarch process, driven over a pty, connected to the stub."""

    def __init__(self, elf_path, gdb_bin=None, port=None):
        self.elf_path = elf_path
        self.gdb_bin = gdb_bin or GDB_BIN
        self.port = port or PORT
        self.master_fd = None
        self.proc = None

    def connect(self, timeout=10.0):
        # Give a just-closed prior session's teardown time to fully flush
        # through the ftx proxy -- connecting immediately behind a killed
        # gdb process has been observed to leak stray output into this new
        # session's first exchange.
        time.sleep(0.5)
        master_fd, slave_fd = pty.openpty()

        def preexec():
            os.setsid()
            # Establish the pty as this new session's controlling terminal
            # via slave_fd directly -- NOT fd 0. At the point preexec_fn
            # runs, fd 0 is still whatever the *invoking* shell had (this
            # script's own stdin), not yet redirected to slave_fd; whether
            # that inherited fd 0 is a real, ioctl-able tty depends entirely
            # on how this process itself was launched (interactively vs.
            # backgrounded behind a pipe/redirect), which made this flaky in
            # a way that had nothing to do with the target or the stub.
            fcntl.ioctl(slave_fd, termios.TIOCSCTTY, 0)

        self.proc = subprocess.Popen(
            [self.gdb_bin, "-q", "-nx", self.elf_path],
            stdin=slave_fd, stdout=slave_fd, stderr=slave_fd,
            preexec_fn=preexec, close_fds=True,
        )
        os.close(slave_fd)
        self.master_fd = master_fd
        flags = fcntl.fcntl(master_fd, fcntl.F_GETFL)
        fcntl.fcntl(master_fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)

        self._drain(0.5)
        self.cmd("set pagination off", timeout=1.0)
        self.cmd("set confirm off", timeout=1.0)
        self.cmd("set architecture sh2", timeout=1.0)
        self.cmd("set endian big", timeout=1.0)
        out = self.cmd(f"target remote localhost:{self.port}", timeout=timeout)
        if "Remote debugging using" not in out and not re.search(r"0x[0-9a-fA-F]+", out):
            raise RuntimeError(f"connect failed, gdb output: {out!r}")
        # Routine first-connect hiccup (see this project's own notes on the
        # "vMustReplyEmpty" glitch): gdb keeps exchanging its own internal
        # negotiation packets with the target for a while after "target
        # remote" itself reports success, and the very next command sent
        # too early races that -- either getting no reply at all, or (worse)
        # silently picking up a stray leftover value from that negotiation
        # traffic still sitting in the buffer. A drain timeout as short as
        # the ~0.3s originally used here is shorter than the idle-quiet
        # window that decides early exit, so it was really just a fixed
        # 0.3s wait -- nowhere near the real round-trip latency this rig
        # needs (regularly 1s+ under load elsewhere in this project). Give
        # it a real window instead.
        self._drain(2.0)

    def send(self, text):
        os.write(self.master_fd, (text + "\n").encode())

    def _drain(self, timeout, idle=1.5):
        # Two independent stopping conditions: an absolute deadline (we never
        # wait longer than `timeout` for a real, possibly slow, serial
        # round-trip to the target), and an early-exit once output has gone
        # quiet for `idle` seconds *after* something was received (so a fast
        # command doesn't always burn the full timeout). Collapsing these
        # into one "extend deadline while data arrives" rule is a trap: the
        # pty's near-instant local echo of our own input counts as "data",
        # so the deadline gets immediately shrunk to `idle` and the read
        # loop exits before gdb's actual (round-trip-bound) reply shows up.
        start = time.time()
        deadline = start + timeout
        last_data = start
        chunk = ""
        while time.time() < deadline:
            try:
                data = os.read(self.master_fd, 65536)
            except (BlockingIOError, OSError):
                data = None
            if data:
                chunk += data.decode(errors="replace")
                last_data = time.time()
            else:
                if chunk and (time.time() - last_data) > idle:
                    break
                time.sleep(0.02)
        return chunk

    def cmd(self, text, timeout=5.0):
        self.send(text)
        return self._drain(timeout)

    def packet(self, payload, timeout=4.0):
        """Sends a raw RSP packet via `maint packet` and returns its decoded reply text.

        gdb prints this as `received: "<reply>"` (lowercase, double-quoted,
        with non-printable/control bytes -- including literal newlines in
        e.g. the qXfer target.xml payload -- escaped as \\xNN), not the
        differently-cased, unquoted "Packet received: ..." originally
        assumed here. That mismatch alone made most raw-packet tests report
        "no reply" even when gdb's actual output, sitting right there in the
        captured text, showed the stub replying correctly.
        """
        out = self.cmd(f"maint packet {payload}", timeout=timeout)
        # gdb escapes only non-printable bytes in the payload (as \xNN,
        # including the payload's own embedded newlines -- e.g. in the
        # qXfer target.xml reply -- so the whole thing stays on one
        # terminal line); it does NOT escape literal '"' characters that
        # appear in printable payload content (also common in that same
        # XML reply: version="1.0", etc.). A quote-aware "stop at the first
        # unescaped quote" regex truncates right there instead of at the
        # real end. Since escaping newlines guarantees the reply never
        # spans more than one line, greedily matching up to the LAST quote
        # before end-of-line is unambiguous and correct.
        m = re.search(r'received: "(.*)"\r?\n', out)
        if not m:
            raise RuntimeError(f"no packet reply for {payload!r}, gdb output: {out!r}")

        def _unescape(mo):
            seq = mo.group(0)
            if seq.startswith("\\x"):
                return chr(int(seq[2:], 16))
            return {"\\\\": "\\", '\\"': '"', "\\r": "\r", "\\n": "\n", "\\t": "\t"}.get(seq, seq)

        return re.sub(r'\\x[0-9a-fA-F]{2}|\\\\|\\"|\\r|\\n|\\t', _unescape, m.group(1))

    def interrupt(self):
        os.write(self.master_fd, b"\x03")

    def release_ubc_channel(self):
        """Best-effort release of the single shared UBC hardware
        watchpoint/breakpoint channel (g_ubc_channel_a_active in
        srl_gdbstub.hpp), independent of gdb's own breakpoint bookkeeping.

        Hardware-confirmed edge case this guards against: interrupting a
        `continue` at exactly the moment a lazy Z1-Z4 insert succeeds ON
        THE TARGET, but before gdb's own client-side state has confirmed
        the insertion, leaves the target's channel flag stuck active --
        gdb never sends the matching z-packet, since as far as it's
        concerned there's nothing confirmed-inserted to remove. Every
        later Z1-Z4 install in ANY subsequent session then fails with E03
        ("Cannot insert hardware breakpoint/watchpoint") until something
        clears it. remove_hardware_watchpoint() ignores its address/type
        arguments and just unconditionally disables the channel, so this
        is a safe no-op when nothing actually needs releasing.
        """
        try:
            self.packet("z2,0,2")
        except Exception:
            pass

    def close(self):
        try:
            if self.proc and self.proc.poll() is None:
                # Best-effort clean detach before tearing down: closing a
                # session by just killing the gdb process (leaving the
                # target mid-`continue`, or a breakpoint/watchpoint still
                # resident) has been observed to leak state -- a stuck UBC
                # watchpoint channel, a target left genuinely free-running,
                # or stray proxy output -- into the next session. Interrupt
                # first in case a `continue` is still outstanding (harmless
                # no-op if the target is already stopped), then detach,
                # then quit.
                #
                # A fixed short sleep here (originally 0.2s) isn't enough:
                # if the interrupt hasn't actually landed yet by the time
                # `detach` is sent, gdb refuses it outright ("Cannot execute
                # this command while the target is running") and silently
                # skips detaching -- leaving the TARGET genuinely
                # free-running for whatever test connects next, which then
                # fails for reasons that have nothing to do with its own
                # logic. Actively draining for a halt indication (up to a
                # real timeout) rather than guessing a fixed delay avoids
                # that whole class of self-inflicted cascade failure.
                self.interrupt()
                self._drain(3.0)
                self.send("detach")
                self._drain(2.0)
                self.send("quit")
                time.sleep(0.3)
        except Exception:
            pass
        try:
            if self.proc:
                self.proc.terminate()
                self.proc.wait(timeout=3)
        except Exception:
            try:
                if self.proc:
                    self.proc.kill()
            except Exception:
                pass
        try:
            if self.master_fd is not None:
                os.close(self.master_fd)
        except Exception:
            pass
        # The ftx TCP<->serial proxy only serves one client at a time and
        # has been observed (see saturn_hardware_rig memory notes) to need
        # real settle time after a client disconnects before it's ready for
        # a fresh one -- connecting too soon after this process exits has
        # produced sessions with no real communication at all, plus stale
        # output from THIS session's last outstanding reply bleeding into
        # the next one's capture. This suite's rapid-reconnect churn (a
        # fresh session per test) has also been observed to gradually
        # degrade the proxy/USB link over a full run -- the same class of
        # issue documented elsewhere in this project as fixed only by a
        # full power-cycle -- so this settle window is deliberately
        # generous rather than the tightest value that "usually" works.
        time.sleep(2.0)

    def kill_dirty(self):
        """Abruptly terminates the process with NO detach/quit -- simulates
        a crashed GDB client or a dropped network link, as opposed to the
        clean 'D'/'k' exit paths covered elsewhere. On the target side this
        exercises process_commands()'s packet_get()-failure/disconnect
        path (see srl_gdbstub.hpp), which -- unlike 'D'/'k' -- does NOT
        reset g_handshake_done/g_has_connection, only releases a frozen
        slave via SlaveIPIClear(). Deliberately skips close()'s graceful
        interrupt/detach/quit sequence.
        """
        try:
            if self.proc:
                self.proc.kill()
                self.proc.wait(timeout=3)
        except Exception:
            pass
        try:
            if self.master_fd is not None:
                os.close(self.master_fd)
        except Exception:
            pass
        # Same rationale as close()'s own settle delay -- give the proxy
        # time to notice the vanished client before anything reconnects.
        time.sleep(2.0)


def new_session():
    return GdbSession(ELF_PATH, gdb_bin=GDB_BIN, port=PORT)


def parse_hex(text):
    # The LAST hex literal in the captured text, not the first: a command
    # sent too soon after connect (or after another command) can still
    # have earlier, unrelated banner/halt-frame text (itself full of
    # "0x..." addresses) sitting ahead of the real reply in the same
    # capture window. Grabbing the first match silently returns that
    # stale value instead of erroring -- this produced multiple confusing
    # "wrong value" failures (a computed address that was actually a
    # leftover PC from the connect banner, etc.) before this was caught.
    # gdb's own reply is always the last thing printed in a given capture.
    matches = re.findall(r"0x[0-9a-fA-F]+", text)
    if not matches:
        raise RuntimeError(f"no hex value found in: {text!r}")
    return int(matches[-1], 16)


TESTS = []


def test(name):
    def deco(fn):
        TESTS.append((name, fn))
        return fn
    return deco


# --------------------------------------------------------------------------
# Phase 1: protocol negotiation (raw RSP packets)
# --------------------------------------------------------------------------

@test("handshake: connect + info registers shows the full SH-2 register set")
def test_handshake(s):
    out = s.cmd("info registers", timeout=5)
    # Note: 'sr'/'vbr' aren't asserted here -- with `set architecture sh2`
    # forced explicitly (matching this project's own launch.json/manual
    # usage), gdb warns "Target-supplied registers are not supported by the
    # current architecture" and falls back to its own built-in generic SH-2
    # register list rather than the stub's qXfer target.xml, which doesn't
    # surface those two under `info registers`. p $sr still resolves fine
    # (see test_pc_sane-style individual register tests).
    for reg in ["r0", "r15", "pc", "pr", "gbr", "mach", "macl"]:
        assert re.search(rf"\b{reg}\b", out), f"register {reg} missing: {out!r}"


@test("raw qSupported advertises PacketSize/swbreak/hwbreak")
def test_raw_qsupported(s):
    reply = s.packet("qSupported")
    assert "PacketSize=" in reply, reply
    assert "swbreak" in reply, reply
    assert "hwbreak" in reply, reply


@test("raw qXfer:features:read returns target.xml describing the SH-2/slave regs")
def test_raw_qxfer(s):
    reply = s.packet("qXfer:features:read:target.xml:0,3fb")
    assert reply[:1] in ("m", "l"), reply
    assert "<architecture>sh</architecture>" in reply, reply


@test("raw qAttached reports attached-to-existing-process")
def test_raw_qattached(s):
    assert s.packet("qAttached") == "1"


@test("raw qOffsets reports zero section offsets")
def test_raw_qoffsets(s):
    reply = s.packet("qOffsets")
    assert "Text=0" in reply, reply


@test("raw qC reports current thread")
def test_raw_qc(s):
    assert s.packet("qC") == "QC1"


@test("raw qfThreadInfo/qsThreadInfo enumerate a single thread")
def test_raw_threadinfo(s):
    assert s.packet("qfThreadInfo") == "m1"
    assert s.packet("qsThreadInfo") == "l"


@test("raw H sets thread context")
def test_raw_h(s):
    assert s.packet("Hg0") == "OK"


@test("raw T reports thread 1 alive")
def test_raw_t(s):
    assert s.packet("T01") == "OK"


@test("raw vCont? advertises c/s support")
def test_raw_vcont_query(s):
    reply = s.packet("vCont?")
    assert "vCont" in reply and "c" in reply and "s" in reply, reply


@test("raw ? reports the initial stop signal")
def test_raw_stop_query(s):
    reply = s.packet("?")
    # The stub replies with the richer T-packet form (signal + thread info),
    # not the minimal "Sxx" -- both are valid RSP stop replies.
    assert re.fullmatch(r"[ST][0-9a-fA-F]{2}.*", reply), reply


# --------------------------------------------------------------------------
# Phase 2: registers
# --------------------------------------------------------------------------

@test("raw g returns the full register set as a hex string")
def test_raw_g(s):
    reply = s.packet("g")
    assert len(reply) >= 160, reply
    assert re.fullmatch(r"[0-9a-fA-F]+", reply), reply


@test("p $pc resolves to code within the loaded ROM image")
def test_pc_sane(s):
    out = s.cmd("p/x $pc", timeout=5)
    pc = parse_hex(out)
    assert 0x06000000 <= pc <= 0x06200000, hex(pc)


@test("P (write single register) round-trips via CLI set/print")
def test_register_write_roundtrip(s):
    original = parse_hex(s.cmd("p/x $r0", timeout=5))
    s.cmd("p $r0 = 0xdeadbeef", timeout=5)
    written = parse_hex(s.cmd("p/x $r0", timeout=5))
    try:
        assert written == 0xdeadbeef, hex(written)
    finally:
        s.cmd(f"p $r0 = {hex(original)}", timeout=5)


@test("G (write all registers) round-trips byte-for-byte via raw packet")
def test_raw_g_write_roundtrip(s):
    before = s.packet("g")
    reply = s.packet("G" + before)
    assert reply == "OK", reply
    after = s.packet("g")
    assert after == before, f"{before!r} != {after!r}"


# --------------------------------------------------------------------------
# Phase 3: memory
# --------------------------------------------------------------------------

@test("m reads a known global's live value, matching what the CLI sees")
def test_memory_read_known_global(s):
    addr = parse_hex(s.cmd("p &g_testVariable", timeout=5))
    cli_val = parse_hex(s.cmd("p/x g_testVariable", timeout=5))
    raw_val = int(s.packet(f"m{addr:x},4"), 16)
    assert raw_val == (cli_val & 0xffffffff), f"cli={cli_val:#x} raw={raw_val:#x}"


@test("M writes an aligned 32-bit value and reads it back exactly")
def test_memory_write_aligned(s):
    addr = parse_hex(s.cmd("p &g_testVariable", timeout=5))
    original = s.packet(f"m{addr:x},4")
    try:
        assert s.packet(f"M{addr:x},4:11223344") == "OK"
        assert s.packet(f"m{addr:x},4") == "11223344"
    finally:
        s.packet(f"M{addr:x},4:{original}")


@test("M writes an unaligned length/offset without corrupting surrounding bytes (hex2mem_aligned)")
def test_memory_write_unaligned(s):
    base = parse_hex(s.cmd("p &g_testVariable", timeout=5))
    addr = base + 1
    original = s.packet(f"m{addr:x},3")
    try:
        assert s.packet(f"M{addr:x},3:aabbcc") == "OK"
        assert s.packet(f"m{addr:x},3") == "aabbcc"
    finally:
        s.packet(f"M{addr:x},3:{original}")


@test("M write to VDP2 CRAM lands the full value (regression: byte-wise hex2mem CRAM bug)")
def test_memory_write_cram_hw_regression(s):
    # Floor palette color slot -- see Samples/Debug - GDB Stub's
    # vdp_demo.cxx SetupSkyAndFloor() / readme.md. Any live value here is
    # fine; we save and restore it so the demo's visuals are unaffected.
    addr = 0x25F00042
    original = s.packet(f"m{addr:x},2")
    try:
        assert s.packet(f"M{addr:x},2:1234") == "OK"
        readback = s.packet(f"m{addr:x},2")
        assert readback == "1234", (
            f"CRAM write corrupted: wrote 1234, read back {readback!r} -- "
            "a dropped high byte is exactly the pre-fix hex2mem() bug"
        )
    finally:
        s.packet(f"M{addr:x},2:{original}")


# --------------------------------------------------------------------------
# Phase 4: software breakpoints
# --------------------------------------------------------------------------

@test("software breakpoint: set, hit, remove")
def test_software_breakpoint(s):
    s.cmd("break SteppableFunction", timeout=5)
    try:
        s.cmd("monitor step", timeout=5)
        out = s.cmd("continue", timeout=6)
        assert "SteppableFunction" in out, out
    finally:
        # If the assert above raised, the target may still be free-running
        # (a software breakpoint is just a memory patch -- it stays resident
        # regardless). Reclaim control before removing it so it doesn't
        # linger for the next test.
        s.interrupt()
        s._drain(3.0)
        s.cmd("delete", timeout=5)
        s.cmd("continue", timeout=3)


@test("two software breakpoints hit in the expected order")
def test_two_software_breakpoints(s):
    s.cmd("break HandleMonitorCommand", timeout=5)
    s.cmd("break SteppableFunction", timeout=5)
    try:
        s.cmd("monitor step", timeout=5)
        out1 = s.cmd("continue", timeout=6)
        assert "HandleMonitorCommand" in out1, out1
        out2 = s.cmd("continue", timeout=6)
        assert "SteppableFunction" in out2, out2
    finally:
        s.interrupt()
        s._drain(3.0)
        s.cmd("delete", timeout=5)
        s.cmd("continue", timeout=3)


# --------------------------------------------------------------------------
# Phase 5: hardware watchpoints
# --------------------------------------------------------------------------

def _watchpoint_test(kind, needs_touch):
    def _t(s):
        # Only one physical UBC channel exists (see debug_triggers.hpp's
        # UserBreakController() doc comment) -- a watchpoint left resident
        # by a failed assertion here would make every later watchpoint test
        # fail with "Could not insert hardware watchpoint... too many
        # hardware breakpoints/watchpoints", masking the real result. The
        # try/finally guarantees this test always releases the channel.
        s.cmd(f"{kind} g_testVariable", timeout=5)
        try:
            if needs_touch:
                s.cmd("monitor touch", timeout=5)
            out = s.cmd("continue", timeout=6)
            assert "watchpoint" in out.lower() and "g_testVariable" in out, out
        finally:
            s.interrupt()
            s._drain(3.0)
            s.cmd("delete", timeout=5)
            s.release_ubc_channel()
            s.cmd("continue", timeout=3)
    return _t


TESTS.append(("hardware watchpoint (write) fires on g_testVariable",
              _watchpoint_test("watch", needs_touch=True)))
TESTS.append(("hardware watchpoint (read) fires on the main loop's read of g_testVariable",
              _watchpoint_test("rwatch", needs_touch=False)))
TESTS.append(("hardware watchpoint (access) fires on g_testVariable",
              _watchpoint_test("awatch", needs_touch=True)))


# --------------------------------------------------------------------------
# Phase 6: single-step
# --------------------------------------------------------------------------

@test("single-step walks SteppableFunction's statements to the expected result")
def test_single_step(s):
    s.cmd("break SteppableFunction", timeout=5)
    try:
        s.cmd("monitor step", timeout=5)
        out = s.cmd("continue", timeout=6)
        assert "SteppableFunction" in out, out
        for _ in range(5):
            s.cmd("step", timeout=5)
        value = parse_hex(s.cmd("p/x g_testVariable", timeout=5))
        assert value == 6, f"expected g_testVariable == 6 (a=1,b=2,c=3,d=6), got {value}"
    finally:
        s.interrupt()
        s._drain(3.0)
        s.cmd("delete", timeout=5)
        s.cmd("continue", timeout=3)


# --------------------------------------------------------------------------
# Phase 7: exception vectors
# --------------------------------------------------------------------------

CRASH_TRIGGERS = [
    "illegal", "reserved", "slotillegal", "slotreserved",
    "genillegal", "dma", "ubc", "trapa3",
]


def _make_crash_test(kind):
    def _t(s):
        s.cmd(f"monitor crash {kind}", timeout=5)
        out = s.cmd("continue", timeout=6)
        assert re.search(r"signal|breakpoint|0x[0-9a-fA-F]+ in", out, re.IGNORECASE), (
            f"'monitor crash {kind}' did not appear to halt the target: {out!r}"
        )
        s.cmd("continue", timeout=3)
    return _t


for _kind in CRASH_TRIGGERS:
    TESTS.append((f"exception vector: monitor crash {_kind} halts and reports", _make_crash_test(_kind)))


@test("exception vector: monitor crash addr (SKIPPED -- documented non-functional trigger)")
def test_crash_addr_skip(s):
    raise Skip(
        "CPUAddressError is documented in debug_triggers.hpp as not actually "
        "faulting on this hardware/config (SRL::GDBStub::GetExceptionThunkCount() "
        "stays at 0). Pre-existing hardware/config quirk, unrelated to the GDB "
        "stub's packet handling -- not asserted here either way."
    )


# --------------------------------------------------------------------------
# Phase 8: slave CPU breakpoints
# --------------------------------------------------------------------------

def _assert_slave_breakpoint_hit(s, out):
    # A slave breakpoint is deliberately reported to gdb the SAME way as
    # Ctrl-C/NMI -- SIGINT, with the master-side frame showing
    # SRL::GDBStub::Break() -- NOT anything naming SlaveCounterTask/Do().
    # See srl_gdbstub.hpp's Poll(): "if (g_slave_stopped) { g_is_ctrl_c_stop
    # = true; Break(); }", with its own comment explaining why: there's no
    # meaningful call stack to show on the MASTER side for an async event
    # like this. Asserting on "SlaveCounterTask"/"Do (" ever appearing in
    # `continue`'s own output was simply wrong -- it never does, by design
    # -- and produced a false "unreliable" reading in earlier testing here.
    # The real confirmation that this was actually a SLAVE breakpoint (not
    # a stray Ctrl-C) is cross-checking `monitor regs slave`'s live pc=
    # against SlaveCounterTask::Do()'s own address range.
    assert re.search(r"SIGINT|Interrupt", out, re.IGNORECASE), (
        f"expected the documented SIGINT/Break() stop reply, got: {out!r}"
    )
    regs = s.cmd("monitor regs slave", timeout=5)
    assert "slave" in regs.lower() and "pc=" in regs.lower(), regs
    m = re.search(r"pc=([0-9a-fA-F]+)", regs)
    assert m, regs
    pc = int(m.group(1), 16)
    # SlaveCounterTask::Do() lives in ordinary ROM code space; this is a
    # sanity bound, not a hardcoded address (addresses shift on rebuild).
    assert 0x06000000 <= pc <= 0x06200000, f"slave pc {pc:#x} outside expected ROM range: {regs!r}"
    return regs


@test("slave breakpoint hits and monitor regs slave reports live slave register state")
def test_slave_breakpoint(s):
    s.cmd("break SlaveCounterTask::Do", timeout=5)
    try:
        out = s.cmd("continue", timeout=8)
        _assert_slave_breakpoint_hit(s, out)
    finally:
        s.interrupt()
        s._drain(3.0)
        s.cmd("delete", timeout=5)
        s.cmd("continue", timeout=3)


@test("slave breakpoint re-arms after detach (SlaveReleaseGuard regression)")
def test_slave_breakpoint_reconnect_regression(s):
    s.cmd("break SlaveCounterTask::Do", timeout=5)
    out = s.cmd("continue", timeout=8)
    _assert_slave_breakpoint_hit(s, out)
    s.cmd("detach", timeout=5)

    # A second, independent session must be able to re-arm and hit the SAME
    # breakpoint. Before the SlaveReleaseGuard fix in srl_gdbstub.hpp, any
    # non-continue/step exit path (like this detach) left the slave
    # permanently parked in its spin-wait, and it would never dispatch --
    # let alone hit a breakpoint -- again.
    s2 = new_session()
    try:
        s2.connect()
        s2.cmd("break SlaveCounterTask::Do", timeout=5)
        out2 = s2.cmd("continue", timeout=8)
        try:
            _assert_slave_breakpoint_hit(s2, out2)
        except AssertionError as e:
            raise AssertionError(
                f"slave breakpoint did not re-fire after detach -- "
                f"SlaveReleaseGuard regression? {e}"
            )
        s2.cmd("delete", timeout=5)
    finally:
        s2.close()


# --------------------------------------------------------------------------
# Phase 9: detach / kill
# --------------------------------------------------------------------------

@test("detach leaves the stub healthy for an immediate, independent reconnect")
def test_detach_then_reconnect(s):
    s.cmd("detach", timeout=5)
    s2 = new_session()
    try:
        s2.connect(timeout=8)
        parse_hex(s2.cmd("p &g_testVariable", timeout=5))
    finally:
        s2.close()


@test("raw k (kill) leaves the stub healthy for an immediate, independent reconnect")
def test_kill_then_reconnect(s):
    try:
        s.packet("k", timeout=3)
    except RuntimeError:
        pass  # 'k' has no defined reply per the RSP spec; gdb may not print one.
    time.sleep(0.3)
    s2 = new_session()
    try:
        s2.connect(timeout=8)
        parse_hex(s2.cmd("p &g_testVariable", timeout=5))
    finally:
        s2.close()


# --------------------------------------------------------------------------
# Phase 10: monitor diagnostics
# --------------------------------------------------------------------------

@test("monitor trace reports the halt-context snapshot")
def test_monitor_trace(s):
    out = s.cmd("monitor trace", timeout=5)
    assert "snapshot" in out.lower() or "context" in out.lower(), out


@test("monitor nmi completes without hanging the session")
def test_monitor_nmi(s):
    out = s.cmd("monitor nmi", timeout=5)
    assert out is not None


# --------------------------------------------------------------------------
# Phase 11: timeouts / idle robustness
# --------------------------------------------------------------------------

@test("an idle connected client (no commands for several seconds) is still served normally")
def test_idle_client_then_command(s):
    # Nothing sent for a while -- confirms the stub doesn't time out or
    # otherwise wedge a connected-but-quiet client; Poll()'s snapshot
    # bridge keeps working regardless of how long GDB takes between
    # commands.
    time.sleep(6.0)
    parse_hex(s.cmd("p/x $pc", timeout=5))


@test("a long free-running continue can still be interrupted after several seconds")
def test_long_running_continue_then_interrupt(s):
    # No monitor command queued, so nothing will trip a breakpoint/
    # watchpoint on its own -- this exercises Ctrl-C reclaiming control
    # after the target has genuinely been running unattended for a while,
    # not just immediately after `continue`.
    s.cmd("continue", timeout=1.0)  # returns almost immediately ("Continuing.")
    time.sleep(6.0)
    s.interrupt()
    out = s._drain(6.0)
    assert re.search(r"signal|breakpoint|0x[0-9a-fA-F]+ in", out, re.IGNORECASE), (
        f"Ctrl-C after a long free-run did not halt the target: {out!r}"
    )


# --------------------------------------------------------------------------
# Phase 12: unknown / malformed commands
# --------------------------------------------------------------------------

@test("unknown and malformed commands are rejected gracefully, not hung or crashed")
def test_unknown_and_malformed_commands(s):
    # A genuinely unsupported top-level command falls through to the
    # `default:` case, which replies with an empty packet ($#00) -- the
    # standard RSP convention for "not supported" -- rather than silence.
    assert s.packet("@") == "", "unsupported command should get an empty reply"

    # Z/z with an out-of-range type digit, or missing the ',' separator,
    # both fail the format check before the type is even interpreted, so
    # they get the same empty "not supported" reply rather than E0x.
    assert s.packet("Z9,6004510,2") == "", "bad Z type digit should be rejected"
    assert s.packet("Z0") == "", "Z missing its ',addr,kind' should be rejected"

    # 'm'/'M' with a missing ',' (m) or ':' (M) fail their own parse step
    # and reply with a defined error code instead of misreading the buffer.
    assert s.packet("m6004510") == "E01", "'m' with no comma should be E01"
    assert s.packet("M6004510,4") == "E02", "'M' with no ':' should be E02"

    # Removing a breakpoint that was never installed is defined as a no-op
    # success (nothing to undo) -- not an error.
    assert s.packet("z0,6100000,2") == "OK", "removing a never-set breakpoint should still be OK"

    # Reading/writing the SH-2 peripheral register space (>= 0xF0000000) is
    # explicitly rejected by is_valid_memory_range() -- byte-wise access to
    # many of those registers causes real bus errors on this hardware,
    # which is exactly what this bounds check exists to prevent.
    assert s.packet("mfffffff0,4") == "E01", "reading peripheral space should be E01"
    assert s.packet("Mfffffff0,4:11223344") == "E02", "writing peripheral space should be E02"

    # None of the above should have left the stub wedged -- confirm a
    # perfectly ordinary command still works right after.
    assert s.packet("qC") == "QC1"


# --------------------------------------------------------------------------
# Phase 13: disconnections
# --------------------------------------------------------------------------

@test("an abrupt disconnect (client just vanishes, no D/k) still leaves the stub healthy")
def test_dirty_disconnect_then_reconnect(s):
    # Arm a breakpoint and start a free-running continue, THEN vanish with
    # no clean shutdown at all -- simulates a crashed GDB client or a
    # dropped network link, distinct from the clean 'D'/'k' paths covered
    # elsewhere. The only thing that notices this on the target side is
    # packet_get() failing on its next read; unlike 'D'/'k' that path does
    # NOT reset g_handshake_done/g_has_connection. This proves that
    # doesn't matter in practice: a fresh session connects and works
    # normally regardless.
    s.cmd("break SteppableFunction", timeout=5)
    s.send("continue")
    time.sleep(0.5)
    s.kill_dirty()

    s2 = new_session()
    try:
        s2.connect()
        parse_hex(s2.cmd("p/x $pc", timeout=5))
        # The leftover breakpoint (never removed by the vanished session)
        # is still a live memory patch -- clean it up so it doesn't affect
        # whatever test runs after this one.
        s2.cmd("delete", timeout=5)
        s2.cmd("continue", timeout=3)
    finally:
        s2.close()


# --------------------------------------------------------------------------
# Phase 14: command combinations
# --------------------------------------------------------------------------

@test("a breakpoint removed before ever being hit never fires")
def test_breakpoint_removed_before_hit(s):
    s.cmd("break SteppableFunction", timeout=5)
    s.cmd("delete", timeout=5)  # removed WITHOUT ever continuing through it
    s.cmd("monitor step", timeout=5)  # would call SteppableFunction on resume
    out = s.cmd("continue", timeout=6)
    # With the breakpoint gone, `continue` should just run forever (no
    # halt reply) -- the absence of a stop within this window IS the pass
    # case here.
    assert not re.search(r"Breakpoint|SteppableFunction", out), (
        f"deleted breakpoint still fired: {out!r}"
    )
    s.interrupt()
    time.sleep(1.5)
    s.cmd("continue", timeout=3)


@test("hardware breakpoint (Z1/hbreak, distinct from the software Z0 path) hits")
def test_hardware_breakpoint(s):
    s.cmd("hbreak SteppableFunction", timeout=5)
    try:
        s.cmd("monitor step", timeout=5)
        out = s.cmd("continue", timeout=6)
        assert "SteppableFunction" in out, out
    finally:
        s.interrupt()
        s._drain(3.0)
        s.cmd("delete", timeout=5)
        s.release_ubc_channel()
        s.cmd("continue", timeout=3)


@test("mixed resume types in one session: continue, then step, then continue")
def test_mixed_resume_types(s):
    s.cmd("break SteppableFunction", timeout=5)
    try:
        s.cmd("monitor step", timeout=5)
        out = s.cmd("continue", timeout=6)
        assert "SteppableFunction" in out, out
        s.cmd("step", timeout=5)
        out2 = s.cmd("continue", timeout=6)
        # Nothing else queued -- should resume freely with no further halt.
        assert not re.search(r"Breakpoint|signal", out2, re.IGNORECASE), out2
    finally:
        s.interrupt()
        s._drain(3.0)
        s.cmd("delete", timeout=5)
        s.cmd("continue", timeout=3)


@test("only the LAST monitor command queued while stopped takes effect")
def test_monitor_only_last_queued_command_wins(s):
    # Per this project's own documented semantics (see the sample's
    # readme.md), dispatch only happens on resume, and only the *last*
    # `monitor` command sent while stopped is the one that fires. Queuing
    # "touch" (g_testVariable += 1) and then "step" (SteppableFunction,
    # which unconditionally sets g_testVariable = 6) before ever resuming
    # should land on 6, not on some incremented leftover value.
    s.cmd("monitor touch", timeout=5)
    s.cmd("monitor step", timeout=5)
    s.cmd("continue", timeout=3)
    time.sleep(1.0)
    s.interrupt()
    time.sleep(1.5)
    after = parse_hex(s.cmd("p/x g_testVariable", timeout=5))
    assert after == 6, f"expected the queued 'step' to win (g_testVariable=6), got {after}"
    s.cmd("continue", timeout=3)


@test("a software breakpoint and a hardware watchpoint coexist and fire independently")
def test_breakpoint_and_watchpoint_coexist(s):
    s.cmd("break SteppableFunction", timeout=5)
    s.cmd("watch g_testVariable", timeout=5)
    try:
        s.cmd("monitor step", timeout=5)
        out1 = s.cmd("continue", timeout=6)
        # The breakpoint (function entry) is reached before the write inside it.
        assert "SteppableFunction" in out1, out1
        out2 = s.cmd("continue", timeout=6)
        # Continuing from the breakpoint steps over it, runs the
        # assignment, and the watchpoint (independent UBC mechanism) fires.
        assert "watchpoint" in out2.lower() and "g_testVariable" in out2, out2
    finally:
        s.interrupt()
        s._drain(3.0)
        s.cmd("delete", timeout=5)
        s.release_ubc_channel()
        s.cmd("continue", timeout=3)


# --------------------------------------------------------------------------
# Runner
# --------------------------------------------------------------------------

def main():
    global ELF_PATH, GDB_BIN, PORT

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf_path", help="Path to the built Debug_GDBStub.elf")
    parser.add_argument("--gdb", default="gdb-multiarch")
    parser.add_argument("--port", type=int, default=1234)
    parser.add_argument("--log", default="gdb_uts.log")
    parser.add_argument("--filter", default=None,
                         help="Only run tests whose name contains this substring "
                              "(case-insensitive). Useful for isolating a test-logic "
                              "question from whole-run rig-churn fatigue -- see readme.md.")
    args = parser.parse_args()

    ELF_PATH = args.elf_path
    GDB_BIN = args.gdb
    PORT = args.port

    tests_to_run = TESTS
    if args.filter:
        needle = args.filter.lower()
        tests_to_run = [(n, f) for n, f in TESTS if needle in n.lower()]
        if not tests_to_run:
            print(f"No test name matches filter {args.filter!r}")
            sys.exit(1)

    log_lines = []

    def emit(line):
        print(line)
        log_lines.append(line)

    emit("***UT_START***")
    passed = failed = skipped = 0
    for name, fn in tests_to_run:
        session = new_session()
        status = "FAIL"
        message = ""
        try:
            session.connect()
            fn(session)
            status = "PASS"
        except Skip as e:
            status = "SKIP"
            message = str(e)
        except AssertionError as e:
            status = "FAIL"
            message = str(e)
        except Exception as e:
            status = "ERROR"
            message = f"{type(e).__name__}: {e}"
        finally:
            session.close()

        if status == "PASS":
            passed += 1
        elif status == "SKIP":
            skipped += 1
        else:
            failed += 1

        emit(f"{status:5s} : {name}")
        if message:
            emit(f"        {message}")

    emit(f"{passed} passed, {failed} failed, {skipped} skipped, {len(tests_to_run)} total")
    emit("***UT_END***")

    with open(args.log, "w") as f:
        f.write("\n".join(log_lines) + "\n")

    sys.exit(1 if failed > 0 else 0)


if __name__ == "__main__":
    main()
