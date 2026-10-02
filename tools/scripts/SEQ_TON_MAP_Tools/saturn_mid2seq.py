#!/usr/bin/env python3

"""
saturn_mid2seq.py by Frogbull version 0.1
(nearly fully based on mid2seq by João Felipe Santos: https://github.com/jfsantos/mid2seq)

Usage:
    python saturn_mid2seq.py <input.mid> <OUTPUT.SEQ>

Description:
  - Converts a Standard MIDI File (format 0 only) into a Sega Saturn SEQ file
  - Forcing all instruments to use only TON Bank 1

"""

import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List, Tuple, Set

# ---------- Helpers: big-endian IO ----------
def be_u16(x: int) -> bytes:
    return bytes([ (x >> 8) & 0xFF, x & 0xFF] )

def be_u32(x: int) -> bytes:
    return bytes([ (x >> 24) & 0xFF, (x >> 16) & 0xFF, (x >> 8) & 0xFF, x & 0xFF] )

def read_u16_be(buf: bytes, off: int) -> Tuple[int, int]:
    return ((buf[off] << 8) | buf[off + 1], off + 2)

def read_u32_be(buf: bytes, off: int) -> Tuple[int, int]:
    return ((buf[off] << 24) | (buf[off + 1] << 16) | (buf[off + 2] << 8) | buf[off + 3], off + 4)

def read_vlq(buf: bytes, off: int) -> Tuple[int, int]:
    value = 0
    b = buf[off]
    off += 1
    value = b & 0x7F
    while b & 0x80:
        b = buf[off]
        off += 1
        value = (value << 7) | (b & 0x7F)
    return value, off

# ---------- SEQ building blocks ----------
def write_large_delta_events(out: bytearray, delta: int) -> int:
    # 0x8F = +0x1000, 0x8E = +0x800, 0x8D = +0x200
    while delta >= 0x1000:
        out.append(0x8F)
        delta -= 0x1000
    while delta >= 0x800:
        out.append(0x8E)
        delta -= 0x800
    while delta >= 0x200:
        out.append(0x8D)
        delta -= 0x200
    return delta

def write_extended_gate(out: bytearray, gate: int) -> int:
    # 0x8B=+0x2000, 0x8A=+0x1000, 0x89=+0x800, 0x88=+0x200
    while gate >= 0x2000:
        out.append(0x8B)
        gate -= 0x2000
    while gate >= 0x1000:
        out.append(0x8A)
        gate -= 0x1000
    while gate >= 0x800:
        out.append(0x89)
        gate -= 0x800
    while gate >= 0x200:
        out.append(0x88)
        gate -= 0x200
    return gate

@dataclass
class TrackEvent:
    absolute_time: int
    status: int
    data1: int = 0
    data2: int = 0
    gate_time: int = 0

def is_note_off(status: int, d2: int) -> bool:
    t = status & 0xF0
    return t == 0x80 or (t == 0x90 and d2 == 0)

def event_sort_key(ev: TrackEvent) -> Tuple[int, int]:
    if is_note_off(ev.status, ev.data2):
        prio = 0
    else:
        prio = 1
    return (ev.absolute_time, prio)

# ---------- MIDI parsing (format 0 only) ----------
@dataclass
class TempoEvent:
    step_time: int
    mspb: int # microseconds per beat

def parse_midi_format0(path: Path) -> Tuple[int, List[TrackEvent], List[TempoEvent], Set[int]]:
    buf = path.read_bytes()
    off = 0

    # Header chunk
    if buf[off:off+4] != b"MThd":
        raise ValueError("Not a MIDI file (missing MThd).")
    off += 4

    header_len, off = read_u32_be(buf, off)
    fmt, off2 = read_u16_be(buf, off)
    ntrks, off2 = read_u16_be(buf, off2)
    division, off2 = read_u16_be(buf, off2)
    off = off + header_len # skip any extra header data

    if fmt != 0:
        raise ValueError("Only MIDI format 0 is supported")
    if ntrks < 1:
        raise ValueError("No tracks in MIDI")

    # Track chunk
    if buf[off:off+4] != b"MTrk":
        raise ValueError("Missing MTrk.")
    off += 4
    trk_len, off = read_u32_be(buf, off)
    trk_end = off + trk_len

    events: List[TrackEvent] = []
    tempo_raw: List[Tuple[int, int]] = [] # (absolute_time, mspb)
    used_channels: Set[int] = set()

    last_status = 0
    current_time = 0

    while off < trk_end:
        delta, off = read_vlq(buf, off)
        current_time += delta

        status = buf[off]
        off += 1

        if (status & 0x80) == 0:
            # running status: status byte is actually data
            off -= 1
            status = last_status
        else:
            last_status = status

        etype = status & 0xF0
        ch = status & 0x0F

        if 0xB0 <= status <= 0xBF:
            used_channels.add(ch)

        if etype in (0x80, 0x90, 0xA0, 0xB0, 0xE0):
            d1 = buf[off]; d2 = buf[off+1]; off += 2
            events.append(TrackEvent(current_time, status, d1, d2))
        elif etype in (0xC0, 0xD0):
            d1 = buf[off]; off += 1
            events.append(TrackEvent(current_time, status, d1, 0))
        elif status == 0xFF:
            # meta
            meta_type = buf[off]; off += 1
            length, off = read_vlq(buf, off)
            if meta_type == 0x51 and length == 3:
                mspb = (buf[off] << 16) | (buf[off+1] << 8) | buf[off+2]
                tempo_raw.append((current_time, mspb))
            off += length
        elif status in (0xF0, 0xF7):
            # sysex: skip
            length, off = read_vlq(buf, off)
            off += length
        else:
            # unsupported/system: best-effort skip (rare in format0 tracks)
            pass

    # Store step_time from previous tempo
    tempo_events: List[TempoEvent] = []
    last_tempo_time = 0
    for t, mspb in tempo_raw[:255]:
        tempo_events.append(TempoEvent(step_time=t - last_tempo_time, mspb=mspb))
        last_tempo_time = t

    return division, events, tempo_events, used_channels

# ---------- Bank forcing (bank 1 only) ----------
def force_bank1(events: List[TrackEvent], used_channels: Set[int]) -> List[TrackEvent]:
    # 1) Patch any existing Bank Select CC:
    #    CC 0x00 (MSB) and CC 0x20 (LSB) forced to 1
    for ev in events:
        if 0xB0 <= ev.status <= 0xBF:
            if ev.data1 == 0x00 or ev.data1 == 0x20:
                ev.data2 = 0x01

    # 2) Inject at time 0 for each used channel:
    #    Bn 00 01  (MSB=1)
    #    Bn 20 01  (LSB=1)
    injected: List[TrackEvent] = []
    for ch in sorted(used_channels):
        injected.append(TrackEvent(0, 0xB0 | ch, 0x00, 0x01))
        injected.append(TrackEvent(0, 0xB0 | ch, 0x20, 0x01))

    return injected + events

# ---------- Gate computation (same logic as C) ----------
def compute_gates(events: List[TrackEvent]) -> None:
    active = [[-1 for _ in range(128)] for _ in range(16)]

    for i, ev in enumerate(events):
        etype = ev.status & 0xF0
        ch = ev.status & 0x0F

        if etype == 0x90 and ev.data2 > 0:
            prev = active[ch][ev.data1]
            if prev != -1:
                events[prev].gate_time = ev.absolute_time - events[prev].absolute_time
            active[ch][ev.data1] = i

        elif etype == 0x80 or (etype == 0x90 and ev.data2 == 0):
            note_on_idx = active[ch][ev.data1]
            if note_on_idx != -1:
                events[note_on_idx].gate_time = ev.absolute_time - events[note_on_idx].absolute_time
                ev.status = 0x00 # mark for removal
                active[ch][ev.data1] = -1

# ---------- Tempo track synthesis (same logic as C) ----------
def synthesize_tempo_track(events: List[TrackEvent], tempo_events: List[TempoEvent]) -> List[TempoEvent]:
    if not tempo_events:
        return []

    # first musical event time (non-meta; our list already excludes meta)
    first_musical = 0
    if events:
        first_musical = min(ev.absolute_time for ev in events if ev.status != 0x00)

    total_song_time = 0
    if events:
        total_song_time = max(ev.absolute_time for ev in events if ev.status != 0x00)

    mspb = tempo_events[0].mspb

    # exactly two events
    return [
        TempoEvent(step_time=first_musical, mspb=mspb),
        TempoEvent(step_time=max(0, total_song_time - first_musical), mspb=mspb),
    ]

# ---------- Write SEQ ----------
def write_seq(output_path: Path, division: int, events: List[TrackEvent], tempo_events: List[TempoEvent]) -> None:
    out = bytearray()

    # Bank header: num_songs=1, song_ptr=6 (big endian)
    out += be_u16(1)
    out += be_u32(6)

    # SEQ header (8 bytes): resolution, num_tempo_events, data_offset, tempo_loop_offset
    tempo_count = len(tempo_events)
    data_offset = 8 + tempo_count * 8
    tempo_loop_offset = (8 + 8) if tempo_count > 0 else 0  # start of 2nd tempo event

    out += be_u16(division)
    out += be_u16(tempo_count)
    out += be_u16(data_offset)
    out += be_u16(tempo_loop_offset)

    # Tempo track
    for te in tempo_events:
        out += be_u32(te.step_time)
        out += be_u32(te.mspb)

    # Sort events by absolute time (note-offs first on same tick)
    events_sorted = sorted(events, key=event_sort_key)

    last_time = 0
    for ev in events_sorted:
        if ev.status == 0x00:
            continue

        delta = ev.absolute_time - last_time
        last_time = ev.absolute_time

        delta = write_large_delta_events(out, delta)

        etype = ev.status & 0xF0
        ch = ev.status & 0x0F

        if etype == 0x90 and ev.data2 > 0:
            gate = ev.gate_time
            gate = write_extended_gate(out, gate)

            ctl = ch
            if delta >= 256:
                ctl |= 0x20
                delta -= 256
            if gate >= 256:
                ctl |= 0x40
                gate -= 256

            out.append(ctl)
            out.append(ev.data1)          # key
            out.append(ev.data2)          # velocity
            out.append(gate & 0xFF)
            out.append(delta & 0xFF)

        else:
            while delta >= 256:
                out.append(0x8C)
                delta -= 256

            out.append(ev.status)

            if etype in (0xB0, 0xA0):
                out.append(ev.data1)
                out.append(ev.data2)
            elif etype == 0xE0:
                # write MSB as the value
                out.append(ev.data2)
            else:
                out.append(ev.data1)

            out.append(delta & 0xFF)

    out.append(0x83)  # end of track marker
    output_path.write_bytes(out)

def main() -> None:
    if len(sys.argv) != 3:
        print("Usage: python midi2seq.py input.mid output.seq")
        raise SystemExit(1)

    in_mid = Path(sys.argv[1])
    out_seq = Path(sys.argv[2])

    division, events, tempo_events, used_channels = parse_midi_format0(in_mid)

    # Force bank 1: patch CC00/CC20 and inject at time 0 for used channels
    events = force_bank1(events, used_channels)

    # Gate calculation requires the raw list, then we sort for output
    compute_gates(events)

    # Tempo track synthesis
    tempo_events = synthesize_tempo_track(events, tempo_events)

    write_seq(out_seq, division, events, tempo_events)
    print("OK:", out_seq)


if __name__ == "__main__":
    main()
