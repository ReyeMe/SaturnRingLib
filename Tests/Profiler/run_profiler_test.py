#!/usr/bin/env python3
"""Hardware test of the SRL profiler (milestone 1): calibration and own/total time.

Builds "Samples/Debug - Profiler", uploads it, lets it run, pulls g_srl_prof through
the GDB stub and checks the profile against the workload's known busy times
(Samples/Debug - Profiler/src/workload.hpp).

Needs the real rig (ftx, usbreset, gdb-multiarch on PATH, e.g. inside the toolchain
container), like Tests/GDB/run_gdb_tests.sh.

Usage: run_profiler_test.py [--psu-ip IP] [--seconds N]
"""

import argparse
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

# Busy ticks per call, from workload.hpp
EXPECTED = {"Calibrate100()": 100, "InnerA()": 50, "InnerB()": 20}
OUTER_OWN = 30


def within(name, value, expected, failures):
    """Spin() stops on the first FRT read at or past its target, so a call lasts n to n+1 ticks."""
    ok = expected - 0.5 <= value <= expected + 1.5
    print(f"{'PASS' if ok else 'FAIL'} : {name} = {value:.2f} ticks, expected {expected} (+1.5/-0.5)")
    if not ok:
        failures.append(name)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--psu-ip", help="network PSU used to power-cycle the console before upload")
    parser.add_argument("--seconds", type=float, default=5.0, help="how long to let the sample run")
    parser.add_argument("--port", type=int, default=1234)
    parser.add_argument("--dump", default=os.path.join(HERE, "prof.bin"))
    args = parser.parse_args()

    subprocess.run(["make"], cwd=SAMPLE, check=True)

    rig = Rig(BIN, args.port, os.path.join(HERE, "ftx_profiler.log"), args.psu_ip)
    session = None
    try:
        rig.reload("profiler")
        session = GdbSession(ELF, port=args.port)
        session.connect()
        session.cmd("continue", timeout=args.seconds)  # returns with the target still running
        stop = session.interrupt_and_wait()
        # Without a live stopped target, GDB would dump the ELF's empty .bss instead.
        if "SIGINT" not in stop:
            raise RuntimeError(f"target did not stop on Ctrl-C: {stop!r}")
        session.cmd(f"source {os.path.join(ROOT, 'tools', 'scripts', 'srl_prof.gdb')}")
        if os.path.exists(args.dump):
            os.remove(args.dump)
        print(session.cmd(f"srl-prof-dump {args.dump}", timeout=120).strip())
    finally:
        if session:
            session.close()
        rig.stop_proxy()

    with open(args.dump, "rb") as f:
        head, funcs, arcs = srl_prof.parse_block(f.read())
    syms = srl_prof.Symbols.from_elf(ELF)
    srl_prof.print_flat(head, funcs, arcs, syms, top=20)
    print()

    failures = []
    warning = srl_prof.check_elf(head, syms)
    if warning:
        print(f"FAIL : {warning}")
        failures.append("elf")

    rows = {r["name"]: r for r in srl_prof.flat_rows(head, funcs, arcs, syms)}
    outer = rows.get("Outer()")
    missing = [n for n in list(EXPECTED) + ["Outer()"] if n not in rows]
    if missing:
        print(f"FAIL : functions missing from the profile: {missing}")
        return 1

    for key in ("dropped_funcs", "dropped_arcs", "dropped_depth", "mismatched_exits"):
        if head[key]:
            print(f"FAIL : {key} = {head[key]}")
            failures.append(key)

    if head["frames"] < 60 or head["hook_cost_q16"] == 0:
        print(f"FAIL : frames = {head['frames']}, hook cost = {srl_prof.hook_cost(head):.3f} ticks")
        failures.append("header")

    n = outer["calls"]
    counts_ok = (rows["InnerA()"]["calls"] == 2 * n and rows["InnerB()"]["calls"] == n
                 and rows["Calibrate100()"]["calls"] == n)
    print(f"{'PASS' if counts_ok else 'FAIL'} : call counts Outer={n} InnerA={rows['InnerA()']['calls']} "
          f"InnerB={rows['InnerB()']['calls']} Calibrate100={rows['Calibrate100()']['calls']}")
    if not counts_ok:
        failures.append("calls")

    for name, ticks in EXPECTED.items():
        within(f"{name} per call", rows[name]["total"] / rows[name]["calls"], ticks, failures)
    within("Outer() own time per call (hook cost removed)", outer["self"] / n, OUTER_OWN, failures)

    children = 2 * rows["InnerA()"]["total"] / rows["InnerA()"]["calls"] + \
        rows["InnerB()"]["total"] / rows["InnerB()"]["calls"]
    within("Outer() total - children, per call (hook cost removed)",
           outer["total"] / n - children - 3 * srl_prof.hook_cost(head), OUTER_OWN, failures)

    print(f"hook pair cost: {srl_prof.hook_cost(head):.3f} ticks = "
          f"{srl_prof.hook_cost(head) * 1e6 / head['tick_hz']:.2f} us")
    print("ALL PASS" if not failures else f"FAILED: {failures}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
