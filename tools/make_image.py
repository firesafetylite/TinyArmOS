#!/usr/bin/env python3
"""Create a bootable GPT/FAT32 ARM64 UEFI disk using only Python's stdlib."""

from __future__ import annotations

import math
import struct
import sys
import uuid
import zlib
from pathlib import Path

SECTOR = 512
IMAGE_BYTES = 64 * 1024 * 1024
PARTITION_START = 2048
GPT_ENTRIES = 128
GPT_ENTRY_BYTES = 128
GPT_ENTRY_SECTORS = (GPT_ENTRIES * GPT_ENTRY_BYTES) // SECTOR
ESP_TYPE = uuid.UUID("c12a7328-f81f-11d2-ba4b-00a0c93ec93b")
DISK_GUID = uuid.UUID("117a71a5-e844-4e20-a475-74696e796f73")
PART_GUID = uuid.UUID("e056959d-53d4-4c21-9ff4-61726d363475")


def gpt_header(
    current_lba: int,
    backup_lba: int,
    first_usable: int,
    last_usable: int,
    entries_lba: int,
    entries_crc: int,
) -> bytes:
    header = bytearray(SECTOR)
    struct.pack_into(
        "<8sIIIIQQQQ16sQIII",
        header,
        0,
        b"EFI PART",
        0x00010000,
        92,
        0,
        0,
        current_lba,
        backup_lba,
        first_usable,
        last_usable,
        DISK_GUID.bytes_le,
        entries_lba,
        GPT_ENTRIES,
        GPT_ENTRY_BYTES,
        entries_crc,
    )
    crc = zlib.crc32(header[:92]) & 0xFFFFFFFF
    struct.pack_into("<I", header, 16, crc)
    return bytes(header)


def short_entry(name: bytes, attributes: int, cluster: int, size: int = 0) -> bytes:
    if len(name) != 11:
        raise ValueError(f"8.3 name must be exactly 11 bytes: {name!r}")
    entry = bytearray(32)
    entry[:11] = name
    entry[11] = attributes
    # Fixed 2026-01-01 00:00 timestamp.
    dos_date = ((2026 - 1980) << 9) | (1 << 5) | 1
    struct.pack_into("<H", entry, 16, dos_date)
    struct.pack_into("<H", entry, 18, dos_date)
    struct.pack_into("<H", entry, 20, (cluster >> 16) & 0xFFFF)
    struct.pack_into("<H", entry, 24, dos_date)
    struct.pack_into("<H", entry, 26, cluster & 0xFFFF)
    struct.pack_into("<I", entry, 28, size)
    return bytes(entry)


def directory(entries: list[bytes]) -> bytes:
    payload = b"".join(entries)
    if len(payload) > SECTOR:
        raise ValueError("directory does not fit in one cluster")
    return payload + bytes(SECTOR - len(payload))


def build_image(efi_path: Path, output_path: Path, freedoom_path: Path) -> None:
    efi = efi_path.read_bytes()
    freedoom = freedoom_path.read_bytes()
    startup = b"fs0:\\EFI\\BOOT\\BOOTAA64.EFI\r\n"
    total_sectors = IMAGE_BYTES // SECTOR
    backup_header_lba = total_sectors - 1
    backup_entries_lba = backup_header_lba - GPT_ENTRY_SECTORS
    first_usable = 34
    last_usable = backup_entries_lba - 1
    partition_last = last_usable
    partition_sectors = partition_last - PARTITION_START + 1

    reserved = 32
    fat_count = 2
    sectors_per_cluster = 1
    fat_sectors = math.ceil(
        (partition_sectors - reserved) * 4
        / (SECTOR * sectors_per_cluster + fat_count * 4)
    )
    while True:
        clusters = (partition_sectors - reserved - fat_count * fat_sectors) // sectors_per_cluster
        if fat_sectors * (SECTOR // 4) >= clusters + 2:
            break
        fat_sectors += 1
    cluster_count = (partition_sectors - reserved - fat_count * fat_sectors) // sectors_per_cluster
    if cluster_count < 65525:
        raise ValueError("image is too small to be a valid FAT32 volume")

    image = bytearray(IMAGE_BYTES)

    # Protective MBR.
    mbr = memoryview(image)[:SECTOR]
    mbr[446] = 0
    mbr[447:450] = b"\x00\x02\x00"
    mbr[450] = 0xEE
    mbr[451:454] = b"\xff\xff\xff"
    struct.pack_into("<I", mbr, 454, 1)
    struct.pack_into("<I", mbr, 458, min(total_sectors - 1, 0xFFFFFFFF))
    mbr[510:512] = b"\x55\xaa"

    # GPT partition array and primary/backup headers.
    entries = bytearray(GPT_ENTRIES * GPT_ENTRY_BYTES)
    partition_name = "TinyArmOS ESP".encode("utf-16-le")
    entries[0:16] = ESP_TYPE.bytes_le
    entries[16:32] = PART_GUID.bytes_le
    struct.pack_into("<QQQ", entries, 32, PARTITION_START, partition_last, 0)
    entries[56 : 56 + len(partition_name)] = partition_name
    entries_crc = zlib.crc32(entries) & 0xFFFFFFFF
    image[2 * SECTOR : (2 + GPT_ENTRY_SECTORS) * SECTOR] = entries
    image[backup_entries_lba * SECTOR : (backup_entries_lba + GPT_ENTRY_SECTORS) * SECTOR] = entries
    image[SECTOR : 2 * SECTOR] = gpt_header(
        1, backup_header_lba, first_usable, last_usable, 2, entries_crc
    )
    image[backup_header_lba * SECTOR : total_sectors * SECTOR] = gpt_header(
        backup_header_lba, 1, first_usable, last_usable, backup_entries_lba, entries_crc
    )

    # FAT32 boot sector.
    volume_base = PARTITION_START * SECTOR
    boot = bytearray(SECTOR)
    boot[0:3] = b"\xeb\x58\x90"
    boot[3:11] = b"TINYARMS"
    struct.pack_into("<H", boot, 11, SECTOR)
    boot[13] = sectors_per_cluster
    struct.pack_into("<H", boot, 14, reserved)
    boot[16] = fat_count
    struct.pack_into("<H", boot, 17, 0)
    struct.pack_into("<H", boot, 19, 0)
    boot[21] = 0xF8
    struct.pack_into("<H", boot, 22, 0)
    struct.pack_into("<H", boot, 24, 63)
    struct.pack_into("<H", boot, 26, 255)
    struct.pack_into("<I", boot, 28, PARTITION_START)
    struct.pack_into("<I", boot, 32, partition_sectors)
    struct.pack_into("<I", boot, 36, fat_sectors)
    struct.pack_into("<H", boot, 40, 0)
    struct.pack_into("<H", boot, 42, 0)
    struct.pack_into("<I", boot, 44, 2)
    struct.pack_into("<H", boot, 48, 1)
    struct.pack_into("<H", boot, 50, 6)
    boot[64] = 0x80
    boot[66] = 0x29
    struct.pack_into("<I", boot, 67, 0x5441524D)
    boot[71:82] = b"TINYARMOS  "
    boot[82:90] = b"FAT32   "
    boot[510:512] = b"\x55\xaa"
    image[volume_base : volume_base + SECTOR] = boot
    image[volume_base + 6 * SECTOR : volume_base + 7 * SECTOR] = boot

    # FAT32 FSInfo (and backup).
    fsinfo = bytearray(SECTOR)
    struct.pack_into("<I", fsinfo, 0, 0x41615252)
    struct.pack_into("<I", fsinfo, 484, 0x61417272)
    struct.pack_into("<I", fsinfo, 488, 0xFFFFFFFF)
    struct.pack_into("<I", fsinfo, 492, 5)
    struct.pack_into("<I", fsinfo, 508, 0xAA550000)
    image[volume_base + SECTOR : volume_base + 2 * SECTOR] = fsinfo
    image[volume_base + 7 * SECTOR : volume_base + 8 * SECTOR] = fsinfo

    # Allocate clusters: 2=root, 3=EFI, 4=BOOT, then EFI, startup.nsh, and Freedoom.
    next_cluster = 5
    efi_clusters = max(1, math.ceil(len(efi) / SECTOR))
    efi_first = next_cluster
    next_cluster += efi_clusters
    startup_first = next_cluster
    next_cluster += 1
    freedoom_clusters = max(1, math.ceil(len(freedoom) / SECTOR))
    freedoom_first = next_cluster
    next_cluster += freedoom_clusters
    if next_cluster > cluster_count + 2:
        raise ValueError("EFI application and Freedoom WAD do not fit in the image")

    fat_entries = [0] * (cluster_count + 2)
    fat_entries[0] = 0x0FFFFFF8
    fat_entries[1] = 0xFFFFFFFF
    fat_entries[2] = fat_entries[3] = fat_entries[4] = 0x0FFFFFFF
    for cluster in range(efi_first, efi_first + efi_clusters):
        fat_entries[cluster] = cluster + 1 if cluster + 1 < efi_first + efi_clusters else 0x0FFFFFFF
    fat_entries[startup_first] = 0x0FFFFFFF
    for cluster in range(freedoom_first, freedoom_first + freedoom_clusters):
        fat_entries[cluster] = cluster + 1 if cluster + 1 < freedoom_first + freedoom_clusters else 0x0FFFFFFF
    fat = bytearray(fat_sectors * SECTOR)
    for index, value in enumerate(fat_entries):
        if index * 4 + 4 > len(fat):
            break
        struct.pack_into("<I", fat, index * 4, value)
    fat1 = volume_base + reserved * SECTOR
    fat2 = fat1 + fat_sectors * SECTOR
    image[fat1 : fat1 + len(fat)] = fat
    image[fat2 : fat2 + len(fat)] = fat

    data_lba = PARTITION_START + reserved + fat_count * fat_sectors

    def put_cluster(cluster: int, payload: bytes) -> None:
        offset = (data_lba + (cluster - 2) * sectors_per_cluster) * SECTOR
        chunk = payload[:SECTOR]
        image[offset : offset + len(chunk)] = chunk

    root = directory([
        short_entry(b"EFI        ", 0x10, 3),
        short_entry(b"STARTUP NSH", 0x20, startup_first, len(startup)),
        short_entry(b"DOOMU   WAD", 0x20, freedoom_first, len(freedoom)),
    ])
    efi_dir = directory([
        short_entry(b".          ", 0x10, 3),
        short_entry(b"..         ", 0x10, 2),
        short_entry(b"BOOT       ", 0x10, 4),
    ])
    boot_dir = directory([
        short_entry(b".          ", 0x10, 4),
        short_entry(b"..         ", 0x10, 3),
        short_entry(b"BOOTAA64EFI", 0x20, efi_first, len(efi)),
    ])
    put_cluster(2, root)
    put_cluster(3, efi_dir)
    put_cluster(4, boot_dir)
    for index in range(efi_clusters):
        put_cluster(efi_first + index, efi[index * SECTOR : (index + 1) * SECTOR])
    put_cluster(startup_first, startup)
    for index in range(freedoom_clusters):
        put_cluster(freedoom_first + index, freedoom[index * SECTOR : (index + 1) * SECTOR])

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(image)
    print(f"Created {output_path} ({IMAGE_BYTES // (1024 * 1024)} MiB)")
    print(f"Embedded {efi_path} ({len(efi)} bytes) as EFI/BOOT/BOOTAA64.EFI")
    print(f"Embedded {freedoom_path} ({len(freedoom)} bytes) as DOOMU.WAD (Freedoom Phase 1)")


def main() -> None:
    if len(sys.argv) != 4:
        print(f"usage: {sys.argv[0]} BOOTAA64.EFI OUTPUT.img FREEDOOM.WAD", file=sys.stderr)
        raise SystemExit(2)
    build_image(Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]))


if __name__ == "__main__":
    main()
