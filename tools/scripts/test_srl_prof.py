#!/usr/bin/env python3
"""Offline tests for srl_prof.py: a hand-built dump with known numbers, no hardware needed.

Run: python3 tools/scripts/test_srl_prof.py
"""

import os
import struct
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import srl_prof  # noqa: E402

MAIN, OUTER, INNER_A, INNER_B = 0x1000, 0x2000, 0x2100, 0x2200
TICK_HZ = 200000  # 5 us per tick


def make_dump(funcs, arcs, func_capacity=8, arc_capacity=8, hook_cost_ticks=1.0, frames=10, elapsed=10000):
    head = srl_prof.HEADER.pack(b"SRLP", 1, 0x06050000, TICK_HZ, int(hook_cost_ticks * 65536), 1, frames,
                                elapsed, func_capacity, arc_capacity, 64, 0, 0, 0, 0, 0)
    body = b"".join(srl_prof.FUNC.pack(*f) for f in funcs)
    body += b"\0" * (srl_prof.FUNC.size * (func_capacity - len(funcs)))
    body += b"".join(srl_prof.ARC.pack(*a) for a in arcs)
    body += b"\0" * (srl_prof.ARC.size * (arc_capacity - len(arcs)))
    return head + body


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
                        [(0x06050000, "g_srl_prof")])


class SrlProfTest(unittest.TestCase):
    def setUp(self):
        self.head, self.funcs, self.arcs = srl_prof.parse_block(make_dump(FUNCS, ARCS))

    def test_parse_skips_free_rows(self):
        self.assertEqual(len(self.funcs), 3)
        self.assertEqual(len(self.arcs), 3)
        self.assertEqual(self.head["tick_hz"], TICK_HZ)

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
        head, funcs, arcs = srl_prof.parse_block(make_dump(FUNCS, arcs))
        lines = dict(line.rsplit(" ", 1) for line in srl_prof.folded_lines(head, funcs, arcs, SYMS))
        self.assertEqual(lines["main;Outer();InnerB()"], "750")
        self.assertEqual(lines["main;InnerB()"], "250")

    def test_recursion_terminates(self):
        arcs = ARCS + [(INNER_A, OUTER, 5)]  # Outer -> InnerA -> Outer cycle
        head, funcs, arcs = srl_prof.parse_block(make_dump(FUNCS, arcs))
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


if __name__ == "__main__":
    unittest.main()
