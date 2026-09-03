#!/usr/bin/env python3
"""Create the two-partition TinyArmOS ARM64 UEFI disk image."""

from __future__ import annotations

import math
import struct
import sys
import uuid
import zlib
from pathlib import Path

SECTOR = 512
IMAGE_BYTES = 64 * 1024 * 1024
GPT_ENTRIES = 128
GPT_ENTRY_BYTES = 128
GPT_ENTRY_SECTORS = (GPT_ENTRIES * GPT_ENTRY_BYTES) // SECTOR
RECOVERY_START = 2048
RECOVERY_SECTORS = 32 * 1024  # 16 MiB FAT16 pre-OS ESP.
SYSTEM_START = RECOVERY_START + RECOVERY_SECTORS
ESP_TYPE = uuid.UUID("c12a7328-f81f-11d2-ba4b-00a0c93ec93b")
DISK_GUID = uuid.UUID("117a71a5-e844-4e20-a475-74696e796f73")
RECOVERY_GUID = uuid.UUID("e056959d-53d4-4c21-9ff4-7265636f7631")
SYSTEM_GUID = uuid.UUID("e056959d-53d4-4c21-9ff4-61726d363475")


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
    struct.pack_into("<I", header, 16, zlib.crc32(header[:92]) & 0xFFFFFFFF)
    return bytes(header)


def gpt_entry(
    type_guid: uuid.UUID,
    unique_guid: uuid.UUID,
    first_lba: int,
    last_lba: int,
    name: str,
) -> bytes:
    entry = bytearray(GPT_ENTRY_BYTES)
    entry[:16] = type_guid.bytes_le
    entry[16:32] = unique_guid.bytes_le
    struct.pack_into("<QQQ", entry, 32, first_lba, last_lba, 0)
    encoded_name = name.encode("utf-16-le")
    entry[56 : 56 + len(encoded_name)] = encoded_name
    return bytes(entry)


def short_entry(name: bytes, attributes: int, cluster: int, size: int = 0) -> bytes:
    if len(name) != 11:
        raise ValueError(f"8.3 name must be exactly 11 bytes: {name!r}")
    entry = bytearray(32)
    entry[:11] = name
    entry[11] = attributes
    dos_date = ((2026 - 1980) << 9) | (1 << 5) | 1
    struct.pack_into("<H", entry, 16, dos_date)
    struct.pack_into("<H", entry, 18, dos_date)
    struct.pack_into("<H", entry, 20, (cluster >> 16) & 0xFFFF)
    struct.pack_into("<H", entry, 24, dos_date)
    struct.pack_into("<H", entry, 26, cluster & 0xFFFF)
    struct.pack_into("<I", entry, 28, size)
    return bytes(entry)


def directory(entries: list[bytes], size: int = SECTOR) -> bytes:
    payload = b"".join(entries)
    if len(payload) > size:
        raise ValueError("directory does not fit in its allocation")
    return payload + bytes(size - len(payload))


def put_cluster(
    image: bytearray,
    data_lba: int,
    cluster: int,
    payload: bytes,
    sectors_per_cluster: int = 1,
) -> None:
    cluster_bytes = sectors_per_cluster * SECTOR
    offset = (data_lba + (cluster - 2) * sectors_per_cluster) * SECTOR
    image[offset : offset + cluster_bytes] = payload[:cluster_bytes].ljust(
        cluster_bytes, b"\0"
    )


def format_recovery_fat16(
    image: bytearray, efi: bytes, startup: bytes
) -> None:
    start = RECOVERY_START
    sectors = RECOVERY_SECTORS
    reserved = 1
    fat_count = 2
    root_entries = 512
    root_sectors = root_entries * 32 // SECTOR
    sectors_per_cluster = 1
    fat_sectors = 1
    while True:
        data_sectors = sectors - reserved - fat_count * fat_sectors - root_sectors
        clusters = data_sectors // sectors_per_cluster
        required = math.ceil((clusters + 2) * 2 / SECTOR)
        if required <= fat_sectors:
            break
        fat_sectors = required
    if not 4085 <= clusters < 65525:
        raise ValueError("recovery partition is not a valid FAT16 volume")

    volume_base = start * SECTOR
    boot = bytearray(SECTOR)
    boot[0:3] = b"\xeb\x3c\x90"
    boot[3:11] = b"TINYARMS"
    struct.pack_into("<H", boot, 11, SECTOR)
    boot[13] = sectors_per_cluster
    struct.pack_into("<H", boot, 14, reserved)
    boot[16] = fat_count
    struct.pack_into("<H", boot, 17, root_entries)
    struct.pack_into("<H", boot, 19, sectors)
    boot[21] = 0xF8
    struct.pack_into("<H", boot, 22, fat_sectors)
    struct.pack_into("<H", boot, 24, 63)
    struct.pack_into("<H", boot, 26, 255)
    struct.pack_into("<I", boot, 28, start)
    boot[36] = 0x80
    boot[38] = 0x29
    struct.pack_into("<I", boot, 39, 0x54415252)
    boot[43:54] = b"TINYRECOV  "
    boot[54:62] = b"FAT16   "
    boot[510:512] = b"\x55\xaa"
    image[volume_base : volume_base + SECTOR] = boot

    next_cluster = 5
    efi_clusters = max(1, math.ceil(len(efi) / SECTOR))
    efi_first = next_cluster
    next_cluster += efi_clusters
    startup_first = next_cluster
    next_cluster += 1
    if next_cluster > clusters + 2:
        raise ValueError("pre-OS EFI application does not fit in recovery partition")

    fat_entries = [0] * (clusters + 2)
    fat_entries[0] = 0xFFF8
    fat_entries[1] = 0xFFFF
    fat_entries[2] = fat_entries[3] = fat_entries[4] = 0xFFFF
    for cluster in range(efi_first, efi_first + efi_clusters):
        fat_entries[cluster] = (
            cluster + 1 if cluster + 1 < efi_first + efi_clusters else 0xFFFF
        )
    fat_entries[startup_first] = 0xFFFF
    fat = bytearray(fat_sectors * SECTOR)
    for index, value in enumerate(fat_entries):
        if index * 2 + 2 > len(fat):
            break
        struct.pack_into("<H", fat, index * 2, value)
    fat1 = volume_base + reserved * SECTOR
    fat2 = fat1 + fat_sectors * SECTOR
    image[fat1 : fat1 + len(fat)] = fat
    image[fat2 : fat2 + len(fat)] = fat

    root_lba = start + reserved + fat_count * fat_sectors
    data_lba = root_lba + root_sectors
    root = directory(
        [
            short_entry(b"TINYRECOV  ", 0x08, 0),
            short_entry(b"EFI        ", 0x10, 2),
            short_entry(b"STARTUP NSH", 0x20, startup_first, len(startup)),
        ],
        root_sectors * SECTOR,
    )
    image[root_lba * SECTOR : (root_lba + root_sectors) * SECTOR] = root
    put_cluster(
        image,
        data_lba,
        2,
        directory(
            [
                short_entry(b".          ", 0x10, 2),
                short_entry(b"..         ", 0x10, 0),
                short_entry(b"BOOT       ", 0x10, 3),
            ]
        ),
    )
    put_cluster(
        image,
        data_lba,
        3,
        directory(
            [
                short_entry(b".          ", 0x10, 3),
                short_entry(b"..         ", 0x10, 2),
                short_entry(b"BOOTAA64EFI", 0x20, efi_first, len(efi)),
            ]
        ),
    )
    # Cluster 4 is reserved for future pre-OS metadata.
    put_cluster(image, data_lba, 4, directory([]))
    for index in range(efi_clusters):
        put_cluster(image, data_lba, efi_first + index, efi[index * SECTOR : (index + 1) * SECTOR])
    put_cluster(image, data_lba, startup_first, startup)


def format_system_fat32(
    image: bytearray, system_last: int, freedoom: bytes, factory_install: bytes
) -> None:
    start = SYSTEM_START
    sectors = system_last - start + 1
    reserved = 32
    fat_count = 2
    sectors_per_cluster = 1
    fat_sectors = 1
    while True:
        clusters = (sectors - reserved - fat_count * fat_sectors) // sectors_per_cluster
        required = math.ceil((clusters + 2) * 4 / SECTOR)
        if required <= fat_sectors:
            break
        fat_sectors = required
    if clusters < 65525:
        raise ValueError("system partition is too small for FAT32")

    volume_base = start * SECTOR
    boot = bytearray(SECTOR)
    boot[0:3] = b"\xeb\x58\x90"
    boot[3:11] = b"TINYARMS"
    struct.pack_into("<H", boot, 11, SECTOR)
    boot[13] = sectors_per_cluster
    struct.pack_into("<H", boot, 14, reserved)
    boot[16] = fat_count
    boot[21] = 0xF8
    struct.pack_into("<H", boot, 24, 63)
    struct.pack_into("<H", boot, 26, 255)
    struct.pack_into("<I", boot, 28, start)
    struct.pack_into("<I", boot, 32, sectors)
    struct.pack_into("<I", boot, 36, fat_sectors)
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

    factory_first = 3
    freedoom_clusters = max(1, math.ceil(len(freedoom) / SECTOR))
    freedoom_first = 4
    next_cluster = freedoom_first + freedoom_clusters
    if next_cluster > clusters + 2:
        raise ValueError("Freedoom WAD does not fit in system partition")

    fsinfo = bytearray(SECTOR)
    struct.pack_into("<I", fsinfo, 0, 0x41615252)
    struct.pack_into("<I", fsinfo, 484, 0x61417272)
    struct.pack_into("<I", fsinfo, 488, clusters - (next_cluster - 2))
    struct.pack_into("<I", fsinfo, 492, next_cluster)
    struct.pack_into("<I", fsinfo, 508, 0xAA550000)
    image[volume_base + SECTOR : volume_base + 2 * SECTOR] = fsinfo
    image[volume_base + 7 * SECTOR : volume_base + 8 * SECTOR] = fsinfo

    fat_entries = [0] * (clusters + 2)
    fat_entries[0] = 0x0FFFFFF8
    fat_entries[1] = 0xFFFFFFFF
    fat_entries[2] = fat_entries[factory_first] = 0x0FFFFFFF
    for cluster in range(freedoom_first, freedoom_first + freedoom_clusters):
        fat_entries[cluster] = (
            cluster + 1
            if cluster + 1 < freedoom_first + freedoom_clusters
            else 0x0FFFFFFF
        )
    fat = bytearray(fat_sectors * SECTOR)
    for index, value in enumerate(fat_entries):
        if index * 4 + 4 > len(fat):
            break
        struct.pack_into("<I", fat, index * 4, value)
    fat1 = volume_base + reserved * SECTOR
    fat2 = fat1 + fat_sectors * SECTOR
    image[fat1 : fat1 + len(fat)] = fat
    image[fat2 : fat2 + len(fat)] = fat
    data_lba = start + reserved + fat_count * fat_sectors

    put_cluster(
        image,
        data_lba,
        2,
        directory(
            [
                short_entry(b"TINYARMOS  ", 0x08, 0),
                short_entry(b"TINYOS  NEW", 0x20, factory_first, len(factory_install)),
                short_entry(b"DOOMU   WAD", 0x20, freedoom_first, len(freedoom)),
            ]
        ),
    )
    put_cluster(image, data_lba, factory_first, factory_install)
    for index in range(freedoom_clusters):
        put_cluster(
            image,
            data_lba,
            freedoom_first + index,
            freedoom[index * SECTOR : (index + 1) * SECTOR],
        )


def build_image(efi_path: Path, output_path: Path, freedoom_path: Path) -> None:
    efi = efi_path.read_bytes()
    freedoom = freedoom_path.read_bytes()
    startup = b"fs0:\\EFI\\BOOT\\BOOTAA64.EFI\r\n"
    factory_install = b"Initialize TinyArmOS on first boot\n"
    total_sectors = IMAGE_BYTES // SECTOR
    backup_header_lba = total_sectors - 1
    backup_entries_lba = backup_header_lba - GPT_ENTRY_SECTORS
    first_usable = 34
    last_usable = backup_entries_lba - 1
    recovery_last = RECOVERY_START + RECOVERY_SECTORS - 1
    if SYSTEM_START > last_usable:
        raise ValueError("disk is too small for the partition layout")

    image = bytearray(IMAGE_BYTES)
    mbr = memoryview(image)[:SECTOR]
    mbr[446] = 0
    mbr[447:450] = b"\x00\x02\x00"
    mbr[450] = 0xEE
    mbr[451:454] = b"\xff\xff\xff"
    struct.pack_into("<I", mbr, 454, 1)
    struct.pack_into("<I", mbr, 458, min(total_sectors - 1, 0xFFFFFFFF))
    mbr[510:512] = b"\x55\xaa"

    entries = bytearray(GPT_ENTRIES * GPT_ENTRY_BYTES)
    entries[:GPT_ENTRY_BYTES] = gpt_entry(
        ESP_TYPE,
        RECOVERY_GUID,
        RECOVERY_START,
        recovery_last,
        "TinyArmOS Recovery",
    )
    entries[GPT_ENTRY_BYTES : 2 * GPT_ENTRY_BYTES] = gpt_entry(
        ESP_TYPE,
        SYSTEM_GUID,
        SYSTEM_START,
        last_usable,
        "TinyArmOS System",
    )
    entries_crc = zlib.crc32(entries) & 0xFFFFFFFF
    image[2 * SECTOR : (2 + GPT_ENTRY_SECTORS) * SECTOR] = entries
    image[backup_entries_lba * SECTOR : (backup_entries_lba + GPT_ENTRY_SECTORS) * SECTOR] = entries
    image[SECTOR : 2 * SECTOR] = gpt_header(
        1, backup_header_lba, first_usable, last_usable, 2, entries_crc
    )
    image[backup_header_lba * SECTOR : total_sectors * SECTOR] = gpt_header(
        backup_header_lba, 1, first_usable, last_usable, backup_entries_lba, entries_crc
    )

    format_recovery_fat16(image, efi, startup)
    format_system_fat32(image, last_usable, freedoom, factory_install)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(image)
    print(f"Created {output_path} ({IMAGE_BYTES // (1024 * 1024)} MiB, 2 partitions)")
    print(f"Embedded {efi_path} ({len(efi)} bytes) in the recovery ESP")
    print(f"Embedded {freedoom_path} ({len(freedoom)} bytes) in the TinyArmOS system partition")


def main() -> None:
    if len(sys.argv) != 4:
        print(f"usage: {sys.argv[0]} BOOTAA64.EFI OUTPUT.img FREEDOOM.WAD", file=sys.stderr)
        raise SystemExit(2)
    build_image(Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]))


if __name__ == "__main__":
    main()
