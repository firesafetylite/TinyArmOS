from __future__ import annotations

import runpy
import struct
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UPDATER = runpy.run_path(str(ROOT / "tinyarmos"), run_name="tinyarmos_module")
IMAGE_BUILDER = runpy.run_path(
    str(ROOT / "tools" / "make_utm_image.py"), run_name="image_builder_module"
)


def fake_efi(version: str, size: int = 4096) -> bytes:
    marker = (
        f"TinyArmOS {version}\n"
        "architecture=ARM64\n"
        "firmware=UEFI\n"
    ).encode("ascii")
    payload = bytearray(max(size, 1024))
    payload[:2] = b"MZ"
    pe_offset = 0x80
    struct.pack_into("<I", payload, 0x3C, pe_offset)
    payload[pe_offset : pe_offset + 4] = b"PE\0\0"
    struct.pack_into(
        "<HHIIIHH", payload, pe_offset + 4, 0xAA64, 1, 0, 0, 0, 0xF0, 0
    )
    optional_offset = pe_offset + 24
    struct.pack_into("<H", payload, optional_offset, 0x20B)
    struct.pack_into("<H", payload, optional_offset + 68, 10)
    payload[512 : 512 + len(marker)] = marker
    return bytes(payload)


class UpdaterTests(unittest.TestCase):
    def test_version_and_efi_validation(self) -> None:
        image = fake_efi("0.1")
        self.assertEqual(UPDATER["version_tuple"]("0.1"), (0, 1, 0))
        self.assertEqual(UPDATER["version_tuple"]("0.1.1"), (0, 1, 1))
        self.assertEqual(UPDATER["version_tuple"]("0.1.12"), (0, 1, 12))
        self.assertEqual(UPDATER["validate_efi"](image, "0.1"), "0.1")
        with self.assertRaises(UPDATER["UpdateError"]):
            UPDATER["version_tuple"]("00.1")
        with self.assertRaises(UPDATER["UpdateError"]):
            UPDATER["validate_efi"](image, "0.1.1")

    def test_github_host_allowlist(self) -> None:
        self.assertTrue(UPDATER["github_host"]("github.com"))
        self.assertTrue(UPDATER["github_host"]("api.github.com"))
        self.assertTrue(
            UPDATER["github_host"]("release-assets.githubusercontent.com")
        )
        self.assertFalse(UPDATER["github_host"]("github.com.example.org"))
        self.assertFalse(UPDATER["github_host"]("example.org"))

    def test_checksum_parser_requires_exact_asset(self) -> None:
        digest = "a" * 64
        text = f"{digest}  TinyArmOS-v1.2.3-BOOTAA64.EFI\n".encode("ascii")
        self.assertEqual(
            UPDATER["checksum_for"](text, "TinyArmOS-v1.2.3-BOOTAA64.EFI"),
            digest,
        )
        with self.assertRaises(UPDATER["UpdateError"]):
            UPDATER["checksum_for"](text, "other.EFI")

    def test_disk_update_preserves_other_files_and_creates_backup(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            old_efi = fake_efi("1.0.0")
            new_efi = fake_efi("1.1.0", 700_000)
            wad = b"FREEDOOM-TEST" * 1000
            old_path = root / "old.EFI"
            wad_path = root / "doom.wad"
            disk_path = root / "TinyArmOS.img"
            old_path.write_bytes(old_efi)
            wad_path.write_bytes(wad)
            IMAGE_BUILDER["build_image"](old_path, disk_path, wad_path)

            before = UPDATER["Fat32Image"](disk_path.read_bytes())
            self.assertEqual(before.read_file(UPDATER["BOOT_PATH"]), old_efi)
            self.assertEqual(before.read_file((b"DOOMU   WAD",)), wad)
            startup = before.read_file((b"STARTUP NSH",))

            backup = UPDATER["install_image"]("disk", disk_path, new_efi)

            after = UPDATER["Fat32Image"](disk_path.read_bytes())
            self.assertEqual(after.read_file(UPDATER["BOOT_PATH"]), new_efi)
            self.assertEqual(after.read_file((b"DOOMU   WAD",)), wad)
            self.assertEqual(after.read_file((b"STARTUP NSH",)), startup)
            backed_up = UPDATER["Fat32Image"](backup.read_bytes())
            self.assertEqual(backed_up.read_file(UPDATER["BOOT_PATH"]), old_efi)

    def test_utm_target_resolves_to_disk(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            bundle = Path(directory) / "TinyArmOS.utm"
            images = bundle / "Images"
            images.mkdir(parents=True)
            disk = images / "disk-0.img"
            disk.write_bytes(b"disk")
            kind, resolved = UPDATER["target_disk"](bundle)
            self.assertEqual(kind, "disk")
            self.assertEqual(resolved, disk.resolve())


if __name__ == "__main__":
    unittest.main()
