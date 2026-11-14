#!/usr/bin/env python3
"""Writes the TGA and WAV files of the media campaign into cd/data/.

The pixel and sample formulas must match the expected values in Tests/src/testsTGA.hpp and testsWave.hpp.
Run it from anywhere: python3 Tests/campaigns/media/make_data.py
"""
import os
import struct

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "cd", "data")
W, H = 8, 4


def plain_key(x, y):
    return x * 3 + y * 5


def rle_key(x, y):
    # A run of 4 equal pixels, then 4 different ones, on every row
    return (0 if x < 4 else x) + y * 9


def true_rgb(k):
    return (k * 7) & 0xFF, (k * 13) & 0xFF, (255 - k * 5) & 0xFF


def palette_rgb(j):
    return (j * 8) & 0xFF, (255 - j * 8) & 0xFF, (0x40 + j) & 0xFF


def file_order(origin):
    """Image (x, y) coordinates (y = 0 at the top) in the order the file stores them."""
    right, top = "R" in origin, "T" in origin
    for row in range(H):
        y = row if top else H - 1 - row
        yield [((W - 1 - col) if right else col, y) for col in range(W)]


def pixel_bytes(value, bpp):
    if bpp == 8:
        return bytes([value])
    r, g, b = value
    if bpp == 16:
        return struct.pack("<H", 0x8000 | (r >> 3) << 10 | (g >> 3) << 5 | (b >> 3))
    if bpp == 24:
        return bytes([b, g, r])
    return bytes([b, g, r, 255])


def rle_row(pixels):
    """RLE packets for one row: runs of 2 or more become RLE packets, the rest raw packets."""
    out, i = b"", 0
    while i < len(pixels):
        run = 1
        while i + run < len(pixels) and pixels[i + run] == pixels[i] and run < 128:
            run += 1
        if run > 1:
            out += bytes([0x80 | (run - 1)]) + pixels[i]
            i += run
        else:
            j = i
            while j < len(pixels) and (j + 1 >= len(pixels) or pixels[j + 1] != pixels[j]) and j - i < 128:
                j += 1
            out += bytes([j - i - 1]) + b"".join(pixels[i:j])
            i = j
    return out


def tga(name, image_type, bpp, origin, key, palette_length=0):
    paletted = palette_length > 0
    rle = image_type in (9, 10)
    descriptor = (0x10 if "R" in origin else 0) | (0x20 if "T" in origin else 0) | (8 if bpp == 32 else 0)
    header = struct.pack("<BBBHHBHHHHBB", 0, 1 if paletted else 0, image_type,
                         0, palette_length, 24 if paletted else 0, 0, 0, W, H, bpp, descriptor)
    palette = b"".join(bytes(reversed(palette_rgb(j))) for j in range(palette_length))
    data = b""
    for row in file_order(origin):
        values = [key(x, y) % palette_length if paletted else true_rgb(key(x, y)) for x, y in row]
        pixels = [pixel_bytes(v, bpp) for v in values]
        data += rle_row(pixels) if rle else b"".join(pixels)
    open(os.path.join(OUT, name), "wb").write(header + palette + data)


def wav(name, channels, bits, frames, extra_chunk=False):
    if bits == 8:
        samples = [(f * 7 + c * 100) & 0xFF for f in range(frames) for c in range(channels)]
        data = bytes(samples)
    else:
        samples = [f * 300 - 5000 if c == 0 else 5000 - f * 200 for f in range(frames) for c in range(channels)]
        data = struct.pack("<%dh" % len(samples), *samples)
    rate = 11025
    fmt = struct.pack("<4sIHHIIHH", b"fmt ", 16, 1, channels, rate, rate * channels * bits // 8, channels * bits // 8, bits)
    chunks = fmt
    if extra_chunk:
        chunks += struct.pack("<4sI", b"LIST", 8) + b"INFOtest"
    chunks += struct.pack("<4sI", b"data", len(data)) + data
    open(os.path.join(OUT, name), "wb").write(struct.pack("<4sI4s", b"RIFF", 4 + len(chunks), b"WAVE") + chunks)


os.makedirs(OUT, exist_ok=True)
tga("TC16BL.TGA", 2, 16, "BL", plain_key)
tga("TC24TL.TGA", 2, 24, "TL", plain_key)
tga("TC32TR.TGA", 2, 32, "TR", plain_key)
tga("TC24BR.TGA", 2, 24, "BR", plain_key)
tga("TCRLE24.TGA", 10, 24, "BL", rle_key)
tga("TCRLE32.TGA", 10, 32, "TL", rle_key)
tga("PAL256.TGA", 1, 8, "BL", plain_key, 256)
tga("PAL16.TGA", 1, 8, "TL", plain_key, 16)
tga("PAL16BR.TGA", 1, 8, "BR", plain_key, 16)
tga("PALRLE.TGA", 9, 8, "BL", rle_key, 256)
tga("PALRLE16.TGA", 9, 8, "TL", rle_key, 16)
wav("M8.WAV", 1, 8, 64)
wav("M16.WAV", 1, 16, 64, extra_chunk=True)
wav("S8.WAV", 2, 8, 64)
wav("S16.WAV", 2, 16, 64)
