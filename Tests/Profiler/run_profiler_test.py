#!/usr/bin/env python3
"""Hardware test of the SRL profiler, on "Samples/Debug - Profiler".

Each mode builds the sample, uploads it and checks the profile against the workload's
known busy times (Samples/Debug - Profiler/src/workload.hpp), on both SH-2s:
  gdb      pulls g_srl_prof through the GDB stub (srl-prof-dump)
  stream   builds with SRL_PROFILER_REPORT_FRAMES=60, records the reports through ftx -g
           (no GDB), checks the timeline, writes a Perfetto trace and a gmon.out
  sampler  also builds with SRL_PROFILER_SAMPLE_HZ=16000: checks the sample rate, the
           functions' sample shares, and that sh-elf-gprof reads the sampled gmon.out
  vdp1     same build, 15 s through the sample's VDP1 load steps (0 to 16 full-screen
           polygons): checks VDP1 draw times grow with the load, missed frames, the
           costliest VDP1 command, SCU DMA marks, and the VDP1 track of the Perfetto trace

Needs the real rig (ftx, usbreset, gdb-multiarch, sh-elf-gprof on PATH, e.g. inside the
toolchain container), like Tests/GDB/run_gdb_tests.sh.

Usage: run_profiler_test.py [--mode gdb|stream|sampler|vdp1|all] [--psu-ip IP] [--seconds N]
"""

import argparse
import json
import math
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

# Busy ticks per frame, from workload.hpp
EXPECTED = {"Calibrate100()": 100, "InnerA()": 50, "InnerB()": 20}
OUTER_OWN = 30
SLAVE_WORK = 40
REPORT_FRAMES = 60
SAMPLE_HZ = 16000


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


def check_tables(profile, syms, checks, calibrate=True, streamed=True):
    """Known busy times and call counts on both CPUs, nothing dropped.

    calibrate=False skips the exact-time checks (the sampler's interrupts add to them).
    streamed=False: without reports nothing drains the event buffer, so it fills up and
    drops events after about 4096 / 7 = 585 frames; only the tables are checked then.
    """
    head = profile["head"]
    srl_prof.print_flat(profile, syms, top=20)
    print()

    warning = srl_prof.check_elf(head, syms)
    checks.check(warning is None, warning or "ELF matches the data")

    rows = {r["name"]: r for r in srl_prof.flat_rows(head, profile["master"]["funcs"], profile["master"]["arcs"], syms)}
    slave = {r["name"]: r for r in srl_prof.flat_rows(head, profile["slave"]["funcs"], profile["slave"]["arcs"], syms)}
    missing = [n for n in list(EXPECTED) + ["Outer()"] if n not in rows] + \
              [f"{n} (slave)" for n in ("SlaveWork()",) if n not in slave]
    checks.check(not missing, f"functions missing from the profile: {missing}")
    if missing:
        return rows

    for cpu in srl_prof.CPUS:
        for key in srl_prof.CPU_FIELDS:
            checks.check(profile[cpu][key] == 0, f"{cpu} {key} = {profile[cpu][key]}")
    if streamed:
        checks.check(head["dropped_events"] == 0, f"dropped_events = {head['dropped_events']}")
    checks.check(head["frames"] >= 60 and head["hook_cost_q16"] > 0 and head["slave_ready"] == 1,
                 f"frames = {head['frames']}, hook cost = {srl_prof.hook_cost(head):.3f} ticks, "
                 f"slave ready = {head['slave_ready']}")

    outer = rows["Outer()"]
    n = outer["calls"]
    checks.check(rows["InnerA()"]["calls"] == 2 * n and rows["InnerB()"]["calls"] == n
                 and rows["Calibrate100()"]["calls"] == n,
                 f"call counts Outer={n} InnerA={rows['InnerA()']['calls']} InnerB={rows['InnerB()']['calls']} "
                 f"Calibrate100={rows['Calibrate100()']['calls']}")
    # The slave job is skipped when the previous one is still running: about one per frame.
    slave_calls = slave["SlaveWork()"]["calls"]
    checks.check(0.9 * n <= slave_calls <= n + 2, f"SlaveWork() ran {slave_calls} times on the slave for {n} frames")

    if not calibrate:
        return rows

    for name, ticks in EXPECTED.items():
        checks.ticks(f"{name} per call", rows[name]["total"] / rows[name]["calls"], ticks)
    checks.ticks("Outer() own time per call (hook cost removed)", outer["self"] / n, OUTER_OWN)

    children = (2 * rows["InnerA()"]["total"] / rows["InnerA()"]["calls"]
                + rows["InnerB()"]["total"] / rows["InnerB()"]["calls"])
    checks.ticks("Outer() total - children, per call (hook cost removed)",
                 outer["total"] / n - children - 3 * srl_prof.hook_cost(head), OUTER_OWN)
    # The slave's hooks keep their state in uncached memory, which adds about a tick inside
    # the timed window: allow +2.5 instead of +1.5.
    value = slave["SlaveWork()"]["total"] / slave_calls
    checks.check(SLAVE_WORK - 0.5 <= value <= SLAVE_WORK + 2.5,
                 f"SlaveWork() per call, on the slave's FRT = {value:.2f} ticks, expected {SLAVE_WORK} (+2.5/-0.5)")

    print(f"hook pair cost: {srl_prof.hook_cost(head):.3f} ticks = "
          f"{srl_prof.hook_cost(head) * 1e6 / head['tick_hz']:.2f} us")
    return rows


def check_gprof(profile, syms, checks, path, sampled):
    """Writes the master's gmon.out and checks sh-elf-gprof reads it."""
    head, cpu = profile["head"], profile["master"]
    rows = srl_prof.flat_rows(head, cpu["funcs"], cpu["arcs"], syms)
    with open(path, "wb") as f:
        f.write(srl_prof.gmon_bytes(head, rows, cpu["arcs"], profile["hist"] if sampled else {}))
    result = subprocess.run(["sh-elf-gprof", "-b", ELF, path], capture_output=True, text=True)
    print("\n".join(result.stdout.splitlines()[:12]))
    flat = result.stdout.split("Call graph")[0]
    found = [n for n in ("Calibrate100()", "InnerA()", "InnerB()", "Outer()") if n in flat]
    checks.check(result.returncode == 0 and len(found) == 4,
                 f"sh-elf-gprof reads {os.path.basename(path)} ({'sampled' if sampled else 'hook times'}): "
                 f"found {found}{' ' + result.stderr.strip() if result.stderr.strip() else ''}")
    # Call counts come from the arcs: InnerA is called twice per Outer call.
    checks.check("InnerA()" in result.stdout.split("Call graph")[-1], "sh-elf-gprof call graph lists InnerA()")


def build(report_frames, sample_hz=0):
    subprocess.run(["make", f"SRL_PROFILER_REPORT_FRAMES={report_frames}", f"SRL_PROFILER_SAMPLE_HZ={sample_hz}"],
                   cwd=SAMPLE, check=True, stdout=subprocess.DEVNULL)


def capture_stream(args, label):
    rig = Rig(BIN, args.port, os.path.join(HERE, "ftx_profiler.log"), args.psu_ip)
    try:
        rig.reload(label)
        data = srl_prof_capture.capture("localhost", args.port, args.seconds)
    finally:
        rig.stop_proxy()
    with open(os.path.join(HERE, "capture.bin"), "wb") as f:
        f.write(data)
    print(f"captured {len(data)} bytes")
    return srl_prof.parse_stream(data)


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
        out = session.cmd(f"srl-prof-dump {dump}", timeout=180).strip()
        print(out)
        if "wrote" not in out:
            raise RuntimeError(f"srl-prof-dump failed: {out!r}")
    finally:
        if session:
            session.close()
        rig.stop_proxy()

    check_tables(srl_prof.load(dump), srl_prof.Symbols.from_elf(ELF), checks, streamed=False)


def run_stream(args, checks):
    print(f"=== stream: reports every {REPORT_FRAMES} frames through ftx -g, no GDB ===")
    build(REPORT_FRAMES)
    profile = capture_stream(args, "profiler stream")
    head, events, stats = profile["head"], profile["events"], profile["stats"]
    syms = srl_prof.Symbols.from_elf(ELF)
    rows = check_tables(profile, syms, checks)

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
    checks.check(len(workload) >= 60, f"{len(workload)} Workload slices")
    if workload and "RunWorkload()" in rows:
        run = rows["RunWorkload()"]["total"] / rows["RunWorkload()"]["calls"]
        mean = sum(workload) / len(workload)
        checks.check(run <= mean <= run + srl_prof.hook_cost(head) + 3,
                     f"Workload slice {mean:.2f} ticks vs RunWorkload() {run:.2f} ticks + one hook pair (+3)")
    # Same job timed by both clocks: the master's "Slave job" slice (dispatch to done) must be
    # a bit longer than the slave's own SlaveWorkTask::Do() time. A slave FRT left at another
    # rate would be off by a factor of 4 or more.
    job = [d for name, d in slices if name == "Slave job"]
    slave = {r["name"]: r for r in srl_prof.flat_rows(head, profile["slave"]["funcs"], profile["slave"]["arcs"], syms)}
    task = slave.get("SlaveWorkTask::Do()")
    if job and task:
        master_ticks = sum(job) / len(job)
        slave_ticks = task["total"] / task["calls"]
        checks.check(1.0 <= master_ticks / slave_ticks <= 1.5,
                     f"slave job: {master_ticks:.1f} master ticks (dispatch to done) vs {slave_ticks:.1f} slave "
                     f"ticks inside: the slave's FRT runs at the master's rate")
    else:
        checks.check(False, "no Slave job slices or no SlaveWorkTask::Do() row")

    reports = [d for name, d in slices if name == "SRL profiler report"]
    checks.check(len(reports) >= expected_reports - 1, f"{len(reports)} report slices on the timeline")

    trace_path = os.path.join(HERE, "trace.json")
    trace = srl_prof.perfetto_trace(head, events, syms)
    with open(trace_path, "w") as f:
        json.dump(trace, f)
    frames = sum(1 for e in trace["traceEvents"] if e.get("name") == "VBlank")
    checks.check(frames >= 60, f"Perfetto trace {trace_path}: {frames} frames, {len(trace['traceEvents'])} events")

    check_gprof(profile, syms, checks, os.path.join(HERE, "gmon_hooks.out"), sampled=False)


def run_sampler(args, checks):
    print(f"=== sampler: {SAMPLE_HZ} PC samples per second, streamed ===")
    build(REPORT_FRAMES, SAMPLE_HZ)
    profile = capture_stream(args, "profiler sampler")
    head = profile["head"]
    syms = srl_prof.Symbols.from_elf(ELF)
    check_tables(profile, syms, checks, calibrate=False)

    # The sampler re-arms every interval - 4 .. interval + 3 ticks (jitter): 0.5 tick short on average.
    interval = head["tick_hz"] // SAMPLE_HZ - 0.5
    expected = head["elapsed_ticks"] / interval
    checks.check(abs(head["samples"] / expected - 1) < 0.03,
                 f"{head['samples']} samples since reset, expected {expected:.0f} (3%); "
                 f"{head['samples_outside']} outside the program")

    # Busy ticks per frame: the functions' sample counts must keep the same ratios.
    sampled = dict(srl_prof.sampled_functions(head, profile["hist"], syms))
    busy = {"Calibrate100()": 100, "InnerA()": 100, "InnerB()": 20, "Outer()": 30}
    for a, b in (("Calibrate100()", "InnerA()"), ("InnerA()", "InnerB()"), ("Outer()", "InnerB()")):
        na, nb = sampled.get(a, 0), sampled.get(b, 0)
        if not na or not nb:
            checks.check(False, f"no samples in {a} or {b}")
            continue
        ratio, want = na / nb, busy[a] / busy[b]
        # 5%, or 3 standard deviations of a ratio of two sample counts if that is wider
        tolerance = max(0.05, 3 * math.sqrt(1 / na + 1 / nb))
        checks.check(abs(ratio / want - 1) <= tolerance,
                     f"samples {a} / {b} = {na} / {nb} = {ratio:.3f}, expected {want:.3f} (+-{tolerance:.1%})")

    check_gprof(profile, syms, checks, os.path.join(HERE, "gmon_sampled.out"), sampled=True)


def run_vdp1(args, checks):
    print("=== vdp1: VDP1 load steps, streamed with the sampler on ===")
    build(REPORT_FRAMES, SAMPLE_HZ)
    seconds = max(args.seconds, 15.0)  # one full cycle of the six 2-second load steps
    profile = capture_stream(argparse.Namespace(**dict(vars(args), seconds=seconds)), "profiler vdp1")
    head = profile["head"]
    syms = srl_prof.Symbols.from_elf(ELF)
    srl_prof.print_vdp1(profile, syms, top=8)
    print()

    checks.check(head["vdp1_draw_ends"] > 0, f"sprite draw end interrupt fired {head['vdp1_draw_ends']} times "
                                             f"(vector 0x4D, SCU bit 13)")

    # Split VDP1 frames by load step, from the "VDP1 load N" marks the sample leaves.
    ms_per_tick = 1000.0 / head["tick_hz"]
    items = srl_prof.timeline(profile["events"], syms)
    steps = [(t, int(name.split()[-1])) for t, kind, name in items
             if kind == srl_prof.EVENT_INSTANT and name.startswith("VDP1 load ")]
    missed = [t for t, kind, name in items if name == "VDP1 missed frame"]
    per_step, missed_per_step = {}, {}
    # Skip the first frames of each step: VDP1 still draws what was queued before the change.
    settle = 4 * 3500
    for start, end, finished in srl_prof.vdp1_frames(profile["events"], syms):
        step = [n for t, n in steps if t < start - settle]
        if finished and step and not [t for t, n in steps if start - settle <= t <= end]:
            per_step.setdefault(step[-1], []).append((end - start) * ms_per_tick)
    for t in missed:
        step = [n for st, n in steps if st < t - settle]
        if step and not [st for st, n in steps if t - settle <= st <= t]:
            missed_per_step[step[-1]] = missed_per_step.get(step[-1], 0) + 1
    for n in sorted(per_step):
        times = per_step[n]
        print(f"  {n:2d} polygons: {len(times):3d} frames, VDP1 {sum(times) / len(times):6.2f} ms on average, "
              f"{missed_per_step.get(n, 0)} missed VBlanks")

    means = {n: sum(t) / len(t) for n, t in per_step.items()}
    checks.check(all(n in means for n in (0, 1, 2, 4)), f"VDP1 frames seen for loads {sorted(means)}")
    light = [n for n in (0, 1, 2, 4) if n in means]
    checks.check(all(means[a] < means[b] for a, b in zip(light, light[1:])),
                 "VDP1 draw time grows with the polygon count: " +
                 ", ".join(f"{n}: {means[n]:.2f} ms" for n in light))
    checks.check(sum(missed_per_step.get(n, 0) for n in (0, 1, 2, 4)) == 0, "no missed frames with 0 to 4 polygons")
    checks.check(missed_per_step.get(16, 0) > 0, f"missed frames with 16 polygons: {missed_per_step.get(16, 0)}")

    slots = sorted(profile["vdp1"].items(), key=lambda kv: kv[1], reverse=True)
    top = srl_prof.decode_vdp1_command(profile["vdp1_commands"][slots[0][0]]) if slots else "none"
    checks.check(top.startswith("polygon") and "(-160, -120)-(159, 119)" in top,
                 f"costliest VDP1 command is a full-screen polygon: {top}")

    dma = sum(1 for t, kind, name in items if name.startswith("SCU DMA"))
    checks.check(dma > 0, f"{dma} SCU DMA end marks")

    trace = srl_prof.perfetto_trace(head, profile["events"], syms)
    with open(os.path.join(HERE, "trace_vdp1.json"), "w") as f:
        json.dump(trace, f)
    slices = sum(1 for e in trace["traceEvents"] if e.get("name") == "VDP1 drawing")
    cut = sum(1 for e in trace["traceEvents"] if e.get("name") == "VDP1 drawing (cut at VBlank)")
    checks.check(slices > 60 and cut > 60, f"Perfetto trace_vdp1.json: {slices} VDP1 frames, {cut} cut at VBlank")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--mode", choices=("gdb", "stream", "sampler", "vdp1", "all"), default="all")
    parser.add_argument("--psu-ip", help="network PSU used to power-cycle the console before upload")
    parser.add_argument("--seconds", type=float, default=8.0, help="how long to let the sample run")
    parser.add_argument("--port", type=int, default=1234)
    args = parser.parse_args()

    checks = Checks()
    if args.mode in ("gdb", "all"):
        run_gdb(args, checks)
    if args.mode in ("stream", "all"):
        run_stream(args, checks)
    if args.mode in ("sampler", "all"):
        run_sampler(args, checks)
    if args.mode in ("vdp1", "all"):
        run_vdp1(args, checks)

    print("ALL PASS" if not checks.failures else f"FAILED ({len(checks.failures)}):")
    for failure in checks.failures:
        print(f"  {failure}")
    return 1 if checks.failures else 0


if __name__ == "__main__":
    sys.exit(main())
