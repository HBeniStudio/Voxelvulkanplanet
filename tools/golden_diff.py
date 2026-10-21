#!/usr/bin/env python3
"""Compare two PNG images with per-channel tolerance (text summary only)."""

from __future__ import annotations

import argparse
import struct
import sys
import zlib
from pathlib import Path


def _paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def _unfilter_row(filter_type: int, row: bytearray, prev: bytes | None, bpp: int) -> bytes:
    if filter_type == 0:
        return bytes(row)
    if filter_type == 1:
        for i in range(bpp, len(row)):
            row[i] = (row[i] + row[i - bpp]) & 0xFF
        return bytes(row)
    if filter_type == 2:
        if prev is None:
            return bytes(row)
        for i in range(len(row)):
            row[i] = (row[i] + prev[i]) & 0xFF
        return bytes(row)
    if filter_type == 3:
        for i in range(len(row)):
            left = row[i - bpp] if i >= bpp else 0
            up = prev[i] if prev is not None else 0
            row[i] = (row[i] + ((left + up) // 2)) & 0xFF
        return bytes(row)
    if filter_type == 4:
        for i in range(len(row)):
            left = row[i - bpp] if i >= bpp else 0
            up = prev[i] if prev is not None else 0
            up_left = prev[i - bpp] if prev is not None and i >= bpp else 0
            row[i] = (row[i] + _paeth(left, up, up_left)) & 0xFF
        return bytes(row)
    raise ValueError(f"unsupported scanline filter {filter_type}")


def read_png_rgb(path: Path) -> tuple[int, int, list[int]]:
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG")

    width = height = None
    idat_parts: list[bytes] = []
    pos = 8
    while pos < len(data):
        if pos + 8 > len(data):
            break
        length = struct.unpack(">I", data[pos : pos + 4])[0]
        chunk_type = data[pos + 4 : pos + 8]
        chunk_data = data[pos + 8 : pos + 8 + length]
        pos += 12 + length

        if chunk_type == b"IHDR":
            width, height = struct.unpack(">II", chunk_data[:8])
        elif chunk_type == b"IDAT":
            idat_parts.append(chunk_data)

    if not idat_parts:
        raise ValueError(f"{path}: missing IDAT")

    raw = zlib.decompress(b"".join(idat_parts))
    rgb: list[int] = []
    bpp = 3
    stride = width * bpp + 1
    prev: bytes | None = None
    offset = 0
    for _row in range(height):
        filter_type = raw[offset]
        filtered = bytearray(raw[offset + 1 : offset + stride])
        row_data = _unfilter_row(filter_type, filtered, prev, bpp)
        rgb.extend(row_data)
        prev = row_data
        offset += stride

    if width is None or height is None:
        raise ValueError(f"{path}: missing IHDR")
    expected = width * height * 3
    if len(rgb) != expected:
        raise ValueError(f"{path}: expected {expected} rgb bytes, got {len(rgb)}")
    return width, height, rgb


def main() -> int:
    parser = argparse.ArgumentParser(description="Golden PNG diff with tolerance")
    parser.add_argument("candidate")
    parser.add_argument("golden")
    parser.add_argument("--tolerance", type=int, default=2)
    parser.add_argument("--max-diff-fraction", type=float, default=0.005)
    args = parser.parse_args()

    cand_path = Path(args.candidate)
    gold_path = Path(args.golden)
    cw, ch, cand = read_png_rgb(cand_path)
    gw, gh, gold = read_png_rgb(gold_path)
    if (cw, ch) != (gw, gh):
        print(f"FAIL size mismatch candidate={cw}x{ch} golden={gw}x{gh}")
        return 1

    pixels = cw * ch
    differing = 0
    max_delta = 0
    for i in range(0, len(cand), 3):
        dr = abs(cand[i] - gold[i])
        dg = abs(cand[i + 1] - gold[i + 1])
        db = abs(cand[i + 2] - gold[i + 2])
        delta = max(dr, dg, db)
        max_delta = max(max_delta, delta)
        if delta > args.tolerance:
            differing += 1

    fraction = differing / pixels if pixels else 0.0
    print(f"candidate={cand_path}")
    print(f"golden={gold_path}")
    print(f"size={cw}x{ch} pixels={pixels}")
    print(f"tolerance={args.tolerance} max_channel_delta={max_delta}")
    print(f"differing_pixels={differing} ({fraction * 100:.4f}%)")

    if fraction > args.max_diff_fraction:
        print("FAIL too many differing pixels")
        return 1
    print("PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
