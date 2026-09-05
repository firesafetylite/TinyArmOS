#!/usr/bin/env python3
"""Build a native TinyGPT disk without EDK II; or prepare an offline disk update.

Only returns in-memory updated bytes for existing disks. It never edits a VM.
"""
from __future__ import annotations
import argparse
import importlib.util
from pathlib import Path
import runpy
import struct
import tempfile

ROOT = Path(__file__).resolve().parents[1]
NATIVE_NAME = b"TINYGPT ELF"


def validate_system(image: bytes) -> None:
    if (not 64 <= len(image) <= 8 * 1024 * 1024 or image[:7] != b"\x7fELF\x02\x01\x01"
            or struct.unpack_from("<HH", image, 16) != (2, 183)
            or b"TinyGPTNativeABI=3\n" not in image):
        raise ValueError("Expected the current native ARM64 TinyGPT ELF system (ABI 3)")
    entry, table = struct.unpack_from("<QQ", image, 24)
    stride, count = struct.unpack_from("<HH", image, 54)
    if stride != 56 or not 1 <= count <= 32 or table + count * stride > len(image):
        raise ValueError("Invalid native program headers")
    ranges = []
    executable = False
    for index in range(count):
        kind, flags, offset, virtual, physical, files, memory, alignment = struct.unpack_from("<IIQQQQQQ", image, table + index * stride)
        if kind in (2, 3):
            raise ValueError("Dynamic native images are unsupported")
        if kind != 1 or not memory:
            continue
        if (physical != virtual or physical < 0x41000000 or physical + memory > 0x44000000
                or files > memory or offset + files > len(image)
                or (alignment > 1 and (alignment & (alignment - 1) or physical % alignment != offset % alignment))
                or any(physical < end and start < physical + memory for start, end in ranges)):
            raise ValueError("Unsafe native load segment")
        ranges.append((physical, physical + memory))
        executable |= bool(flags & 1) and physical <= entry < physical + files
    if not executable or entry % 4:
        raise ValueError("Invalid native entry point")


def install_system(disk: bytes, system: bytes, partition: int = 2) -> bytes:
    validate_system(system)
    if not 2 <= partition <= 16:
        raise ValueError("The OS belongs on a system partition, not recovery")
    updater = runpy.run_path(str(ROOT / "tinygpt"))
    fat = updater["Fat32Image"](disk, partition_index=partition-1)
    matches = [(offset, entry) for offset, entry in fat._directory_entries(fat.root_cluster)
               if entry[:11] == NATIVE_NAME]
    if len(matches) > 1 or (matches and matches[0][1][11] & 0x10):
        raise ValueError("Ambiguous native system entry")
    if not matches:
        if fat.fat_bits == 16:
            regions = [(fat.root_directory_offset, fat.root_directory_sectors * 512)]
        else:
            regions = [(fat._cluster_offset(cluster), fat.cluster_bytes) for cluster in fat._chain(fat.root_cluster)]
        slot = next((base + offset for base, length in regions for offset in range(0, length, 32)
                     if fat.data[base+offset] == 0), None)
        if slot is None:
            if fat.fat_bits != 32:
                raise ValueError("FAT16 root directory is full; firmware repair is required")
            # FAT32 roots can grow. Prepare the new cluster before linking it;
            # all edits are to this private in-memory image, not the live disk.
            free = next((cluster for cluster in range(2, fat.cluster_count + 2)
                         if fat._fat_get(cluster) == 0), None)
            if free is None:
                raise ValueError("No free cluster for the native system directory entry")
            tail = fat._chain(fat.root_cluster)[-1]
            slot = fat._cluster_offset(free)
            fat.data[slot:slot + fat.cluster_bytes] = bytes(fat.cluster_bytes)
            fat._fat_set(free, 0x0FFFFFFF)
            fat._fat_set(tail, free)
        entry = bytearray(32)
        entry[:11] = NATIVE_NAME
        entry[11] = 0x20
        fat.data[slot:slot+32] = entry
    fat.replace_file((NATIVE_NAME,), system)
    if fat.read_file((NATIVE_NAME,)) != system:
        raise ValueError("Native system read-back mismatch")
    start, end = fat.volume_offset, fat.volume_offset + fat.total_sectors * 512
    after = bytes(fat.data)
    if after[:start] != disk[:start] or after[end:] != disk[end:]:
        raise ValueError("Update touched bytes outside the selected system partition")
    return after


def build_image(system: Path, output: Path) -> None:
    payload = system.read_bytes()
    validate_system(payload)
    spec = importlib.util.spec_from_file_location("native_image_builder", ROOT / "tools/make_image.py")
    builder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(builder)
    with tempfile.TemporaryDirectory(prefix="tinygpt-native-image-") as directory:
        temporary = Path(directory) / "source.img"
        builder.build_image(system, temporary, ROOT / "assets/freedoom1.wad")
        updater = runpy.run_path(str(ROOT / "tinygpt"))
        recovery = updater["Fat32Image"](temporary.read_bytes())
        offset, _ = recovery._find_entry(updater["BOOT_PATH"])
        recovery.data[offset:offset+11] = b"RECOVERYELF"
        offset, _ = recovery._find_entry((b"STARTUP NSH",))
        recovery.data[offset:offset+11] = b"NATIVE  TXT"
        recovery.replace_file((b"NATIVE  TXT",), b"Boot with TinyGPT BIOS, not EDK II. The system is TINYGPT.ELF on partition 2.\n")
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(install_system(bytes(recovery.data), payload))
    print(f"Created native disk: {output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("system", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    build_image(args.system, args.output)
