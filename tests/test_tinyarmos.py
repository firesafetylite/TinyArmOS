from __future__ import annotations

import hashlib
import json
import runpy
import struct
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UPDATER = runpy.run_path(str(ROOT / "tinyarmos"), run_name="tinyarmos_module")
IMAGE_BUILDER = runpy.run_path(
    str(ROOT / "tools" / "make_image.py"), run_name="image_builder_module"
)
SITE_BUILDER = runpy.run_path(
    str(ROOT / "tools" / "make_update_site.py"), run_name="site_builder_module"
)


def fake_efi(version: str, size: int = 4096, channel: str = "main") -> bytes:
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
    metadata = (
        f"TinyArmOSBuildVersion={version}\n"
        f"TinyArmOSBuildChannel={channel}\n"
    ).encode("ascii")
    payload[768 : 768 + len(metadata)] = metadata
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

    def test_update_parser_defaults_to_main_and_accepts_nightly(self) -> None:
        parser = UPDATER["build_parser"]()
        self.assertEqual(parser.parse_args(["update", "disk.img"]).channel, "main")
        self.assertEqual(
            parser.parse_args(
                ["update", "--channel", "nightly", "disk.img", "--check"]
            ).channel,
            "nightly",
        )

    def test_release_assets_select_main_and_nightly_channels(self) -> None:
        stable_name = "TinyArmOS-v1.2.3-BOOTAA64.EFI"
        nightly_name = "TinyArmOS-nightly-BOOTAA64.EFI"
        nightly_manifest = "TinyArmOS-nightly-update.txt"
        manifest_url = f"https://github.com/test/{nightly_manifest}"
        releases = {
            UPDATER["MAIN_RELEASE_URL"]: {
                "tag_name": "v1.2.3",
                "draft": False,
                "prerelease": False,
                "assets": [stable_name, "SHA256SUMS"],
            },
            UPDATER["NIGHTLY_RELEASE_URL"]: {
                "tag_name": "nightly",
                "draft": False,
                "prerelease": True,
                "assets": [nightly_name, nightly_manifest, "SHA256SUMS"],
            },
        }
        globals_ = UPDATER["release_assets"].__globals__
        original_get = globals_["https_get"]

        def fake_get(url: str, _limit: int, _accept: str) -> bytes:
            if url == manifest_url:
                return (
                    "version=1.3.0\n"
                    "size=4096\n"
                    f"sha256={'a' * 64}\n"
                    f"url={UPDATER['NIGHTLY_IMAGE_URL']}\n"
                ).encode("ascii")
            release = releases[url]
            assets = [
                {
                    "name": name,
                    "browser_download_url": f"https://github.com/test/{name}",
                    "size": 4096 if name == stable_name or name == nightly_name else 128,
                }
                for name in release["assets"]
            ]
            return json.dumps({**release, "assets": assets}).encode("utf-8")

        globals_["https_get"] = fake_get
        try:
            self.assertEqual(
                UPDATER["release_assets"]("main")[::2],
                ("1.2.3", stable_name),
            )
            self.assertEqual(
                UPDATER["release_assets"]("nightly")[::2],
                ("1.3.0", nightly_name),
            )
        finally:
            globals_["https_get"] = original_get

    def test_update_site_publishes_both_channels(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            main = root / "main"
            nightly = root / "nightly"
            output = root / "public"
            main.mkdir()
            nightly.mkdir()
            for channel, source, version, filename, url in (
                (
                    "main",
                    main,
                    "1.2.3",
                    "TinyArmOS-v1.2.3-BOOTAA64.EFI",
                    "https://firesafetylite.github.io/TinyArmOS/"
                    "TinyArmOS-latest-BOOTAA64.EFI",
                ),
                (
                    "nightly",
                    nightly,
                    "1.3.0",
                    "TinyArmOS-nightly-BOOTAA64.EFI",
                    "https://firesafetylite.github.io/TinyArmOS/nightly/"
                    "TinyArmOS-latest-BOOTAA64.EFI",
                ),
            ):
                image = fake_efi(version, channel=channel)
                (source / filename).write_bytes(image)
                manifest_name = (
                    "TinyArmOS-update.txt"
                    if channel == "main"
                    else "TinyArmOS-nightly-update.txt"
                )
                (source / manifest_name).write_text(
                    f"version={version}\n"
                    f"size={len(image)}\n"
                    f"sha256={hashlib.sha256(image).hexdigest()}\n"
                    f"url={url}\n",
                    encoding="ascii",
                )
            SITE_BUILDER["build_site"](main, output, nightly)
            self.assertEqual(
                (output / "TinyArmOS-latest-BOOTAA64.EFI").read_bytes(),
                fake_efi("1.2.3"),
            )
            self.assertEqual(
                (output / "nightly" / "TinyArmOS-latest-BOOTAA64.EFI").read_bytes(),
                fake_efi("1.3.0", channel="nightly"),
            )
            self.assertIn("Main manifest", (output / "index.html").read_text())
            self.assertIn("Nightly beta manifest", (output / "index.html").read_text())

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
            before_system = UPDATER["Fat32Image"](disk_path.read_bytes(), 1)
            self.assertEqual(before.read_file(UPDATER["BOOT_PATH"]), old_efi)
            self.assertEqual(before_system.read_file((b"DOOMU   WAD",)), wad)
            self.assertEqual(
                before_system.read_file((b"TINYOS  NEW",)),
                b"Initialize TinyArmOS on first boot\n",
            )
            startup = before.read_file((b"STARTUP NSH",))

            backup = UPDATER["install_image"]("disk", disk_path, new_efi)

            after = UPDATER["Fat32Image"](disk_path.read_bytes())
            after_system = UPDATER["Fat32Image"](disk_path.read_bytes(), 1)
            self.assertEqual(after.read_file(UPDATER["BOOT_PATH"]), new_efi)
            self.assertEqual(after_system.read_file((b"DOOMU   WAD",)), wad)
            self.assertEqual(
                after_system.read_file((b"TINYOS  NEW",)),
                b"Initialize TinyArmOS on first boot\n",
            )
            self.assertEqual(after.read_file((b"STARTUP NSH",)), startup)
            backed_up = UPDATER["Fat32Image"](backup.read_bytes())
            self.assertEqual(backed_up.read_file(UPDATER["BOOT_PATH"]), old_efi)

    def test_legacy_utm_target_resolves_to_disk(self) -> None:
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
