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
import urllib.request

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

    PROMPT = re.compile(r"\(gdb\) ")

    def _read_until(self, pattern, timeout):
        """Reads until `pattern` appears in the output, or `timeout` seconds pass."""
        deadline = time.time() + timeout
        chunk = ""
        while time.time() < deadline:
            try:
                data = os.read(self.master_fd, 65536)
            except (BlockingIOError, OSError):
                data = None
            if data:
                chunk += data.decode(errors="replace")
                if pattern.search(chunk):
                    break
            else:
                time.sleep(0.02)
        return chunk

    def cmd(self, text, timeout=5.0):
        """Runs a CLI command and returns its output once gdb shows its prompt again.

        Waiting for the prompt, not for a quiet spell: `continue` prints
        "Continuing." at once and then nothing until the target stops, so an
        idle-based read returned before any breakpoint could be reported. A
        command that leaves the target running returns after `timeout`.
        """
        self._read_until(re.compile(r"$^"), 0)  # discard stale output so an old prompt can't end this read
        self.send(text)
        return self._read_until(self.PROMPT, timeout)

    def interrupt_and_wait(self, timeout=6.0):
        """Ctrl-C a running target and return gdb's stop output."""
        self.interrupt()
        return self._read_until(self.PROMPT, timeout)

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


class Rig:
    """Puts the target into a known state before each test.

    Power-cycle (optional network PSU), USB reset, upload + run the sample,
    then a fresh `ftx -g` proxy -- any usbreset invalidates an already-running
    proxy's FTDI handle. Reloading before every test is what makes the tests
    independent: crash triggers never return, and UBC/slave/breakpoint state
    would otherwise leak from one test into the next.
    """

    LOAD_ADDR = "0x06004000"
    POWER_CYCLES = 2
    UPLOAD_ATTEMPTS = 3

    def __init__(self, bin_path, port, proxy_log, psu_ip=None):
        self.bin_path = bin_path
        self.port = port
        self.proxy_log = proxy_log
        self.psu_ip = psu_ip
        self.proxy = None
        open(self.proxy_log, "w").close()

    def _psu(self, action):
        req = urllib.request.Request(f"http://{self.psu_ip}/api/v1/{action}", method="POST")
        urllib.request.urlopen(req, timeout=5).read()

    def _power_cycle(self):
        self._psu("off")
        time.sleep(6)   # the PSU rejects a second command within its ~5s latch
        self._psu("on")
        time.sleep(12)  # BIOS boot + DevCart settle

    def stop_proxy(self):
        if self.proxy and self.proxy.poll() is None:
            self.proxy.kill()
            self.proxy.wait()
        self.proxy = None

    def reload(self, label):
        self.stop_proxy()
        attempts = 0
        output = ""
        uploaded = False
        for _ in range(self.POWER_CYCLES):
            if self.psu_ip:
                self._power_cycle()
            # The first upload after a power-cycle routinely fails ("usb bulk write
            # failed"); a fresh usbreset and a retry fixes it.
            for _ in range(self.UPLOAD_ATTEMPTS):
                attempts += 1
                subprocess.run(["usbreset", "FT245R USB FIFO"], capture_output=True)
                time.sleep(2)
                r = subprocess.run(["ftx", "-x", self.bin_path, self.LOAD_ADDR],
                                   capture_output=True, text=True, timeout=120)
                output = r.stdout + r.stderr
                if r.returncode == 0 and "upload failed" not in output.lower():
                    uploaded = True
                    break
            if uploaded:
                break
        if not uploaded:
            raise RuntimeError(f"rig: upload failed {attempts} times: {output[-300:]!r}")
        log = open(self.proxy_log, "a")
        log.write(f"[rig] reloaded ({attempts} upload attempts) for: {label}\n")
        log.flush()
        self.proxy = subprocess.Popen(["ftx", "-g", str(self.port), "-v"],
                                      stdout=log, stderr=subprocess.STDOUT)
        log.close()
        time.sleep(2)  # proxy listening


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


# Every test runs on a freshly booted target (see Rig) and checks one thing.
# Tests named "scenario: ..." chain several debugger operations on purpose.

STOPPED_IN = "SteppableFunction"


def _hit_steppable(s):
    """Arms a breakpoint on SteppableFunction, triggers it, and asserts the stop."""
    s.cmd("break SteppableFunction", timeout=5)
    s.cmd("monitor step", timeout=5)
    out = s.cmd("continue", timeout=10)
    assert STOPPED_IN in out, f"breakpoint on SteppableFunction not hit: {out!r}"
    return out


# --------------------------------------------------------------------------
# Protocol negotiation (raw RSP packets)
# --------------------------------------------------------------------------

@test("handshake: connect + info registers shows the full SH-2 register set")
def test_handshake(s):
    out = s.cmd("info registers", timeout=5)
    for reg in ["r0", "r15", "pc", "pr", "gbr", "mach", "macl"]:
        assert re.search(rf"\b{reg}\b", out), f"register {reg} missing: {out!r}"


@test("raw qSupported advertises PacketSize/swbreak/hwbreak")
def test_raw_qsupported(s):
    reply = s.packet("qSupported")
    assert "PacketSize=" in reply, reply
    assert "swbreak" in reply, reply
    assert "hwbreak" in reply, reply


@test("raw qSupported PacketSize (hex) never lets GDB ask for reads the stub rejects")
def test_raw_qsupported_packet_size(s):
    reply = s.packet("qSupported")
    size = int(re.search(r"PacketSize=([0-9a-fA-F]+)", reply).group(1), 16)
    # GDB reads PacketSize as hex and sizes 'm' reads to PacketSize/2 bytes; the stub caps 'm' at 511.
    assert size // 2 <= 511, f"PacketSize={size:#x} lets GDB request reads the stub rejects"


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
    assert "Text=0" in s.packet("qOffsets")


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
    assert re.fullmatch(r"[ST][0-9a-fA-F]{2}.*", s.packet("?"))


@test("raw ? on a fresh attach reports SIGTRAP, never a stale crash signal")
def test_raw_stop_query_is_sigtrap(s):
    reply = s.packet("?")
    assert reply.startswith("T05"), f"fresh attach should report SIGTRAP: {reply!r}"


# --------------------------------------------------------------------------
# Malformed / unsupported packets: each gets its defined reply, stub stays alive
# --------------------------------------------------------------------------

MALFORMED_PACKETS = [
    ("an unsupported command", "@", ""),
    ("Z with a bad type digit", "Z9,6004510,2", ""),
    ("Z missing ',addr,kind'", "Z0", ""),
    ("m missing ','", "m6004510", "E01"),
    ("M missing ':'", "M6004510,4", "E02"),
    ("z0 of a never-set breakpoint", "z0,6100000,2", "OK"),
    ("m of SH-2 peripheral space", "mfffffff0,4", "E01"),
    ("M of SH-2 peripheral space", "Mfffffff0,4:11223344", "E02"),
]


def _make_malformed_test(payload, expected):
    def _t(s):
        reply = s.packet(payload)
        assert reply == expected, f"{payload!r}: expected {expected!r}, got {reply!r}"
        assert s.packet("qC") == "QC1", "stub stopped answering after the malformed packet"
    return _t


for _what, _payload, _expected in MALFORMED_PACKETS:
    TESTS.append((f"malformed: {_what} gets {_expected or 'an empty reply'!s}",
                  _make_malformed_test(_payload, _expected)))


# --------------------------------------------------------------------------
# Registers
# --------------------------------------------------------------------------

@test("raw g returns the full register set as a hex string")
def test_raw_g(s):
    reply = s.packet("g")
    assert len(reply) >= 160, reply
    assert re.fullmatch(r"[0-9a-fA-F]+", reply), reply


@test("p $pc resolves to code within the loaded ROM image")
def test_pc_sane(s):
    pc = parse_hex(s.cmd("p/x $pc", timeout=5))
    assert 0x06000000 <= pc <= 0x06200000, hex(pc)


@test("P (write single register) round-trips via CLI set/print")
def test_register_write_roundtrip(s):
    s.cmd("p $r0 = 0xdeadbeef", timeout=5)
    written = parse_hex(s.cmd("p/x $r0", timeout=5))
    assert written == 0xdeadbeef, hex(written)


@test("G (write all registers) round-trips byte-for-byte via raw packet")
def test_raw_g_write_roundtrip(s):
    before = s.packet("g")
    assert s.packet("G" + before) == "OK"
    assert s.packet("g") == before


# --------------------------------------------------------------------------
# Memory
# --------------------------------------------------------------------------

@test("m reads a known global's live value, matching what the CLI sees")
def test_memory_read_known_global(s):
    addr = parse_hex(s.cmd("p &g_testVariable", timeout=5))
    cli_val = parse_hex(s.cmd("p/x g_testVariable", timeout=5))
    raw_val = int(s.packet(f"m{addr:x},4"), 16)
    assert raw_val == (cli_val & 0xffffffff), f"cli={cli_val:#x} raw={raw_val:#x}"


@test("GDB reads a 600-byte block in one command (PacketSize regression)")
def test_large_memory_read(s):
    out = s.cmd("x/600xb 0x06004000", timeout=15)
    assert "Cannot access memory" not in out, out


@test("M writes an aligned 32-bit value and reads it back exactly")
def test_memory_write_aligned(s):
    addr = parse_hex(s.cmd("p &g_testVariable", timeout=5))
    assert s.packet(f"M{addr:x},4:11223344") == "OK"
    assert s.packet(f"m{addr:x},4") == "11223344"


@test("M writes an unaligned length/offset without corrupting surrounding bytes (hex2mem_aligned)")
def test_memory_write_unaligned(s):
    addr = parse_hex(s.cmd("p &g_testVariable", timeout=5)) + 1
    assert s.packet(f"M{addr:x},3:aabbcc") == "OK"
    assert s.packet(f"m{addr:x},3") == "aabbcc"


@test("M write to VDP2 CRAM lands the full value (regression: byte-wise hex2mem CRAM bug)")
def test_memory_write_cram_hw_regression(s):
    addr = 0x25F00042
    assert s.packet(f"M{addr:x},2:1234") == "OK"
    readback = s.packet(f"m{addr:x},2")
    assert readback == "1234", (
        f"CRAM write corrupted: wrote 1234, read back {readback!r} -- "
        "a dropped high byte is exactly the pre-fix hex2mem() bug"
    )


# --------------------------------------------------------------------------
# Breakpoints, watchpoints, stepping
# --------------------------------------------------------------------------

@test("software breakpoint: set, hit")
def test_software_breakpoint(s):
    _hit_steppable(s)


@test("software breakpoint: deleted before it is reached never fires")
def test_breakpoint_removed_before_hit(s):
    s.cmd("break SteppableFunction", timeout=5)
    s.cmd("delete", timeout=5)
    s.cmd("monitor step", timeout=5)
    out = s.cmd("continue", timeout=6)
    assert not re.search(r"Breakpoint|SteppableFunction", out), f"deleted breakpoint still fired: {out!r}"


@test("hardware breakpoint (Z1/hbreak, distinct from the software Z0 path) hits")
def test_hardware_breakpoint(s):
    s.cmd("hbreak SteppableFunction", timeout=5)
    s.cmd("monitor step", timeout=5)
    out = s.cmd("continue", timeout=10)
    assert STOPPED_IN in out, out


def _watchpoint_test(kind, needs_touch):
    def _t(s):
        s.cmd(f"{kind} g_testVariable", timeout=5)
        if needs_touch:
            s.cmd("monitor touch", timeout=5)
        out = s.cmd("continue", timeout=10)
        assert "watchpoint" in out.lower() and "g_testVariable" in out, out
    return _t


TESTS.append(("hardware watchpoint (write) fires on g_testVariable",
              _watchpoint_test("watch", needs_touch=True)))
TESTS.append(("hardware watchpoint (read) fires on the main loop's read of g_testVariable",
              _watchpoint_test("rwatch", needs_touch=False)))
TESTS.append(("hardware watchpoint (access) fires on g_testVariable",
              _watchpoint_test("awatch", needs_touch=True)))


@test("single-step walks SteppableFunction's statements to the expected result")
def test_single_step(s):
    _hit_steppable(s)
    # Step line by line until the line after the store; how many lines that takes
    # depends on where GDB placed the breakpoint (the '{' or the first statement).
    for _ in range(8):
        if "Steppable result" in s.cmd("next", timeout=15):
            break
    else:
        raise AssertionError("never reached the Print() line after the store within 8 'next'")
    value = parse_hex(s.cmd("p/x g_testVariable", timeout=5))
    assert value == 6, f"expected g_testVariable == 6 (a=1,b=2,c=3,d=6), got {value}"


# --------------------------------------------------------------------------
# Exception vectors: one trigger per freshly booted target (they never return)
# --------------------------------------------------------------------------

CRASH_TRIGGERS = [
    "illegal", "reserved", "slotillegal", "slotreserved",
    "genillegal", "dma", "ubc", "trapa3",
]


def _make_crash_test(kind):
    def _t(s):
        s.cmd(f"monitor crash {kind}", timeout=5)
        out = s.cmd("continue", timeout=10)
        # "received signal", not bare "signal": GDB's "Can't send signals to this
        # remote system" warning (a hung resume) must not count as a halt.
        assert re.search(r"received signal|breakpoint|0x[0-9a-fA-F]+ in", out, re.IGNORECASE), (
            f"'monitor crash {kind}' did not appear to halt the target: {out!r}"
        )
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
# Slave CPU
# --------------------------------------------------------------------------

def _assert_slave_breakpoint_hit(s, out):
    # A slave breakpoint is reported the same way as Ctrl-C: SIGINT, with the
    # master frame in SRL::GDBStub::Break() (Poll() bridges the slave's stop).
    # The real confirmation is `monitor regs slave`'s live pc.
    assert re.search(r"SIGINT|Interrupt", out, re.IGNORECASE), (
        f"expected the documented SIGINT/Break() stop reply, got: {out!r}"
    )
    regs = s.cmd("monitor regs slave", timeout=5)
    m = re.search(r"pc=([0-9a-fA-F]+)", regs)
    assert m, regs
    pc = int(m.group(1), 16)
    assert 0x06000000 <= pc <= 0x06200000, f"slave pc {pc:#x} outside expected ROM range: {regs!r}"
    return pc


@test("slave breakpoint hits and the slave stops exactly on it")
def test_slave_breakpoint(s):
    s.cmd("break SlaveCounterTask::Do", timeout=5)
    # 'break' output can arrive mixed with leftover attach output; 'info breakpoints' is clean.
    bp_info = re.sub(r"\x1b\[[0-9;?]*[A-Za-z]", "", s.cmd("info breakpoints", timeout=5))  # drop GDB's color codes
    bp_addr = re.search(r"breakpoint\s+keep\s+y\s+(0x[0-9a-fA-F]+)", bp_info)
    assert bp_addr, "could not read the breakpoint address from 'info breakpoints'"
    pc = _assert_slave_breakpoint_hit(s, s.cmd("continue", timeout=10))
    # On the breakpoint, not 2 bytes past it: the restored instruction must still run on resume.
    assert pc == int(bp_addr.group(1), 16), f"slave pc {pc:#x} != breakpoint {bp_addr.group(1)}"


# --------------------------------------------------------------------------
# Monitor commands, timeouts
# --------------------------------------------------------------------------

@test("monitor trace reports the halt-context snapshot")
def test_monitor_trace(s):
    out = s.cmd("monitor trace", timeout=5)
    # Fields send_halt_trace_dump() always prints.
    assert "pc=" in out and "last_stop_signal=" in out and "opcode_at_pc=" in out, out


@test("an idle connected client (no commands for several seconds) is still served normally")
def test_idle_client_then_command(s):
    time.sleep(6.0)
    parse_hex(s.cmd("p/x $pc", timeout=5))


@test("a long free-running continue can still be interrupted after several seconds")
def test_long_running_continue_then_interrupt(s):
    s.cmd("continue", timeout=1.0)
    time.sleep(6.0)
    out = s.interrupt_and_wait()
    assert re.search(r"received signal|SIGINT", out, re.IGNORECASE), (
        f"Ctrl-C after a long free-run did not halt the target: {out!r}"
    )


# --------------------------------------------------------------------------
# Scenarios: several debugger operations chained in one session
# --------------------------------------------------------------------------

@test("scenario: two software breakpoints hit in the expected order")
def test_two_software_breakpoints(s):
    s.cmd("break HandleMonitorCommand", timeout=5)
    s.cmd("break SteppableFunction", timeout=5)
    s.cmd("monitor step", timeout=5)
    out1 = s.cmd("continue", timeout=10)
    assert "HandleMonitorCommand" in out1, out1
    out2 = s.cmd("continue", timeout=10)
    assert "SteppableFunction" in out2, out2


@test("scenario: the same breakpoint hits again on each of three triggers")
def test_breakpoint_rehit(s):
    s.cmd("break SteppableFunction", timeout=5)
    for i in range(3):
        s.cmd("monitor step", timeout=5)
        out = s.cmd("continue", timeout=10)
        assert STOPPED_IN in out, f"hit {i + 1} of 3 missing: {out!r}"


@test("scenario: continue, then step, then continue runs free")
def test_mixed_resume_types(s):
    _hit_steppable(s)
    s.cmd("step", timeout=10)
    out = s.cmd("continue", timeout=6)
    assert not re.search(r"Breakpoint|signal", out, re.IGNORECASE), out


@test("scenario: finish returns from SteppableFunction to its caller with the result stored")
def test_finish(s):
    _hit_steppable(s)
    out = s.cmd("finish", timeout=15)
    assert "HandleMonitorCommand" in out, f"finish did not return to the caller: {out!r}"
    assert parse_hex(s.cmd("p/x g_testVariable", timeout=5)) == 6


@test("scenario: nexti through SteppableFunction's return never reports a fake SIGILL")
def test_nexti_through_return(s):
    # Steps every delayed branch (jsr/rts) in the function: the step trap sits in a delay slot.
    _hit_steppable(s)
    s.cmd("delete", timeout=5)
    for _ in range(120):
        out = s.cmd("nexti", timeout=10)
        assert "SIGILL" not in out, f"single-step reported a fake crash: {out!r}"
        if "HandleMonitorCommand" in out:
            return
    raise AssertionError("never left SteppableFunction within 120 nexti")


@test("scenario: a conditional breakpoint whose condition is true stops")
def test_conditional_breakpoint_true(s):
    s.cmd("break SteppableFunction if g_testVariable == 0", timeout=5)  # 0 after a fresh boot
    s.cmd("monitor step", timeout=5)
    assert STOPPED_IN in s.cmd("continue", timeout=10)


@test("scenario: a conditional breakpoint whose condition is false lets the program run through")
def test_conditional_breakpoint_false(s):
    s.cmd("break SteppableFunction if g_testVariable == 12345", timeout=5)
    s.cmd("monitor step", timeout=5)
    out = s.cmd("continue", timeout=6)
    assert STOPPED_IN not in out, f"false condition still stopped: {out!r}"
    s.interrupt_and_wait()
    assert parse_hex(s.cmd("p/x g_testVariable", timeout=5)) == 6, "SteppableFunction did not run through"


@test("scenario: a variable written while stopped is what the running program sees")
def test_write_visible_after_resume(s):
    s.cmd("set var g_testVariable = 41", timeout=5)
    s.cmd("monitor touch", timeout=5)  # g_testVariable += 1 once the main loop resumes
    s.cmd("continue", timeout=3)
    s.interrupt_and_wait()
    value = parse_hex(s.cmd("p/x g_testVariable", timeout=5))
    assert value == 42, f"expected 41 + 1, got {value} (stale cache line after the write?)"


@test("scenario: only the LAST monitor command queued while stopped takes effect")
def test_monitor_only_last_queued_command_wins(s):
    s.cmd("monitor touch", timeout=5)
    s.cmd("monitor step", timeout=5)
    s.cmd("continue", timeout=3)
    s.interrupt_and_wait()
    after = parse_hex(s.cmd("p/x g_testVariable", timeout=5))
    assert after == 6, f"expected the queued 'step' to win (g_testVariable=6), got {after}"


@test("scenario: Ctrl-C, backtrace, then a breakpoint still hits")
def test_interrupt_bt_then_breakpoint(s):
    s.cmd("continue", timeout=2)
    assert re.search(r"SIGINT|received signal", s.interrupt_and_wait(), re.IGNORECASE)
    # Unwinding through SGL's no-debug-info interrupt code costs GDB hundreds of 2-byte reads.
    assert "(gdb)" in s.cmd("bt", timeout=60), "bt did not return"
    _hit_steppable(s)


@test("scenario: after a reserved-instruction crash, skipping it lets the program run on")
def test_crash_recover(s):
    s.cmd("monitor crash reserved", timeout=5)
    assert re.search(r"received signal", s.cmd("continue", timeout=10)), "crash did not halt"
    s.cmd("set var $pc = $pc + 2", timeout=5)  # step over the 2-byte reserved opcode
    _hit_steppable(s)  # resumes with "C04" (GDB passes SIGILL); the main loop must still run


@test("scenario: a software breakpoint and a hardware watchpoint coexist and fire independently")
def test_breakpoint_and_watchpoint_coexist(s):
    s.cmd("watch g_testVariable", timeout=5)
    _hit_steppable(s)
    out = s.cmd("continue", timeout=10)
    assert "watchpoint" in out.lower() and "g_testVariable" in out, out


@test("scenario: after a slave breakpoint stop, master breakpoints still hit")
def test_slave_then_master(s):
    s.cmd("break SlaveCounterTask::Do", timeout=5)
    _assert_slave_breakpoint_hit(s, s.cmd("continue", timeout=10))
    s.cmd("delete", timeout=5)
    _hit_steppable(s)  # the slave's release must not leave the main loop wedged


@test("scenario: slave breakpoint re-arms after detach (SlaveReleaseGuard regression)")
def test_slave_breakpoint_reconnect_regression(s):
    s.cmd("break SlaveCounterTask::Do", timeout=5)
    _assert_slave_breakpoint_hit(s, s.cmd("continue", timeout=10))
    s.cmd("detach", timeout=5)

    s2 = new_session()
    try:
        s2.connect()
        s2.cmd("break SlaveCounterTask::Do", timeout=5)
        _assert_slave_breakpoint_hit(s2, s2.cmd("continue", timeout=10))
    finally:
        s2.close()


@test("scenario: detach leaves the stub healthy for an immediate, independent reconnect")
def test_detach_then_reconnect(s):
    s.cmd("detach", timeout=5)
    s2 = new_session()
    try:
        s2.connect(timeout=8)
        parse_hex(s2.cmd("p &g_testVariable", timeout=5))
    finally:
        s2.close()


@test("scenario: raw k (kill) leaves the stub healthy for an immediate, independent reconnect")
def test_kill_then_reconnect(s):
    try:
        s.packet("k", timeout=3)
    except RuntimeError:
        pass  # 'k' has no defined reply per the RSP spec; gdb may not print one.
    s2 = new_session()
    try:
        s2.connect(timeout=8)
        parse_hex(s2.cmd("p &g_testVariable", timeout=5))
    finally:
        s2.close()


@test("scenario: an abrupt disconnect (client just vanishes, no D/k) still leaves the stub healthy")
def test_dirty_disconnect_then_reconnect(s):
    s.send("continue")
    time.sleep(0.5)
    s.kill_dirty()
    s2 = new_session()
    try:
        s2.connect()
        parse_hex(s2.cmd("p/x $pc", timeout=5))
    finally:
        s2.close()


@test("scenario: a breakpoint left armed by a killed GDB is gone when the next session connects")
def test_stale_breakpoint_after_kill(s):
    s.cmd("break SteppableFunction", timeout=5)
    bp_info = re.sub(r"\x1b\[[0-9;?]*[A-Za-z]", "", s.cmd("info breakpoints", timeout=5))
    bp_addr = int(re.search(r"breakpoint\s+keep\s+y\s+(0x[0-9a-fA-F]+)", bp_info).group(1), 16)
    s.send("continue")  # running, with the 0xFFFF patched into SteppableFunction
    time.sleep(1.0)
    s.kill_dirty()
    s2 = new_session()
    try:
        s2.connect()
        # Checked in memory, not by watching for a stop: GDB silently resumes a
        # swbreak at an address it never set, so the stale patch fires unseen.
        code = s2.packet(f"m{bp_addr:x},2")
        assert code != "ffff", f"the dead session's breakpoint is still patched in at {bp_addr:#x}"
    finally:
        s2.close()


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
    parser.add_argument("--bin", default=None,
                        help="Built cd/data/0.bin: when given, the target is reloaded "
                             "(uploaded, plus a fresh ftx -g proxy) before every test")
    parser.add_argument("--psu-ip", default=None,
                        help="Network PSU (relay_status/on/off API) used to power-cycle "
                             "the console before each upload")
    parser.add_argument("--proxy-log", default="ftx_gdb.log",
                        help="Where the ftx -g proxy's traffic log is written (with --bin)")
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

    rig = Rig(args.bin, PORT, args.proxy_log, args.psu_ip) if args.bin else None

    emit("***UT_START***")
    passed = failed = skipped = 0
    for name, fn in tests_to_run:
        session = None
        status = "FAIL"
        message = ""
        try:
            if rig:
                rig.reload(name)
            session = new_session()
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
            if session:
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

    if rig:
        rig.stop_proxy()

    emit(f"{passed} passed, {failed} failed, {skipped} skipped, {len(tests_to_run)} total")
    emit("***UT_END***")

    with open(args.log, "w") as f:
        f.write("\n".join(log_lines) + "\n")

    sys.exit(1 if failed > 0 else 0)


if __name__ == "__main__":
    main()
