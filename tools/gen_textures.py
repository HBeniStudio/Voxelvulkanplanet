#!/usr/bin/env python3
"""Generate procedural 16x16 placeholder PNGs for assets/textures/."""

from __future__ import annotations

import hashlib
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "textures"

# Face names referenced by data/blocks.json
TEXTURE_NAMES = [
    "stone",
    "dirt",
    "grass_top",
    "grass_side",
    "sand",
    "water",
    "log_top",
    "log_side",
    "leaves",
    "coal_ore",
    "iron_ore",
    "bedrock",
    "planks",
    "crafting_table_top",
    "crafting_table_side",
]


def _png_chunk(tag: bytes, data: bytes) -> bytes:
    crc = zlib.crc32(tag + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)


def write_png_rgba(path: Path, width: int, height: int, rgba: bytes) -> None:
    assert len(rgba) == width * height * 4
    raw = b"".join(b"\x00" + rgba[y * width * 4 : (y + 1) * width * 4] for y in range(height))
    compressed = zlib.compress(raw, 9)
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n"
    png += _png_chunk(b"IHDR", ihdr)
    png += _png_chunk(b"IDAT", compressed)
    png += _png_chunk(b"IEND", b"")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def color_for_name(name: str) -> tuple[int, int, int]:
    digest = hashlib.sha256(name.encode()).digest()
    base = (digest[0], digest[1], digest[2])
    # Slight name-based tint bands for readability in-game.
    if "grass" in name:
        return (40 + base[0] % 40, 120 + base[1] % 80, 30 + base[2] % 40)
    if "water" in name:
        return (30 + base[0] % 30, 80 + base[1] % 60, 180 + base[2] % 50)
    if "log" in name:
        return (90 + base[0] % 40, 60 + base[1] % 30, 30 + base[2] % 20)
    if "leaves" in name:
        return (35 + base[0] % 30, 100 + base[1] % 50, 35 + base[2] % 30)
    if "ore" in name:
        return (80 + base[0] % 40, 80 + base[1] % 40, 80 + base[2] % 40)
    if "bedrock" in name:
        return (25, 25, 28)
    if "planks" in name or "crafting" in name:
        return (150 + base[0] % 40, 110 + base[1] % 30, 60 + base[2] % 30)
    if "sand" in name:
        return (200 + base[0] % 30, 180 + base[1] % 30, 120 + base[2] % 30)
    if "dirt" in name:
        return (100 + base[0] % 40, 70 + base[1] % 30, 40 + base[2] % 20)
    return (100 + base[0] % 80, 100 + base[1] % 80, 100 + base[2] % 80)


def make_texture(name: str, size: int = 16) -> bytes:
    r0, g0, b0 = color_for_name(name)
    digest = hashlib.sha256((name + ":v1").encode()).digest()
    pixels = bytearray()
    for y in range(size):
        for x in range(size):
            checker = ((x ^ y) & 1) * 12
            noise = digest[(x + y * size) % len(digest)] % 16
            r = max(0, min(255, r0 + checker + noise - 8))
            g = max(0, min(255, g0 + checker + (noise // 2) - 4))
            b = max(0, min(255, b0 + checker + (noise // 3) - 4))
            pixels.extend((r, g, b, 255))
    return bytes(pixels)


def main() -> None:
    for name in TEXTURE_NAMES:
        path = OUT_DIR / f"{name}.png"
        rgba = make_texture(name)
        write_png_rgba(path, 16, 16, rgba)
        print(f"wrote {path}")


if __name__ == "__main__":
    main()
