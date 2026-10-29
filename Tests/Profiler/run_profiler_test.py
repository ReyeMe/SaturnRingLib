#!/usr/bin/env python3
"""Hardware test of the SRL profiler, on "Samples/Debug - Profiler".

Two modes, each building the sample, uploading it and checking the profile against
the workload's known busy times (Samples/Debug - Profiler/src/workload.hpp):
  gdb     pulls g_srl_prof through the GDB stub (srl-prof-dump)
  stream  builds with SRL_PROFILER_REPORT_FRAMES=60, records the reports through ftx -g
          (no GDB), checks the timeline and writes a Perfetto trace

Needs the real rig (ftx, usbreset, gdb-multiarch on PATH, e.g. inside the toolchain
container), like Tests/GDB/run_gdb_tests.sh.

Usage: run_profiler_test.py [--mode gdb|stream|all] [--psu-ip IP] [--seconds N]
"""

import argparse
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
SAMPLE = os.path.join(ROOT, "Samples", "Debug - Profiler")
ELF = os.path.join(SAMPLE, "BuildDrop", "Debug_Profiler.elf")
BIN = os.path.join(SAMPLE, "cd", "data", "0.bin")

sys.path.insert(0, os.path.join(ROOT, "Tests", "GDB"))
sys.path.insert(0, os.path.join(ROOT, "tools", "scripts"))
from gdb_protocol_tests import GdbSession, Rig  # noqa: E402
import srl_prof  # noqa: E402
import srl_prof_capture  # noqa: E402

# Busy ticks per call, from workload.hpp
EXPECTED = {"Calibrate100()": 100, "InnerA()": 50, "InnerB()": 20}
OUTER_OWN = 30
REPORT_FRAMES = 60


class Checks:
    def __init__(self):
        self.failures = []

    def check(self, ok, text):
        print(f"{'PASS' if ok else 'FAIL'} : {text}")
        if not ok:
            self.failures.append(text)

    def ticks(self, name, value, expected):
        """Spin() stops on the first FRT read at or past its target, so a call lasts n to n+1 ticks."""
        self.check(expected - 0.5 <= value <= expected + 1.5,
                   f"{name} = {value:.2f} ticks, expected {expected} (+1.5/-0.5)")


def check_tables(head, funcs, arcs, syms, checks):
    """The calibration checks: known busy times, call counts, nothing dropped."""
    srl_prof.print_flat(head, funcs, arcs, syms, top=20)
    print()

    warning = srl_prof.check_elf(head, syms)
    checks.check(warning is None, warning or "ELF matches the data")

    rows = {r["name"]: r for r in srl_prof.flat_rows(head, funcs, arcs, syms)}
    missing = [n for n in list(EXPECTED) + ["Outer()"] if n not in rows]
    checks.check(not missing, f"functions missing from the profile: {missing}")
    if missing:
        return rows

    for key in ("dropped_funcs", "dropped_arcs", "dropped_depth", "mismatched_exits", "dropped_events"):
        checks.check(head[key] == 0, f"{key} = {head[key]}")
    checks.check(head["frames"] >= 60 and head["hook_cost_q16"] > 0,
                 f"frames = {head['frames']}, hook cost = {srl_prof.hook_cost(head):.3f} ticks")

    outer = rows["Outer()"]
    n = outer["calls"]
    checks.check(rows["InnerA()"]["calls"] == 2 * n and rows["InnerB()"]["calls"] == n
                 and rows["Calibrate100()"]["calls"] == n,
                 f"call counts Outer={n} InnerA={rows['InnerA()']['calls']} InnerB={rows['InnerB()']['calls']} "
                 f"Calibrate100={rows['Calibrate100()']['calls']}")

    for name, ticks in EXPECTED.items():
        checks.ticks(f"{name} per call", rows[name]["total"] / rows[name]["calls"], ticks)
    checks.ticks("Outer() own time per call (hook cost removed)", outer["self"] / n, OUTER_OWN)

    children = (2 * rows["InnerA()"]["total"] / rows["InnerA()"]["calls"]
                + rows["InnerB()"]["total"] / rows["InnerB()"]["calls"])
    checks.ticks("Outer() total - children, per call (hook cost removed)",
                 outer["total"] / n - children - 3 * srl_prof.hook_cost(head), OUTER_OWN)

    print(f"hook pair cost: {srl_prof.hook_cost(head):.3f} ticks = "
          f"{srl_prof.hook_cost(head) * 1e6 / head['tick_hz']:.2f} us")
    return rows


def build(report_frames):
    subprocess.run(["make", f"SRL_PROFILER_REPORT_FRAMES={report_frames}"], cwd=SAMPLE, check=True,
                   stdout=subprocess.DEVNULL)


def run_gdb(args, checks):
    print("=== gdb: pull g_srl_prof through the GDB stub ===")
    build(0)
    dump = os.path.join(HERE, "prof.bin")
    rig = Rig(BIN, args.port, os.path.join(HERE, "ftx_profiler.log"), args.psu_ip)
    session = None
    try:
        rig.reload("profiler gdb")
        session = GdbSession(ELF, port=args.port)
        session.connect()
        session.cmd("continue", timeout=args.seconds)  # returns with the target still running
        stop = session.interrupt_and_wait()
        # Without a live stopped target, GDB would dump the ELF's empty .bss instead.
        if "SIGINT" not in stop:
            raise RuntimeError(f"target did not stop on Ctrl-C: {stop!r}")
        session.cmd(f"source {os.path.join(ROOT, 'tools', 'scripts', 'srl_prof.gdb')}")
        if os.path.exists(dump):
            os.remove(dump)
        print(session.cmd(f"srl-prof-dump {dump}", timeout=120).strip())
    finally:
        if session:
            session.close()
        rig.stop_proxy()

    head, funcs, arcs, _, _ = srl_prof.load(dump)
    check_tables(head, funcs, arcs, srl_prof.Symbols.from_elf(ELF), checks)


def run_stream(args, checks):
    print(f"=== stream: reports every {REPORT_FRAMES} frames through ftx -g, no GDB ===")
    build(REPORT_FRAMES)
    capture = os.path.join(HERE, "capture.bin")
    rig = Rig(BIN, args.port, os.path.join(HERE, "ftx_profiler.log"), args.psu_ip)
    try:
        rig.reload("profiler stream")
        data = srl_prof_capture.capture("localhost", args.port, args.seconds)
    finally:
        rig.stop_proxy()
    with open(capture, "wb") as f:
        f.write(data)
    print(f"captured {len(data)} bytes")

    head, funcs, arcs, events, stats = srl_prof.parse_stream(data)
    syms = srl_prof.Symbols.from_elf(ELF)
    check_tables(head, funcs, arcs, syms, checks)

    # Reports sent before the capture connected are cut short or lost: only count what arrived.
    expected_reports = int(args.seconds * 60 / REPORT_FRAMES) - 2
    checks.check(stats["reports"] >= expected_reports,
                 f"{stats['reports']} complete reports in {args.seconds:.0f} s (expected >= {expected_reports}); "
                 f"{stats['bad_packets']} bad packets, {stats['skipped_bytes']} bytes outside packets")

    ms_per_tick = 1000.0 / head["tick_hz"]
    if head["report_ticks"]:
        ms = head["report_ticks"] * ms_per_tick
        print(f"FT245 throughput: {head['report_bytes']} bytes in {ms:.2f} ms "
              f"= {head['report_bytes'] / ms:.0f} KB/s")
    checks.check(head["report_ticks"] > 0, "report send time measured")

    items = srl_prof.timeline(events, syms)
    vblanks = [t for t, kind, name in items if kind == srl_prof.EVENT_INSTANT and name == "VBlank"]
    checks.check(len(vblanks) >= 60, f"{len(vblanks)} VBlank marks (>= 60 frames)")
    if len(vblanks) >= 2:
        gaps = [(b - a) * ms_per_tick for a, b in zip(vblanks, vblanks[1:])]
        mean = sum(gaps) / len(gaps)
        checks.check(16.4 <= mean <= 17.0, f"VBlank every {mean:.3f} ms on average (NTSC 16.68)")

    # The "Workload" scope wraps RunWorkload(): same time, plus one hook pair, plus the
    # cold-cache misses of the first profiler code after the frame sync (up to ~2 ticks).
    begins = {}
    slices = []
    for ticks, kind, name in items:
        if kind == srl_prof.EVENT_BEGIN:
            begins[name] = ticks
        elif kind == srl_prof.EVENT_END and name in begins:
            slices.append((name, ticks - begins.pop(name)))
    workload = [d for name, d in slices if name == "Workload"]
    rows = {r["name"]: r for r in srl_prof.flat_rows(head, funcs, arcs, syms)}
    checks.check(len(workload) >= 60, f"{len(workload)} Workload slices")
    if workload and "RunWorkload()" in rows:
        run = rows["RunWorkload()"]["total"] / rows["RunWorkload()"]["calls"]
        mean = sum(workload) / len(workload)
        checks.check(run <= mean <= run + srl_prof.hook_cost(head) + 3,
                     f"Workload slice {mean:.2f} ticks vs RunWorkload() {run:.2f} ticks + one hook pair (+3)")
    reports = [d for name, d in slices if name == "SRL profiler report"]
    checks.check(len(reports) >= expected_reports - 1, f"{len(reports)} report slices on the timeline")

    trace_path = os.path.join(HERE, "trace.json")
    trace = srl_prof.perfetto_trace(head, events, syms)
    with open(trace_path, "w") as f:
        json.dump(trace, f)
    frames = sum(1 for e in trace["traceEvents"] if e.get("name") == "VBlank")
    checks.check(frames >= 60, f"Perfetto trace {trace_path}: {frames} frames, {len(trace['traceEvents'])} events")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--mode", choices=("gdb", "stream", "all"), default="all")
    parser.add_argument("--psu-ip", help="network PSU used to power-cycle the console before upload")
    parser.add_argument("--seconds", type=float, default=6.0, help="how long to let the sample run")
    parser.add_argument("--port", type=int, default=1234)
    args = parser.parse_args()

    checks = Checks()
    if args.mode in ("gdb", "all"):
        run_gdb(args, checks)
    if args.mode in ("stream", "all"):
        run_stream(args, checks)

    print("ALL PASS" if not checks.failures else f"FAILED ({len(checks.failures)}):")
    for failure in checks.failures:
        print(f"  {failure}")
    return 1 if checks.failures else 0


if __name__ == "__main__":
    sys.exit(main())
