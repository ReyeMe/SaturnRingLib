#!/usr/bin/env python3

"""
saturn_map_generator by Frogbull version 0.2
(thanks to Andreas Scholl for helping me understand the MAP structure)

Usage:
    python saturn_map_generator.py <input_folder> <OUTPUT.MAP>

Description:
  - Scan <input_folder> for *.SEQ and *.TON files
  - Find the largest SEQ file and the largest TON file
  - Generate a Binary Sega Saturn MAP with:
    *Line N°1: DSP work + DSP program (fixed)
    *Line N°2: SEQ id=0 + TON id=0 (legacy block, got some problem without it)
    *Line N°3: SEQ id=1 + TON id=1 (sizes computed from largest SEQ/TON, TON start shifted accordingly)
    *End Line: ffff
  - Uses safe alignment (0x20) for starts/sizes

"""

import sys
from pathlib import Path
from dataclasses import dataclass

# Fixed entries for DSP
DSP_WORK_CMD   = 0x30
DSP_WORK_START = 0x00C000
DSP_WORK_SIZE  = 0x010040

DSP_PROG_CMD   = 0x20
DSP_PROG_START = 0x01C040
DSP_PROG_SIZE  = 0x000540

# Legacy compat block (probably pure waste of memory)
SEQ0_CMD   = 0x10
SEQ0_START = 0x024580
SEQ0_SIZE  = 0x00016E

TON0_CMD   = 0x00
TON0_START = 0x0246EE
TON0_SIZE  = 0x0088EC

# Main banks (bank 1)
SEQ1_CMD   = 0x11
SEQ1_START = 0x02CFDC
TON1_CMD   = 0x01

FLAGS = 0x80
ALIGN = 0x20

def align_up(v: int, a: int) -> int:
    return (v + (a - 1)) & ~(a - 1)

@dataclass
class MapEntry:
    cmd: int
    start: int
    size: int
    flags: int = FLAGS

    def to_bytes(self) -> bytes:
        if not (0 <= self.cmd <= 0xFF):
            raise ValueError("cmd must be 8-bit")
        if not (0 <= self.start <= 0xFFFFFF):
            raise ValueError("start must be 24-bit")
        if not (0 <= self.size <= 0xFFFFFF):
            raise ValueError("size must be 24-bit")
        if not (0 <= self.flags <= 0xFF):
            raise ValueError("flags must be 8-bit")

        return bytes([
            self.cmd & 0xFF,
            (self.start >> 16) & 0xFF,
            (self.start >> 8) & 0xFF,
            (self.start >> 0) & 0xFF,
            self.flags & 0xFF,
            (self.size >> 16) & 0xFF,
            (self.size >> 8) & 0xFF,
            (self.size >> 0) & 0xFF,
        ])

def find_largest_file(folder: Path, ext: str) -> tuple[Path, int]:
    files = sorted(folder.glob(f"*{ext}"))
    if not files:
        raise FileNotFoundError(f"No {ext} files found in {folder}")
    best = max(files, key=lambda p: p.stat().st_size)
    return best, best.stat().st_size

def main() -> None:
    if len(sys.argv) != 3:
        print("Usage: python saturn_map_generator.py <input_folder> <OUTPUT.MAP>")
        raise SystemExit(1)

    folder = Path(sys.argv[1])
    out_map = Path(sys.argv[2])

    if not folder.is_dir():
        print(f"Error: input_folder is not a folder: {folder}")
        raise SystemExit(1)

    largest_seq_path, largest_seq_size = find_largest_file(folder, ".SEQ")
    largest_ton_path, largest_ton_size = find_largest_file(folder, ".TON")

    seq1_size_aligned = align_up(largest_seq_size, ALIGN)
    ton1_size_aligned = align_up(largest_ton_size, ALIGN)

    # Compute TON1 start; align start as well
    ton1_start = align_up(SEQ1_START + seq1_size_aligned, ALIGN)

    if ton1_start > 0xFFFFFF:
        print("Error: Computed TON1 start exceeds 24-bit range")
        raise SystemExit(1)

    entries = [
        MapEntry(DSP_WORK_CMD, DSP_WORK_START, DSP_WORK_SIZE),
        MapEntry(DSP_PROG_CMD, DSP_PROG_START, DSP_PROG_SIZE),
        MapEntry(SEQ0_CMD, SEQ0_START, SEQ0_SIZE),
        MapEntry(TON0_CMD, TON0_START, TON0_SIZE),
        MapEntry(SEQ1_CMD, SEQ1_START, seq1_size_aligned),
        MapEntry(TON1_CMD, ton1_start, ton1_size_aligned),
    ]

    blob = bytearray()
    for e in entries:
        blob += e.to_bytes()

    blob += b"\xFF" * 2 # Terminator (2 bytes of 0xFF)
    out_map.write_bytes(blob)

    print("OK")
    print(f"Largest SEQ: {largest_seq_path.name} ({largest_seq_size} bytes) -> reserved 0x{seq1_size_aligned:06X}")
    print(f"Largest TON: {largest_ton_path.name} ({largest_ton_size} bytes) -> reserved 0x{ton1_size_aligned:06X}")
    print(f"SEQ1 start: 0x{SEQ1_START:06X}")
    print(f"TON1 start: 0x{ton1_start:06X}")
    print(f"Wrote binary MAP: {out_map} ({len(blob)} bytes)")

if __name__ == "__main__":
    main()
