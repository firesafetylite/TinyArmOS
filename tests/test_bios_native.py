from __future__ import annotations
import ctypes as C
import importlib.util
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest
from bios_fixture import ROOT, disk_fixture, native_elf


class Partition(C.Structure):
    _fields_ = [("first", C.c_uint64), ("sectors", C.c_uint64), ("number", C.c_uint32), ("name", C.c_char * 37)]


class Fat(C.Structure):
    _fields_ = [("partition", Partition)] + [(name, C.c_uint32) for name in
        ("reserved", "fat_sectors", "root_sectors", "data_sector", "clusters", "root_cluster")] + [
        ("root_entries", C.c_uint16), ("sectors_per_cluster", C.c_uint8), ("fats", C.c_uint8), ("bits", C.c_uint8)]


class File(C.Structure):
    _fields_ = [("name", C.c_uint8 * 11), ("attributes", C.c_uint8), ("cluster", C.c_uint32), ("size", C.c_uint32)]


class Segment(C.Structure):
    _fields_ = [(name, C.c_uint64) for name in ("destination", "offset", "files", "memory")] + [("flags", C.c_uint32)]


class Executable(C.Structure):
    _fields_ = [("entry", C.c_uint64), ("count", C.c_uint32), ("segments", Segment * 16)]


class NativeBiosTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        compiler = shutil.which("cc")
        if not compiler:
            raise unittest.SkipTest("Native BIOS parser tests require a host C compiler")
        cls.temp = tempfile.TemporaryDirectory(prefix="tinygpt-bios-native-")
        cls.addClassCleanup(cls.temp.cleanup)
        directory = Path(cls.temp.name)
        stub = directory / "stub.c"
        stub.write_text('''
#include "bios.h"
static const uint8_t *disk;
static size_t size;
void fixture(const uint8_t *data, size_t bytes) { disk=data; size=bytes; }
uint64_t disk_sectors(void) { return size/512; }
int disk_read(uint64_t lba, uint8_t out[512]) {
    if (lba>=size/512) return 0;
    memcpy(out, disk+lba*512, 512); return 1;
}
void puts_bios(const char *text) { (void)text; }
void putc_bios(char ch) { (void)ch; }
void number(uint64_t n) { (void)n; }
''')
        library = directory / "bios.so"
        subprocess.run([compiler, "-shared", "-fPIC", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-I", str(ROOT / "firmware/bios"), str(stub),
                        str(ROOT / "firmware/bios/storage.c"), str(ROOT / "firmware/bios/elf.c"),
                        "-o", str(library)], check=True, capture_output=True, text=True)
        cls.lib = C.CDLL(str(library))
        cls.lib.fixture.argtypes = [C.c_void_p, C.c_size_t]
        cls.lib.executable_parse.argtypes = [C.c_void_p, C.c_size_t, C.POINTER(Executable)]
        cls.lib.fat_mount.argtypes = [C.POINTER(Partition), C.POINTER(Fat)]
        cls.lib.fat_find.argtypes = [C.POINTER(Fat), C.c_char_p, C.POINTER(File)]
        cls.lib.fat_read.argtypes = [C.POINTER(Fat), C.POINTER(File), C.c_void_p, C.c_uint32]
        cls.disk = bytearray(disk_fixture(directory).read_bytes())
        cls.buffer = (C.c_uint8 * len(cls.disk)).from_buffer(cls.disk)
        cls.lib.fixture(cls.buffer, len(cls.disk))

    def parse(self, image: bytes) -> tuple[int, Executable]:
        executable = Executable()
        return self.lib.executable_parse(image, len(image), C.byref(executable)), executable

    def test_native_elf_and_invalid_images(self) -> None:
        valid, executable = self.parse(native_elf())
        self.assertEqual(valid, 1)
        self.assertEqual(executable.entry, 0x41000000)
        self.assertEqual(executable.count, 1)
        for invalid in (b"", b"MZ" + bytes(512), native_elf()[:63], native_elf()[:120]):
            self.assertEqual(self.parse(invalid)[0], 0)

    def test_elf_rejects_unsafe_segments_and_entrypoints(self) -> None:
        for offset, value in ((24, 0x40100000), (24, 0x41000001), (32, 2**64-1),
                              (64+8, 2**64-1), (64+24, 0x40100000),
                              (64+32, 2**64-1), (64+40, 0x4000000), (64+48, 3)):
            with self.subTest(offset=offset, value=value):
                image = bytearray(native_elf())
                struct.pack_into("<Q", image, offset, value)
                self.assertEqual(self.parse(bytes(image))[0], 0)
        image = bytearray(native_elf())
        struct.pack_into("<I", image, 64+4, 4)  # Entry segment is not executable.
        self.assertEqual(self.parse(bytes(image))[0], 0)
        image = bytearray(native_elf())
        struct.pack_into("<H", image, 56, 2)
        image[120:176] = image[64:120]  # Overlapping LOAD segments.
        self.assertEqual(self.parse(bytes(image))[0], 0)

    def test_gpt_crc_failure_and_fat_bounded_reads(self) -> None:
        self.assertEqual(self.lib.partitions_scan(), 1)
        partitions = (Partition * 128).in_dll(self.lib, "partitions")
        self.assertEqual(C.c_uint32.in_dll(self.lib, "partition_count").value, 2)
        for index, expected_bits, path in ((0, 16, b"/EFI/BOOT/NATIVE.ELF"), (1, 32, b"/TINYGPT.NEW")):
            fat, file = Fat(), File()
            self.assertEqual(self.lib.fat_mount(C.byref(partitions[index]), C.byref(fat)), 1)
            self.assertEqual(fat.bits, expected_bits)
            self.assertEqual(self.lib.fat_find(C.byref(fat), path, C.byref(file)), 1)
            output = C.create_string_buffer(file.size)
            self.assertEqual(self.lib.fat_read(C.byref(fat), C.byref(file), output, file.size-1), 0)
            self.assertEqual(self.lib.fat_read(C.byref(fat), C.byref(file), output, file.size), 1)
            self.assertTrue(output.raw.startswith(b"\x7fELF" if index == 0 else b"Initialize TinyGPT"))
            self.assertEqual(self.lib.fat_find(C.byref(fat), b"/../SECRET", C.byref(file)), 0)
        self.disk[528] ^= 1
        try:
            self.assertEqual(self.lib.partitions_scan(), 0)
            self.assertEqual(C.c_uint32.in_dll(self.lib, "partition_count").value, 0)
        finally:
            self.disk[528] ^= 1

    def test_malformed_fat_geometry_is_rejected(self) -> None:
        self.assertEqual(self.lib.partitions_scan(), 1)
        partition = (Partition * 128).in_dll(self.lib, "partitions")[0]
        offset = partition.first * 512
        for field, value in ((13, 3), (16, 0), (16, 3)):
            old = self.disk[offset+field]
            self.disk[offset+field] = value
            try:
                self.assertEqual(self.lib.fat_mount(C.byref(partition), C.byref(Fat())), 0)
            finally:
                self.disk[offset+field] = old


class BiosRomTests(unittest.TestCase):
    def test_rom_packer_requires_reset_vector_and_bounded_segments(self) -> None:
        spec = importlib.util.spec_from_file_location("bios_rom", ROOT / "tools/make_bios_rom.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        image = bytearray(native_elf())
        with self.assertRaises(ValueError):
            module.pack_elf(bytes(image))  # OS payload, not a reset-vector image.
        struct.pack_into("<Q", image, 24, 0)
        struct.pack_into("<Q", image, 64+16, 0)
        struct.pack_into("<Q", image, 64+24, 0)
        packed = module.pack_elf(bytes(image))
        self.assertEqual(packed, image[256:])
        struct.pack_into("<Q", image, 64+32, 2**64-1)
        with self.assertRaises(ValueError):
            module.pack_elf(bytes(image))


if __name__ == "__main__":
    unittest.main()
