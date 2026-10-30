#!/usr/bin/env python3
"""Offline tests for srl_prof.py: hand-built data with known numbers, no hardware needed.

Run: python3 tools/scripts/test_srl_prof.py
"""

import argparse
import os
import socket
import struct
import sys
import tempfile
import threading
import time
import unittest
from unittest import mock

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import srl_prof  # noqa: E402

MAIN, OUTER, INNER_A, INNER_B, SLAVE_FN = 0x1000, 0x2000, 0x2100, 0x2200, 0x3000
STRINGS = 0x06003000  # a fake .rodata holding the event names
TICK_HZ = 200000  # 5 us per tick
RODATA = b"Workload\0VBlank\0VDP1 draw end\0SCU DMA 0 end\0"
WORKLOAD, VBLANK, VDP1_END, DMA0_END = STRINGS, STRINGS + 9, STRINGS + 16, STRINGS + 30


def make_header(capacity, event_count=0, hist_bins=0, **extra):
    values = dict.fromkeys(srl_prof.HEADER_FIELDS, 0)
    values.update(version=4, self_address=0x06050000, tick_hz=TICK_HZ, hook_cost_q16=65536, enabled=1,
                  frames=10, elapsed_ticks=10000, func_capacity=capacity, arc_capacity=capacity,
                  stack_capacity=64, event_capacity=capacity, event_count=event_count,
                  hist_capacity=max(hist_bins, 1), hist_low=OUTER, hist_shift=4, hist_bins=hist_bins,
                  sample_hz=1000 if hist_bins else 0, slave_ready=1, vdp1_hist_capacity=max(hist_bins, 1))
    values.update(extra)
    return srl_prof.HEADER.pack(b"SRLP", *(values[k] for k in srl_prof.HEADER_FIELDS))


def padded(rows, struct_, capacity):
    return b"".join(struct_.pack(*r) for r in rows) + b"\0" * (struct_.size * (capacity - len(rows)))


def make_dump(funcs, arcs, events=(), slave_funcs=(), hist=(), capacity=8):
    data = make_header(capacity, len(events), len(hist))
    for f, a in ((funcs, arcs), (slave_funcs, ())):
        data += bytes(srl_prof.CPU_COUNTERS.size) + padded(f, srl_prof.FUNC, capacity) + padded(a, srl_prof.ARC, capacity)
    data += padded(events, srl_prof.EVENT, capacity)
    data += struct.pack(f">{max(len(hist), 1)}I", *(hist or (0,)))
    data += struct.pack(f">{max(len(hist), 1)}I", *([0] * len(hist) or (0,)))  # VDP1 slots
    return data


def packet(kind, payload):
    return (srl_prof.SYNC + bytes([kind]) + struct.pack(">I", len(payload)) + payload
            + struct.pack(">H", srl_prof.fletcher16(payload)))


def command(ctrl=4, pmod=0, colr=0x801F, srca=0, size=0, coords=(-160, -120, 159, -120, 159, 119, -160, 119)):
    return struct.pack(">6H8h2H", ctrl, 0, pmod, colr, srca, size, *coords, 0, 0)


def report(funcs, arcs, events, slave_funcs=(), hist=None, vdp1=None):
    events_payload = b"".join(srl_prof.EVENT.pack(*e) for e in events)
    tables = make_header(512, hist_bins=16 if hist is not None else 0)
    for f, a in ((funcs, arcs), (slave_funcs, ())):
        tables += bytes(srl_prof.CPU_COUNTERS.size) + struct.pack(">2I", len(f), len(a))
        tables += b"".join(srl_prof.FUNC.pack(*r) for r in f) + b"".join(srl_prof.ARC.pack(*r) for r in a)
    data = packet(srl_prof.PACKET_EVENTS, events_payload) + packet(srl_prof.PACKET_TABLES, tables)
    if hist is not None:
        body = struct.pack(">3I", OUTER, 4, len(hist)) + b"".join(struct.pack(">2I", i, n) for i, n in hist.items())
        data += packet(srl_prof.PACKET_HISTOGRAM, body)
    if vdp1 is not None:
        body = struct.pack(">I", len(vdp1)) + b"".join(struct.pack(">2I", slot, n) + cmd for slot, (n, cmd) in vdp1.items())
        data += packet(srl_prof.PACKET_VDP1, body)
    return data


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
SLAVE_FUNCS = [(SLAVE_FN, 10, 400, 400)]
SYMS = srl_prof.Symbols([(MAIN, "main"), (OUTER, "Outer()"), (INNER_A, "InnerA()"), (INNER_B, "InnerB()"),
                         (SLAVE_FN, "SlaveWork()")],
                        [(0x06050000, "g_srl_prof")], [(STRINGS, RODATA)])


def trace_names(trace):
    return [(e["ph"], e["name"], e["ts"]) for e in trace["traceEvents"] if e["ph"] != "M"]


class DumpTest(unittest.TestCase):
    def setUp(self):
        self.profile = srl_prof.parse_block(make_dump(FUNCS, ARCS, slave_funcs=SLAVE_FUNCS))
        self.head = self.profile["head"]
        self.funcs = self.profile["master"]["funcs"]
        self.arcs = self.profile["master"]["arcs"]

    def test_parse_skips_free_rows(self):
        self.assertEqual(len(self.funcs), 3)
        self.assertEqual(len(self.arcs), 3)
        self.assertEqual(self.head["tick_hz"], TICK_HZ)

    def test_slave_tables(self):
        self.assertEqual(self.profile["slave"]["funcs"], [{"address": SLAVE_FN, "calls": 10, "total": 400, "self": 400}])

    def test_flat_removes_hook_cost_from_own_time(self):
        rows = {r["name"]: r for r in srl_prof.flat_rows(self.head, self.funcs, self.arcs, SYMS)}
        self.assertAlmostEqual(rows["Outer()"]["self"], 300)  # 330 raw - 30 calls made x 1 tick
        self.assertAlmostEqual(rows["InnerA()"]["self"], 1000)  # leaf: unchanged
        self.assertEqual(rows["InnerA()"]["calls"], 20)

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
        p = srl_prof.parse_block(make_dump(FUNCS, arcs))
        lines = dict(line.rsplit(" ", 1) for line in
                     srl_prof.folded_lines(p["head"], p["master"]["funcs"], p["master"]["arcs"], SYMS))
        self.assertEqual(lines["main;Outer();InnerB()"], "750")
        self.assertEqual(lines["main;InnerB()"], "250")

    def test_recursion_terminates(self):
        p = srl_prof.parse_block(make_dump(FUNCS, ARCS + [(INNER_A, OUTER, 5)]))  # Outer -> InnerA -> Outer
        self.assertTrue(srl_prof.folded_lines(p["head"], p["master"]["funcs"], p["master"]["arcs"], SYMS))

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

    def test_warnings_name_what_was_dropped(self):
        self.assertEqual(srl_prof.warnings(self.profile, SYMS), [])
        self.profile["slave"]["dropped_funcs"] = 3
        self.assertEqual(srl_prof.warnings(self.profile, SYMS), ["warning: slave dropped_funcs = 3"])

    def test_dump_events_only_up_to_count(self):
        events = [event(srl_prof.EVENT_INSTANT, VBLANK, 5)]
        self.assertEqual(srl_prof.parse_block(make_dump(FUNCS, ARCS, events))["events"], events)

    def test_event_names_come_from_the_elf(self):
        self.assertEqual(SYMS.string(WORKLOAD), "Workload")
        self.assertEqual(SYMS.string(VBLANK | 0x20000000), "VBlank")  # cache-through alias
        self.assertEqual(SYMS.string(0x1234), "0x0001234")

    def test_histogram_in_dump(self):
        # 16-byte bins from OUTER: bin 0 is Outer, bin 16 is InnerA (0x2100)
        hist = [0] * 32
        hist[0], hist[16] = 3, 9
        p = srl_prof.parse_block(make_dump(FUNCS, ARCS, hist=hist))
        self.assertEqual(p["hist"], {0: 3, 16: 9})
        self.assertEqual(srl_prof.sampled_functions(p["head"], p["hist"], SYMS), [("InnerA()", 9), ("Outer()", 3)])


class StreamTest(unittest.TestCase):
    EVENTS1 = [event(srl_prof.EVENT_INSTANT, VBLANK, 100), event(srl_prof.EVENT_BEGIN, WORKLOAD, 110),
               event(srl_prof.EVENT_END, WORKLOAD, 150)]
    EVENTS2 = [event(srl_prof.EVENT_INSTANT, VBLANK, 3500)]

    def test_reports_with_log_text_and_a_cut_packet(self):
        cut = report(FUNCS, ARCS, self.EVENTS1)[:20]  # abandoned mid-report
        data = (b"log line\n" + report(FUNCS[:1], ARCS[:1], self.EVENTS1) + cut + b"\xa5\x5a junk"
                + report(FUNCS, ARCS, self.EVENTS2, SLAVE_FUNCS))
        p = srl_prof.parse_stream(data)
        self.assertEqual(len(p["master"]["funcs"]), 3)  # tables of the last report
        self.assertEqual(len(p["slave"]["funcs"]), 1)
        self.assertEqual(p["events"], self.EVENTS1 + self.EVENTS2)
        self.assertEqual(p["stats"]["reports"], 2)
        self.assertGreater(p["stats"]["bad_packets"], 0)

    def test_histogram_counts_add_up_across_reports(self):
        data = report(FUNCS, ARCS, [], hist={0: 2, 16: 5}) + report(FUNCS, ARCS, [], hist={16: 1})
        self.assertEqual(srl_prof.parse_stream(data)["hist"], {0: 2, 16: 6})

    def test_no_report_is_an_error(self):
        with self.assertRaises(ValueError):
            srl_prof.parse_stream(b"just log text")

    def test_perfetto_slices_and_instants(self):
        p = srl_prof.parse_stream(report(FUNCS, ARCS, self.EVENTS1))
        self.assertEqual(trace_names(srl_prof.perfetto_trace(p["head"], p["events"], SYMS)), [
            ("i", "VBlank", 0.0), ("B", "Workload", 50.0), ("E", "Workload", 250.0)])

    def test_perfetto_closes_a_scope_whose_end_was_lost(self):
        events = [event(srl_prof.EVENT_BEGIN, WORKLOAD, 0), event(srl_prof.EVENT_BEGIN, VBLANK, 10),
                  event(srl_prof.EVENT_END, WORKLOAD, 20)]  # VBlank's End lost
        trace = trace_names(srl_prof.perfetto_trace(srl_prof.parse_header(make_header(1)), events, SYMS))
        self.assertEqual(trace[-2:], [("E", "VBlank", 100.0), ("E", "Workload", 100.0)])

    def test_tick_wrap(self):
        events = [event(srl_prof.EVENT_INSTANT, VBLANK, 0xFFFFFFF0), event(srl_prof.EVENT_INSTANT, VBLANK, 0x10)]
        ticks = [t for t, _, _ in srl_prof.timeline(events, SYMS)]
        self.assertEqual(ticks[1] - ticks[0], 0x20)


class Vdp1Test(unittest.TestCase):
    def test_vdp1_packets_add_up_and_keep_the_latest_command(self):
        polygon, sprite = command(), command(ctrl=0, size=(4 << 8) | 16, srca=0x200, coords=(10, 20, 0, 0, 0, 0, 0, 0))
        data = report(FUNCS, ARCS, [], vdp1={5: (3, sprite)}) + report(FUNCS, ARCS, [], vdp1={5: (2, polygon), 6: (1, sprite)})
        p = srl_prof.parse_stream(data)
        self.assertEqual(p["vdp1"], {5: 5, 6: 1})
        self.assertEqual(p["vdp1_commands"][5], polygon)

    def test_decode(self):
        self.assertEqual(srl_prof.decode_vdp1_command(command()),
                         "polygon, color 0x801f, covering (-160, -120)-(159, 119)")
        self.assertEqual(srl_prof.decode_vdp1_command(command(ctrl=0, pmod=5 << 3 | 3, size=(4 << 8) | 16, srca=0x200,
                                                              coords=(10, 20, 0, 0, 0, 0, 0, 0))),
                         "normal sprite, 32x16 texture at 0x01000, RGB, at (10, 20), half-transparent")
        self.assertEqual(srl_prof.decode_vdp1_command(command(ctrl=0x8000)), "end of list")

    def test_frames_finished_cut_and_idle(self):
        missed = STRINGS + 44
        syms = srl_prof.Symbols([], [], [(STRINGS, RODATA + b"VDP1 missed frame\0")])
        events = [event(srl_prof.EVENT_INSTANT, VBLANK, 0), event(srl_prof.EVENT_INSTANT, VDP1_END, 100),
                  event(srl_prof.EVENT_INSTANT, VBLANK, 3500),                # VDP1 starts, never ends
                  event(srl_prof.EVENT_INSTANT, VBLANK, 7000), event(srl_prof.EVENT_INSTANT, missed, 7000),
                  event(srl_prof.EVENT_INSTANT, VDP1_END, 8000),             # the next one finishes
                  event(srl_prof.EVENT_INSTANT, VBLANK, 10500),               # idle: no slice
                  event(srl_prof.EVENT_INSTANT, VBLANK, 14000)]
        self.assertEqual(srl_prof.vdp1_frames(events, syms), [(0, 100, True), (3500, 7000, False), (7000, 8000, True)])

    def test_perfetto_tracks(self):
        events = [event(srl_prof.EVENT_INSTANT, VBLANK, 0), event(srl_prof.EVENT_INSTANT, DMA0_END, 10),
                  event(srl_prof.EVENT_INSTANT, VDP1_END, 100)]
        trace = srl_prof.perfetto_trace(srl_prof.parse_header(make_header(1)), events, SYMS)["traceEvents"]
        drawing = [e for e in trace if e["name"] == "VDP1 drawing"]
        self.assertEqual([(e["tid"], e["ts"], e["dur"]) for e in drawing], [(2, 0.0, 500.0)])
        self.assertEqual({e["name"]: e["tid"] for e in trace if e["ph"] == "i"},
                         {"VBlank": 1, "SCU DMA 0 end": 3, "VDP1 draw end": 2})

    def test_dump_vdp1_slots(self):
        p = srl_prof.parse_block(make_dump(FUNCS, ARCS, hist=[0] * 32))
        self.assertEqual(p["vdp1"], {})


def parse_gmon(data):
    """Reads back what gmon_bytes writes: (histogram record, arcs)."""
    assert data[:4] == b"gmon" and struct.unpack_from(">I", data, 4)[0] == 1
    pos = 20
    hist, arcs = None, []
    while pos < len(data):
        tag = data[pos]
        if tag == 0:
            low, high, n, rate = struct.unpack_from(">4I", data, pos + 1)
            assert data[pos + 17:pos + 32].rstrip(b"\0") == b"seconds" and data[pos + 32:pos + 33] == b"s"
            bins = struct.unpack_from(f">{n}H", data, pos + 33)
            hist = (low, high, rate, bins)
            pos += 33 + 2 * n
        else:
            assert tag == 1
            arcs.append(struct.unpack_from(">3I", data, pos + 1))
            pos += 13
    return hist, arcs


class GmonTest(unittest.TestCase):
    def test_from_hook_times(self):
        p = srl_prof.parse_block(make_dump(FUNCS, ARCS))
        head, arcs = p["head"], p["master"]["arcs"]
        rows = srl_prof.flat_rows(head, p["master"]["funcs"], arcs, SYMS)
        hist, arcs = parse_gmon(srl_prof.gmon_bytes(head, rows, arcs, {}))
        low, high, rate, bins = hist
        self.assertEqual((low, high, rate), (OUTER, INNER_B + 4, TICK_HZ))
        self.assertEqual(bins[0], 300)  # Outer's own ticks (hook cost removed), at its first instruction
        self.assertEqual(bins[(INNER_A - OUTER) // 4], 1000)
        self.assertEqual(arcs, ARCS)

    def test_from_samples_scales_to_16_bits(self):
        head = srl_prof.parse_header(make_header(8, hist_bins=4, sample_hz=1000))
        (low, high, rate, bins), _ = parse_gmon(srl_prof.gmon_bytes(head, [], [], {1: 200000, 3: 100}))
        self.assertEqual((low, high), (OUTER, OUTER + 4 * 16))
        self.assertEqual(bins, (0, 50000, 0, 25))  # / 4
        self.assertEqual(rate, 250)  # rate / 4: counts still mean the same seconds


def parse_callgrind(text):
    """{function: (own costs, {callee: (calls, inclusive cost)})} from what callgrind_text writes."""
    functions, fn, callee, calls = {}, None, None, 0
    for line in text.splitlines():
        if line.startswith("fn="):
            fn = line[3:]
            functions.setdefault(fn, ([], {}))
        elif line.startswith("cfn="):
            callee = line[4:]
        elif line.startswith("calls="):
            calls = int(line[6:].split()[0])
        elif fn and line[:1].isdigit():
            costs = [int(v) for v in line.split()[2:]]  # after the address and line
            if callee:
                functions[fn][1][callee] = (calls, costs[0])
                callee = None
            else:
                functions[fn][0].append(costs)
    return functions


class CallgrindTest(unittest.TestCase):
    def locate(self, addresses):
        return {a: ("workload.cxx", a & 0xFFF) for a in addresses}

    def test_inclusive_time_matches_the_measured_total(self):
        p = srl_prof.parse_block(make_dump(FUNCS, ARCS))
        text = srl_prof.callgrind_text(p["head"], p["master"]["funcs"], p["master"]["arcs"], {}, SYMS, self.locate,
                                       elf_path="/src/prog.elf")
        self.assertIn("events: us\n", text)
        self.assertIn("ob=/src/prog.elf\n", text)  # for the Machine Code view
        self.assertIn("fn=Outer()\n0x2000 0 1500\ncfl=workload.cxx\ncfn=InnerA()\ncalls=20 0x2100 256\n0x2000 0 5100\n", text)
        f = parse_callgrind(text)
        self.assertEqual(f["(profiler hooks)"][0], [[(10 + 20 + 10) * 5]])  # one hook pair per call
        own, calls = f["Outer()"]
        self.assertEqual(own, [[1500]])  # 300 own ticks (hook cost removed) x 5 us
        # InnerA: its 1000-tick total + 20 hook pairs; Outer's own + calls = its 1530-tick total
        self.assertEqual(calls, {"InnerA()": (20, 5100), "InnerB()": (10, 1050)})
        self.assertEqual(own[0][0] + sum(c for _, c in calls.values()), 1530 * 5)
        self.assertEqual(f["main"][1], {"Outer()": (10, (1530 + 10) * 5)})

    def test_samples_land_on_their_lines(self):
        head = srl_prof.parse_header(make_header(8, hist_bins=4, sample_hz=1000))
        text = srl_prof.callgrind_text(head, [], [], {0: 7, 16: 3}, SYMS, self.locate)
        self.assertIn("events: Samples us\nsummary: 10 0\n", text)  # samples first: KCachegrind opens on them
        f = parse_callgrind(text)
        self.assertEqual(f["Outer()"][0], [[7, 0]])
        self.assertEqual(f["InnerA()"][0], [[3, 0]])  # bin 16 = OUTER + 16 * 16 = INNER_A

    def test_inlined_code_switches_file(self):
        head = srl_prof.parse_header(make_header(8, hist_bins=4, sample_hz=1000))
        locate = lambda addresses: {a: ("srl_timer.hpp", 9) if a == OUTER + 16 else ("workload.cxx", 1)
                                    for a in addresses}
        text = srl_prof.callgrind_text(head, [], [], {0: 7, 1: 3}, SYMS, locate)
        self.assertIn("fl=workload.cxx\nfn=Outer()\n0x2000 1 7 0\nfi=srl_timer.hpp\n0x2010 9 3 0\n", text)


class MapPathTest(unittest.TestCase):
    def test_only_a_leading_match_is_replaced(self):
        self.assertEqual(srl_prof.map_path("/saturn/a/saturn/b.cxx", "/saturn=/home/me"), "/home/me/a/saturn/b.cxx")
        self.assertEqual(srl_prof.map_path("/other/b.cxx", "/saturn=/home/me"), "/other/b.cxx")
        self.assertEqual(srl_prof.map_path("/saturn/b.cxx", None), "/saturn/b.cxx")


class Addr2lineTest(unittest.TestCase):
    def test_paths_with_spaces_discriminators_and_unknowns(self):
        out = "/src/My Game/a.cxx:12 (discriminator 3)\n??:0\n/saturn/b.hpp:?\n"
        with mock.patch("subprocess.run", return_value=mock.Mock(stdout=out)) as run:
            lines = srl_prof.addr2line("x.elf", [0x30, 0x10, 0x20, 0x10], tool="addr2line", source_map="/saturn=/home")
        self.assertEqual(run.call_args.kwargs["input"], "10\n20\n30")  # sorted, once each, in hex
        self.assertEqual(lines, {0x10: ("/src/My Game/a.cxx", 12), 0x20: ("???", 0), 0x30: ("/home/b.hpp", 0)})


class WatchTest(unittest.TestCase):
    def test_rewrites_the_output_as_reports_arrive(self):
        """A local socket plays ftx -g: a report, a pause, a report cut in two, then it closes."""
        server = socket.create_server(("localhost", 0))
        second = report(FUNCS, ARCS, StreamTest.EVENTS2)

        def ftx():
            conn, _ = server.accept()
            with conn:
                conn.sendall(b"log line\n" + report(FUNCS[:1], ARCS[:1], StreamTest.EVENTS1))
                time.sleep(0.5)
                conn.sendall(second[:10])
                time.sleep(0.5)
                conn.sendall(second[10:])

        thread = threading.Thread(target=ftx)
        thread.start()
        with tempfile.TemporaryDirectory() as tmp:
            args = argparse.Namespace(elf="test.elf", data=os.path.join(tmp, "capture.bin"),
                                      output=os.path.join(tmp, "flat.txt"), watch=0.3, host="localhost",
                                      port=server.getsockname()[1], format="flat", cpu="master", top=40,
                                      vram=None, source_map=None)
            srl_prof.watch(args, SYMS)
            thread.join()
            server.close()
            with open(args.data, "rb") as f:
                self.assertEqual(srl_prof.parse_stream(f.read())["stats"]["reports"], 2)  # every byte recorded
            with open(args.output) as f:
                flat = f.read()
            self.assertIn("InnerB()", flat)  # tables of the second report
            self.assertFalse(os.path.exists(args.output + ".tmp"))


if __name__ == "__main__":
    unittest.main()
