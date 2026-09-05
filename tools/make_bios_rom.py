#!/usr/bin/env python3
"""Pack linked flash LOAD segments into a 64 MiB QEMU virt PFlash image."""
from __future__ import annotations
import argparse
from pathlib import Path
import struct

ROM_SIZE = 64 * 1024 * 1024
MAX_CODE = 16 * 1024 * 1024


def pack_elf(elf: bytes) -> bytes:
    if (len(elf) < 64 or elf[:7] != b"\x7fELF\x02\x01\x01"
            or struct.unpack_from("<HH", elf, 16) != (2, 183)
            or struct.unpack_from("<Q", elf, 24)[0] != 0
            or struct.unpack_from("<HH", elf, 52) != (64, 56)):
        raise ValueError("Expected linked AArch64 ELF64 firmware with entry at flash address zero")
    table = struct.unpack_from("<Q", elf, 32)[0]
    count = struct.unpack_from("<H", elf, 56)[0]
    if not 1 <= count <= 32 or table + count * 56 > len(elf):
        raise ValueError("Invalid program header table")
    segments = []
    boot = False
    for index in range(count):
        kind, flags, offset, virtual, physical, size, memory, _ = struct.unpack_from(
            "<IIQQQQQQ", elf, table + index * 56)
        if kind != 1 or not size:
            continue
        if size > memory or offset + size > len(elf) or physical + size > MAX_CODE:
            raise ValueError("Invalid or oversized flash segment")
        if any(physical < end and start < physical + size for start, end, _ in segments):
            raise ValueError("Overlapping flash segments")
        boot |= physical == 0 and virtual == 0 and bool(flags & 1)
        segments.append((physical, physical + size, elf[offset:offset + size]))
    if not boot:
        raise ValueError("No executable reset vector at flash address zero")
    image = bytearray(b"\xff") * max(end for _, end, _ in segments)
    for start, end, data in segments:
        image[start:end] = data
    return bytes(image)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    code = pack_elf(args.elf.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("wb") as output:
        output.write(code)
        output.truncate(ROM_SIZE)
    print(f"Packed {len(code)} firmware bytes into {args.output}")


if __name__ == "__main__":
    main()
