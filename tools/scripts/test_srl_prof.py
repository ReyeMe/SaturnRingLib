#!/usr/bin/env python3
"""Offline tests for srl_prof.py: hand-built data with known numbers, no hardware needed.

Run: python3 tools/scripts/test_srl_prof.py
"""

import os
import struct
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import srl_prof  # noqa: E402

MAIN, OUTER, INNER_A, INNER_B = 0x1000, 0x2000, 0x2100, 0x2200
STRINGS = 0x06003000  # a fake .rodata holding the event names
TICK_HZ = 200000  # 5 us per tick
RODATA = b"Workload\0VBlank\0"
WORKLOAD, VBLANK = STRINGS, STRINGS + 9


def make_header(func_capacity, arc_capacity, event_capacity, event_count, hook_cost_ticks=1.0):
    values = dict.fromkeys(srl_prof.HEADER_FIELDS, 0)
    values.update(version=2, self_address=0x06050000, tick_hz=TICK_HZ, hook_cost_q16=int(hook_cost_ticks * 65536),
                  enabled=1, frames=10, elapsed_ticks=10000, func_capacity=func_capacity,
                  arc_capacity=arc_capacity, stack_capacity=64, event_capacity=event_capacity,
                  event_count=event_count)
    return srl_prof.HEADER.pack(b"SRLP", *(values[k] for k in srl_prof.HEADER_FIELDS))


def rows(funcs, arcs):
    return b"".join(srl_prof.FUNC.pack(*f) for f in funcs) + b"".join(srl_prof.ARC.pack(*a) for a in arcs)


def make_dump(funcs, arcs, events=(), capacity=8):
    data = make_header(capacity, capacity, capacity, len(events))
    data += b"".join(srl_prof.FUNC.pack(*f) for f in funcs) + b"\0" * (srl_prof.FUNC.size * (capacity - len(funcs)))
    data += b"".join(srl_prof.ARC.pack(*a) for a in arcs) + b"\0" * (srl_prof.ARC.size * (capacity - len(arcs)))
    data += b"".join(srl_prof.EVENT.pack(*e) for e in events) + b"\0" * (srl_prof.EVENT.size * (capacity - len(events)))
    return data


def packet(kind, payload):
    return (srl_prof.SYNC + bytes([kind]) + struct.pack(">I", len(payload)) + payload
            + struct.pack(">H", srl_prof.fletcher16(payload)))


def report(funcs, arcs, events):
    events_payload = b"".join(srl_prof.EVENT.pack(*e) for e in events)
    tables_payload = make_header(512, 1024, 4096, 0) + struct.pack(">2I", len(funcs), len(arcs)) + rows(funcs, arcs)
    return packet(srl_prof.PACKET_EVENTS, events_payload) + packet(srl_prof.PACKET_TABLES, tables_payload)


def event(kind, name, ticks):
    return ((kind << 28) | (name & 0x0FFFFFFF), ticks)


# 10 frames of: main calls Outer (own 30 ticks + 3 hook pairs leaked = 33 raw),
# Outer calls InnerA twice (50 each) and InnerB once (20).
FUNCS = [
    (OUTER, 10, 1530, 330),
    (INNER_A, 20, 1000, 1000),
    (INNER_B, 10, 200, 200),
]
ARCS = [
    (MAIN + 0x10, OUTER, 10),  # return address inside main: main was entered before profiling
    (OUTER, INNER_A, 20),
    (OUTER, INNER_B, 10),
]
SYMS = srl_prof.Symbols([(MAIN, "main"), (OUTER, "Outer()"), (INNER_A, "InnerA()"), (INNER_B, "InnerB()")],
                        [(0x06050000, "g_srl_prof")], [(STRINGS, RODATA)])


def trace_names(trace):
    return [(e["ph"], e["name"], e["ts"]) for e in trace["traceEvents"] if e["ph"] != "M"]


class SrlProfTest(unittest.TestCase):
    def setUp(self):
        self.head, self.funcs, self.arcs, self.events = srl_prof.parse_block(make_dump(FUNCS, ARCS))

    def test_parse_skips_free_rows(self):
        self.assertEqual(len(self.funcs), 3)
        self.assertEqual(len(self.arcs), 3)
        self.assertEqual(self.head["tick_hz"], TICK_HZ)

    def test_flat_removes_hook_cost_from_own_time(self):
        rows_by_name = {r["name"]: r for r in srl_prof.flat_rows(self.head, self.funcs, self.arcs, SYMS)}
        self.assertAlmostEqual(rows_by_name["Outer()"]["self"], 300)  # 330 raw - 30 calls made x 1 tick
        self.assertAlmostEqual(rows_by_name["InnerA()"]["self"], 1000)  # leaf: unchanged
        self.assertEqual(rows_by_name["InnerA()"]["calls"], 20)

    def test_folded_follows_callers(self):
        lines = dict(line.rsplit(" ", 1) for line in srl_prof.folded_lines(self.head, self.funcs, self.arcs, SYMS))
        self.assertEqual(lines, {
            "main;Outer()": "1500",  # 300 ticks x 5 us
            "main;Outer();InnerA()": "5000",
            "main;Outer();InnerB()": "1000",
        })

    def test_shared_callee_is_split_by_call_count(self):
        # InnerB called 3 times from Outer, once from main: its own time splits 3:1.
        arcs = ARCS + [(MAIN + 0x20, INNER_B, 10)]
        arcs[2] = (OUTER, INNER_B, 30)
        head, funcs, arcs, _ = srl_prof.parse_block(make_dump(FUNCS, arcs))
        lines = dict(line.rsplit(" ", 1) for line in srl_prof.folded_lines(head, funcs, arcs, SYMS))
        self.assertEqual(lines["main;Outer();InnerB()"], "750")
        self.assertEqual(lines["main;InnerB()"], "250")

    def test_recursion_terminates(self):
        arcs = ARCS + [(INNER_A, OUTER, 5)]  # Outer -> InnerA -> Outer cycle
        head, funcs, arcs, _ = srl_prof.parse_block(make_dump(FUNCS, arcs))
        self.assertTrue(srl_prof.folded_lines(head, funcs, arcs, SYMS))

    def test_rejects_wrong_magic_and_short_dump(self):
        dump = make_dump(FUNCS, ARCS)
        with self.assertRaises(ValueError):
            srl_prof.parse_block(b"XXXX" + dump[4:])
        with self.assertRaises(ValueError):
            srl_prof.parse_block(dump[:-1])

    def test_elf_mismatch_warning(self):
        self.assertIsNone(srl_prof.check_elf(self.head, SYMS))
        other = srl_prof.Symbols([], [(0x06060000, "g_srl_prof")])
        self.assertIn("doesn't match", srl_prof.check_elf(self.head, other))

    def test_dump_events_only_up_to_count(self):
        events = [event(srl_prof.EVENT_INSTANT, VBLANK, 5)]
        _, _, _, parsed = srl_prof.parse_block(make_dump(FUNCS, ARCS, events))
        self.assertEqual(parsed, events)

    def test_event_names_come_from_the_elf(self):
        self.assertEqual(SYMS.string(WORKLOAD), "Workload")
        self.assertEqual(SYMS.string(VBLANK | 0x20000000), "VBlank")  # cache-through alias
        self.assertEqual(SYMS.string(0x1234), "0x0001234")


class StreamTest(unittest.TestCase):
    EVENTS1 = [event(srl_prof.EVENT_INSTANT, VBLANK, 100), event(srl_prof.EVENT_BEGIN, WORKLOAD, 110),
               event(srl_prof.EVENT_END, WORKLOAD, 150)]
    EVENTS2 = [event(srl_prof.EVENT_INSTANT, VBLANK, 3500)]

    def test_reports_with_log_text_and_a_cut_packet(self):
        cut = report(FUNCS, ARCS, self.EVENTS1)[:20]  # abandoned mid-report
        data = (b"log line\n" + report(FUNCS[:1], ARCS[:1], self.EVENTS1) + cut + b"\xa5\x5a junk"
                + report(FUNCS, ARCS, self.EVENTS2))
        head, funcs, arcs, events, stats = srl_prof.parse_stream(data)
        self.assertEqual(len(funcs), 3)  # tables of the last report
        self.assertEqual(events, self.EVENTS1 + self.EVENTS2)
        self.assertEqual(stats["reports"], 2)
        self.assertGreater(stats["bad_packets"], 0)

    def test_no_report_is_an_error(self):
        with self.assertRaises(ValueError):
            srl_prof.parse_stream(b"just log text")

    def test_perfetto_slices_and_instants(self):
        head, _, _, events, _ = srl_prof.parse_stream(report(FUNCS, ARCS, self.EVENTS1))
        self.assertEqual(trace_names(srl_prof.perfetto_trace(head, events, SYMS)), [
            ("i", "VBlank", 0.0), ("B", "Workload", 50.0), ("E", "Workload", 250.0)])

    def test_perfetto_closes_a_scope_whose_end_was_lost(self):
        events = [event(srl_prof.EVENT_BEGIN, WORKLOAD, 0), event(srl_prof.EVENT_BEGIN, VBLANK, 10),
                  event(srl_prof.EVENT_END, WORKLOAD, 20)]  # VBlank's End lost
        trace = trace_names(srl_prof.perfetto_trace(self_head(), events, SYMS))
        self.assertEqual(trace[-2:], [("E", "VBlank", 100.0), ("E", "Workload", 100.0)])

    def test_tick_wrap(self):
        events = [event(srl_prof.EVENT_INSTANT, VBLANK, 0xFFFFFFF0), event(srl_prof.EVENT_INSTANT, VBLANK, 0x10)]
        ticks = [t for t, _, _ in srl_prof.timeline(events, SYMS)]
        self.assertEqual(ticks[1] - ticks[0], 0x20)


def self_head():
    return srl_prof.parse_header(make_header(1, 1, 1, 0))


if __name__ == "__main__":
    unittest.main()
