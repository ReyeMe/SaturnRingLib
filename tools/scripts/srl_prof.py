#!/usr/bin/env python3
"""Reads SRL profiler data (g_srl_prof, see saturnringlib/srl_profiler.hpp).

Input is either a GDB dump (`srl-prof-dump prof.bin`) or a stream captured with
srl_prof_capture.py. Usage:
    srl_prof.py program.elf prof.bin                              # flat profile
    srl_prof.py program.elf prof.bin --format folded              # stacks for flamegraph.pl / speedscope
    srl_prof.py program.elf capture.bin --format perfetto -o trace.json   # open in ui.perfetto.dev
"""

import argparse
import bisect
import json
import shutil
import struct
import subprocess
import sys

HEADER = struct.Struct(">4s23I")
FUNC = struct.Struct(">4I")
ARC = struct.Struct(">3I")
EVENT = struct.Struct(">2I")
VERSION = 2

HEADER_FIELDS = ("version", "self_address", "tick_hz", "hook_cost_q16", "enabled", "frames",
                 "elapsed_ticks", "func_capacity", "arc_capacity", "stack_capacity", "event_capacity",
                 "dropped_funcs", "dropped_arcs", "dropped_depth", "mismatched_exits", "event_count",
                 "dropped_events", "report_frames", "reports", "dropped_reports", "report_bytes",
                 "report_ticks", "reserved")

SYNC = b"\xa5\x5a"
PACKET_EVENTS, PACKET_TABLES = 1, 2
EVENT_BEGIN, EVENT_END, EVENT_INSTANT = 1, 2, 3


def parse_header(data, offset=0):
    if len(data) < offset + HEADER.size:
        raise ValueError(f"{len(data) - offset} bytes, smaller than the header")
    magic, *values = HEADER.unpack_from(data, offset)
    if magic != b"SRLP":
        raise ValueError(f"not profiler data (magic {magic!r}); was Profiler::Init() called?")
    head = dict(zip(HEADER_FIELDS, values))
    if head["version"] != VERSION:
        raise ValueError(f"data version {head['version']}, this script reads version {VERSION}")
    return head


def parse_rows(data, offset, func_rows, arc_rows):
    """Reads function and arc rows, dropping free ones. Returns (funcs, arcs, offset)."""
    funcs = []
    for _ in range(func_rows):
        address, calls, total, self_ticks = FUNC.unpack_from(data, offset)
        offset += FUNC.size
        if address:
            funcs.append({"address": address, "calls": calls, "total": total, "self": self_ticks})
    arcs = []
    for _ in range(arc_rows):
        src, dst, count = ARC.unpack_from(data, offset)
        offset += ARC.size
        if src:
            arcs.append({"from": src, "to": dst, "count": count})
    return funcs, arcs, offset


def parse_events(data, offset, count):
    return [EVENT.unpack_from(data, offset + i * EVENT.size) for i in range(count)]


def parse_block(data):
    """Splits a GDB dump of g_srl_prof into its header, function rows, arc rows and events."""
    head = parse_header(data)
    expected = (HEADER.size + head["func_capacity"] * FUNC.size + head["arc_capacity"] * ARC.size
                + head["event_capacity"] * EVENT.size)
    if len(data) < expected:
        raise ValueError(f"dump is {len(data)} bytes, header says {expected}")
    funcs, arcs, offset = parse_rows(data, HEADER.size, head["func_capacity"], head["arc_capacity"])
    return head, funcs, arcs, parse_events(data, offset, head["event_count"])


def fletcher16(payload):
    sum1 = sum2 = 0
    for b in payload:
        sum1 = (sum1 + b) % 255
        sum2 = (sum2 + sum1) % 255
    return (sum2 << 8) | sum1


def parse_stream(data):
    """Reads the packets of a captured stream.

    Returns (head, funcs, arcs, events, stats): the tables of the last report and the events
    of all reports in order. Bytes outside valid packets (log text, cut reports) are skipped.
    """
    stats = {"reports": 0, "bad_packets": 0, "skipped_bytes": 0}
    tables = None
    events = []
    pos = 0
    while True:
        start = data.find(SYNC, pos)
        if start < 0:
            stats["skipped_bytes"] += len(data) - pos
            break
        stats["skipped_bytes"] += start - pos
        if start + 7 > len(data):
            stats["skipped_bytes"] += len(data) - start
            break
        kind = data[start + 2]
        length = struct.unpack_from(">I", data, start + 3)[0]
        end = start + 7 + length + 2
        payload = data[start + 7:start + 7 + length]
        if (kind not in (PACKET_EVENTS, PACKET_TABLES) or end > len(data)
                or fletcher16(payload) != struct.unpack_from(">H", data, end - 2)[0]):
            # Not a packet, or a cut one: look for the next sync right after this one.
            stats["bad_packets"] += 1
            stats["skipped_bytes"] += 1
            pos = start + 1
            continue
        if kind == PACKET_EVENTS:
            events.extend(parse_events(payload, 0, length // EVENT.size))
        else:
            head = parse_header(payload)
            func_count, arc_count = struct.unpack_from(">2I", payload, HEADER.size)
            funcs, arcs, _ = parse_rows(payload, HEADER.size + 8, func_count, arc_count)
            tables = (head, funcs, arcs)
            stats["reports"] += 1
        pos = end
    if tables is None:
        raise ValueError(f"no complete report in the capture ({stats})")
    return tables + (events, stats)


def load(path):
    """Reads a GDB dump or a captured stream. Returns (head, funcs, arcs, events, stats or None)."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] == b"SRLP":
        return parse_block(data) + (None,)
    return parse_stream(data)


class Symbols:
    """Maps code addresses to the function that contains them, and reads strings from the ELF."""

    def __init__(self, functions, others=(), sections=()):
        # functions: (address, name) of code symbols; others: data symbols, looked up by name only
        # sections: (address, bytes) of the ELF's loaded sections, for string literals
        functions = sorted(functions)
        self.addresses = [a for a, _ in functions]
        self.names = [n for _, n in functions]
        self.by_name = {n: a for a, n in list(functions) + list(others)}
        self.sections = list(sections)

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
        with open(elf_path, "rb") as f:
            sections = elf_sections(f.read())
        return cls(functions, others, sections)

    def name(self, address):
        i = bisect.bisect_right(self.addresses, address) - 1
        return self.names[i] if i >= 0 else f"0x{address:08x}"

    def start(self, address):
        """Start address of the function containing address (for return addresses)."""
        i = bisect.bisect_right(self.addresses, address) - 1
        return self.addresses[i] if i >= 0 else address

    def string(self, address):
        """The NUL-terminated string at address (top 4 bits ignored, as in event words)."""
        address &= 0x0FFFFFFF
        for base, data in self.sections:
            offset = address - (base & 0x0FFFFFFF)
            if 0 <= offset < len(data):
                end = data.find(b"\0", offset)
                return data[offset:end if end >= 0 else len(data)].decode(errors="replace")
        return f"0x{address:07x}"


def elf_sections(elf):
    """(address, bytes) of each loaded section with contents in a 32-bit big-endian ELF."""
    if elf[:4] != b"\x7fELF" or elf[4] != 1 or elf[5] != 2:
        raise ValueError("not a 32-bit big-endian ELF")
    shoff = struct.unpack_from(">I", elf, 0x20)[0]
    shentsize, shnum = struct.unpack_from(">2H", elf, 0x2E)
    sections = []
    for i in range(shnum):
        _, kind, flags, addr, offset, size = struct.unpack_from(">6I", elf, shoff + i * shentsize)
        if flags & 0x2 and kind != 8 and size:  # SHF_ALLOC, not SHT_NOBITS
            sections.append((addr, elf[offset:offset + size]))
    return sections


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


def timeline(events, syms):
    """Events in time order as (ticks, type, name), ticks unwrapped from 32 bits."""
    out = []
    base = previous = None
    high = 0
    for word, ticks in events:
        if previous is not None and ticks < previous and previous - ticks > 0x80000000:
            high += 1 << 32  # the 32-bit tick count wrapped
        previous = ticks
        out.append((ticks + high, word >> 28, syms.string(word & 0x0FFFFFFF)))
    out.sort(key=lambda e: e[0])  # stable: a scope's End stays after its Begin
    return out


def perfetto_trace(head, events, syms):
    """Chrome Trace Event JSON (opens in ui.perfetto.dev) for the master CPU's timeline."""
    us_per_tick = 1e6 / head["tick_hz"]
    items = timeline(events, syms)
    t0 = items[0][0] if items else 0
    trace = [
        {"name": "process_name", "ph": "M", "pid": 1, "args": {"name": "Saturn"}},
        {"name": "thread_name", "ph": "M", "pid": 1, "tid": 1, "args": {"name": "Master SH-2"}},
    ]
    open_scopes = []
    for ticks, kind, name in items:
        ts = (ticks - t0) * us_per_tick
        if kind == EVENT_BEGIN:
            open_scopes.append(name)
            trace.append({"name": name, "ph": "B", "ts": ts, "pid": 1, "tid": 1})
        elif kind == EVENT_END and name in open_scopes:
            # Close what is still open above it too: an End lost to a full buffer would
            # otherwise leave the rest of the timeline nested under that slice.
            while open_scopes:
                top = open_scopes.pop()
                trace.append({"name": top, "ph": "E", "ts": ts, "pid": 1, "tid": 1})
                if top == name:
                    break
        elif kind == EVENT_INSTANT:
            trace.append({"name": name, "ph": "i", "s": "t", "ts": ts, "pid": 1, "tid": 1})
    return {"traceEvents": trace, "displayTimeUnit": "ms"}


def check_elf(head, syms):
    address = syms.by_name.get("g_srl_prof")
    if address is not None and address != head["self_address"]:
        return (f"warning: data says g_srl_prof is at 0x{head['self_address']:08x}, the ELF says "
                f"0x{address:08x}: the ELF doesn't match the running program")
    return None


def print_flat(head, funcs, arcs, syms, top, out=sys.stdout, stats=None):
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
    if head["report_ticks"]:
        seconds = head["report_ticks"] * ms_per_tick / 1000
        print(f"last report: {head['report_bytes']} bytes in {seconds * 1000:.2f} ms "
              f"({head['report_bytes'] / seconds / 1024:.0f} KB/s)", file=out)
    if stats:
        print(f"capture: {stats['reports']} reports, {stats['bad_packets']} bad packets, "
              f"{stats['skipped_bytes']} bytes outside packets", file=out)
    for key in ("dropped_funcs", "dropped_arcs", "dropped_depth", "mismatched_exits", "dropped_events",
                "dropped_reports"):
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
    parser.add_argument("data", help="GDB dump (srl-prof-dump) or capture (srl_prof_capture.py)")
    parser.add_argument("--format", choices=("flat", "folded", "perfetto"), default="flat")
    parser.add_argument("-o", "--output", help="write here instead of stdout")
    parser.add_argument("--top", type=int, default=40, help="rows in the flat profile")
    parser.add_argument("--nm", help="nm tool to use (default: sh-elf-nm or sh2eb-elf-nm)")
    args = parser.parse_args()

    head, funcs, arcs, events, stats = load(args.data)
    syms = Symbols.from_elf(args.elf, args.nm)

    warning = check_elf(head, syms)
    if warning:
        print(warning, file=sys.stderr)

    out = open(args.output, "w") if args.output else sys.stdout
    try:
        if args.format == "flat":
            print_flat(head, funcs, arcs, syms, args.top, out, stats)
        elif args.format == "folded":
            print("\n".join(folded_lines(head, funcs, arcs, syms)), file=out)
        else:
            json.dump(perfetto_trace(head, events, syms), out)
    finally:
        if args.output:
            out.close()


if __name__ == "__main__":
    main()
