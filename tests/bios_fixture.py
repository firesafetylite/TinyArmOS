"""Synthetic, disposable inputs for native BIOS tests; never use a real VM disk."""
from pathlib import Path
import importlib.util
import struct

ROOT = Path(__file__).resolve().parents[1]


def native_elf() -> bytes:
    # Position-independent UART hello + RET. x0 is the firmware boot-info pointer.
    instructions = (0x58000101, 0x10000122, 0x38401443, 0x34000063,
                    0xB9000023, 0x17FFFFFD, 0xD65F03C0, 0xD503201F)
    code = struct.pack("<8IQ", *instructions, 0x09000000) + b"NATIVE_PAYLOAD_OK\n\0"
    image = bytearray(256 + len(code))
    image[:7] = b"\x7fELF\x02\x01\x01"
    struct.pack_into("<HHIQQQIHHHHHH", image, 16,
                     2, 183, 1, 0x41000000, 64, 0, 0, 64, 56, 1, 0, 0, 0)
    struct.pack_into("<IIQQQQQQ", image, 64,
                     1, 5, 256, 0x41000000, 0x41000000, len(code), len(code) + 16, 1)
    image[256:] = code
    return bytes(image)


def disk_fixture(directory: Path) -> Path:
    spec = importlib.util.spec_from_file_location("bios_image_builder", ROOT / "tools/make_image.py")
    builder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(builder)
    payload, wad, disk = directory / "native.elf", directory / "fixture.wad", directory / "fixture.img"
    payload.write_bytes(native_elf())
    wad.write_bytes(b"fixture data\n" * 1024)
    builder.build_image(payload, disk, wad)
    # Give the native payload an honest .ELF name, not an EFI extension.
    with disk.open("r+b") as image:
        image.seek(builder.RECOVERY_START * 512)
        boot = image.read(512)
        reserved, fats, roots, fat_sectors = (struct.unpack_from("<H", boot, 14)[0], boot[16],
                                           struct.unpack_from("<H", boot, 17)[0], struct.unpack_from("<H", boot, 22)[0])
        data_lba = builder.RECOVERY_START + reserved + fats * fat_sectors + roots * 32 // 512
        image.seek((data_lba + 1) * 512 + 64)  # BOOT directory, third entry
        assert image.read(11) == b"BOOTAA64EFI"
        image.seek(-11, 1)
        image.write(b"NATIVE  ELF")
    return disk
