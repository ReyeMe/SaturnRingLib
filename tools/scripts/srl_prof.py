#!/usr/bin/env python3
"""Reads an SRL profiler dump (g_srl_prof, see saturnringlib/srl_profiler.hpp).

Usage:
    srl_prof.py program.elf prof.bin                  # flat profile
    srl_prof.py program.elf prof.bin --format folded  # stacks for flamegraph.pl / speedscope

Get prof.bin from GDB: `source tools/scripts/srl_prof.gdb`, then `srl-prof-dump prof.bin`.
"""

import argparse
import bisect
import shutil
import struct
import subprocess
import sys

HEADER = struct.Struct(">4s15I")
FUNC = struct.Struct(">4I")
ARC = struct.Struct(">3I")
VERSION = 1

HEADER_FIELDS = ("version", "self_address", "tick_hz", "hook_cost_q16", "enabled", "frames",
                 "elapsed_ticks", "func_capacity", "arc_capacity", "stack_capacity",
                 "dropped_funcs", "dropped_arcs", "dropped_depth", "mismatched_exits", "reserved")


def parse_block(data):
    """Splits a raw dump into its header, function rows and arc rows (free rows dropped)."""
    if len(data) < HEADER.size:
        raise ValueError(f"dump is {len(data)} bytes, smaller than the header")
    magic, *values = HEADER.unpack_from(data, 0)
    if magic != b"SRLP":
        raise ValueError(f"not a profiler dump (magic {magic!r}); was Profiler::Init() called?")
    head = dict(zip(HEADER_FIELDS, values))
    if head["version"] != VERSION:
        raise ValueError(f"dump version {head['version']}, this script reads version {VERSION}")
    expected = HEADER.size + head["func_capacity"] * FUNC.size + head["arc_capacity"] * ARC.size
    if len(data) < expected:
        raise ValueError(f"dump is {len(data)} bytes, header says {expected}")

    offset = HEADER.size
    funcs = []
    for _ in range(head["func_capacity"]):
        address, calls, total, self_ticks = FUNC.unpack_from(data, offset)
        offset += FUNC.size
        if address:
            funcs.append({"address": address, "calls": calls, "total": total, "self": self_ticks})

    arcs = []
    for _ in range(head["arc_capacity"]):
        src, dst, count = ARC.unpack_from(data, offset)
        offset += ARC.size
        if src:
            arcs.append({"from": src, "to": dst, "count": count})

    return head, funcs, arcs


class Symbols:
    """Maps code addresses to the function that contains them."""

    def __init__(self, functions, others=()):
        # functions: (address, name) of code symbols; others: data symbols, looked up by name only
        functions = sorted(functions)
        self.addresses = [a for a, _ in functions]
        self.names = [n for _, n in functions]
        self.by_name = {n: a for a, n in list(functions) + list(others)}

    @classmethod
    def from_elf(cls, elf_path, nm=None):
        nm = nm or next((t for t in ("sh-elf-nm", "sh2eb-elf-nm") if shutil.which(t)), None)
        if not nm:
            raise RuntimeError("no sh-elf-nm or sh2eb-elf-nm on PATH; pass --nm")
        out = subprocess.run([nm, "-C", "--defined-only", elf_path],
                             capture_output=True, text=True, check=True).stdout
        functions, others = [], []
        for line in out.splitlines():
            parts = line.split(maxsplit=2)
            if len(parts) == 3:
                address, kind, name = parts
                # sh-elf prefixes C symbols with "_"; demangled C++ names have none.
                entry = (int(address, 16), name[1:] if name.startswith("_") else name)
                (functions if kind in "TtWw" else others).append(entry)
        return cls(functions, others)

    def name(self, address):
        i = bisect.bisect_right(self.addresses, address) - 1
        return self.names[i] if i >= 0 else f"0x{address:08x}"

    def start(self, address):
        """Start address of the function containing address (for return addresses)."""
        i = bisect.bisect_right(self.addresses, address) - 1
        return self.addresses[i] if i >= 0 else address


def hook_cost(head):
    return head["hook_cost_q16"] / 65536.0


def flat_rows(head, funcs, arcs, syms):
    """Per-function rows. Own time has the hook cost of the calls it made removed."""
    calls_made = {}
    for arc in arcs:
        caller = syms.start(arc["from"])
        calls_made[caller] = calls_made.get(caller, 0) + arc["count"]

    cost = hook_cost(head)
    rows = []
    for f in funcs:
        own = max(0.0, f["self"] - calls_made.get(f["address"], 0) * cost)
        rows.append({"name": syms.name(f["address"]), "address": f["address"], "calls": f["calls"],
                     "total": f["total"], "self_raw": f["self"], "self": own})
    rows.sort(key=lambda r: r["self"], reverse=True)
    return rows


def folded_lines(head, funcs, arcs, syms, max_depth=32):
    """Stacks weighted by own time, in microseconds.

    Each function's own time is shared among its callers in proportion to their call
    counts (the same assumption gprof makes), all the way up to the roots.
    """
    callers = {}
    for arc in arcs:
        callee = arc["to"]
        caller = syms.start(arc["from"])
        callers.setdefault(callee, {})
        callers[callee][caller] = callers[callee].get(caller, 0) + arc["count"]

    def paths(fn, seen, depth):
        ups = {c: n for c, n in callers.get(fn, {}).items() if c not in seen}
        total = sum(ups.values())
        if not ups or depth >= max_depth:
            return [([fn], 1.0)]
        result = []
        for caller, count in ups.items():
            for path, weight in paths(caller, seen | {fn}, depth + 1):
                result.append((path + [fn], weight * count / total))
        return result

    us_per_tick = 1e6 / head["tick_hz"]
    stacks = {}
    for row in flat_rows(head, funcs, arcs, syms):
        for path, weight in paths(row["address"], frozenset(), 0):
            key = ";".join(syms.name(a) for a in path)
            stacks[key] = stacks.get(key, 0.0) + row["self"] * weight * us_per_tick

    return [f"{k} {round(v)}" for k, v in sorted(stacks.items()) if round(v) > 0]


def check_elf(head, syms):
    address = syms.by_name.get("g_srl_prof")
    if address is not None and address != head["self_address"]:
        return (f"warning: dump says g_srl_prof is at 0x{head['self_address']:08x}, the ELF says "
                f"0x{address:08x}: the ELF doesn't match the running program")
    return None


def print_flat(head, funcs, arcs, syms, top, out=sys.stdout):
    ms_per_tick = 1000.0 / head["tick_hz"]
    frames = head["frames"]
    elapsed_ms = head["elapsed_ticks"] * ms_per_tick
    total_calls = sum(f["calls"] for f in funcs)
    overhead_ms = total_calls * hook_cost(head) * ms_per_tick

    print(f"{frames} frames, {elapsed_ms:.1f} ms; tick {ms_per_tick * 1000:.2f} us; "
          f"hook pair {hook_cost(head) * ms_per_tick * 1000:.2f} us", file=out)
    if elapsed_ms > 0:
        print(f"hook overhead: {overhead_ms:.1f} ms ({100 * overhead_ms / elapsed_ms:.1f}% of elapsed time)",
              file=out)
    for key in ("dropped_funcs", "dropped_arcs", "dropped_depth", "mismatched_exits"):
        if head[key]:
            print(f"warning: {key} = {head[key]}", file=out)
    if not head["enabled"]:
        print("note: recording was stopped", file=out)
    print(file=out)

    per = "ms/frame" if frames else "ms"
    div = frames or 1
    print(f"{'own %':>6} {'own ' + per:>13} {'total ' + per:>15} {'calls':>9} {'us/call':>9}  function", file=out)
    for row in flat_rows(head, funcs, arcs, syms)[:top]:
        share = 100.0 * row["self"] * ms_per_tick / elapsed_ms if elapsed_ms else 0.0
        per_call = row["total"] * ms_per_tick * 1000 / row["calls"] if row["calls"] else 0.0
        print(f"{share:6.1f} {row['self'] * ms_per_tick / div:13.3f} {row['total'] * ms_per_tick / div:15.3f} "
              f"{row['calls']:9d} {per_call:9.1f}  {row['name']}", file=out)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("elf", help="the program's .elf (with symbols)")
    parser.add_argument("dump", help="raw g_srl_prof block from srl-prof-dump")
    parser.add_argument("--format", choices=("flat", "folded"), default="flat")
    parser.add_argument("--top", type=int, default=40, help="rows in the flat profile")
    parser.add_argument("--nm", help="nm tool to use (default: sh-elf-nm or sh2eb-elf-nm)")
    args = parser.parse_args()

    with open(args.dump, "rb") as f:
        head, funcs, arcs = parse_block(f.read())
    syms = Symbols.from_elf(args.elf, args.nm)

    warning = check_elf(head, syms)
    if warning:
        print(warning, file=sys.stderr)

    if args.format == "flat":
        print_flat(head, funcs, arcs, syms, args.top)
    else:
        print("\n".join(folded_lines(head, funcs, arcs, syms)))


if __name__ == "__main__":
    main()
