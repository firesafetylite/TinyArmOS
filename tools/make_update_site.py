#!/usr/bin/env python3
"""Validate release update assets and build the main/nightly Pages site."""

from __future__ import annotations

import argparse
import hashlib
import re
import runpy
import shutil
from pathlib import Path
from typing import Dict, Optional

ROOT = Path(__file__).resolve().parents[1]
UPDATER = runpy.run_path(str(ROOT / "tinyarmos"), run_name="update_site_validation")
VERSION_PATTERN = r"(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)(?:\.(?:0|[1-9][0-9]*))?"
CHANNELS = {
    "main": {
        "manifest": "TinyArmOS-update.txt",
        "binary": re.compile(rf"TinyArmOS-v({VERSION_PATTERN})-BOOTAA64\.EFI"),
        "url": (
            "https://firesafetylite.github.io/TinyArmOS/"
            "TinyArmOS-latest-BOOTAA64.EFI"
        ),
        "directory": "",
    },
    "nightly": {
        "manifest": "TinyArmOS-nightly-update.txt",
        "binary": re.compile(
            rf"TinyArmOS-v({VERSION_PATTERN})-nightly-BOOTAA64\.EFI"
        ),
        "url": (
            "https://firesafetylite.github.io/TinyArmOS/nightly/"
            "TinyArmOS-latest-BOOTAA64.EFI"
        ),
        "directory": "nightly",
    },
}


class SiteError(RuntimeError):
    """A release asset cannot be published safely."""


def parse_manifest(path: Path) -> Dict[str, str]:
    try:
        lines = path.read_text(encoding="ascii").splitlines()
    except (OSError, UnicodeDecodeError) as error:
        raise SiteError(f"could not read ASCII manifest {path}: {error}") from error
    entries: Dict[str, str] = {}
    for line in lines:
        if not re.fullmatch(r"[a-z0-9]+=[ -~]+", line):
            raise SiteError(f"{path} has invalid manifest syntax")
        key, value = line.split("=", 1)
        if key in entries:
            raise SiteError(f"{path} contains duplicate field {key}")
        entries[key] = value
    if set(entries) != {"version", "size", "sha256", "url"}:
        raise SiteError(f"{path} has unexpected manifest fields")
    try:
        UPDATER["version_tuple"](entries["version"])
    except UPDATER["UpdateError"] as error:
        raise SiteError(str(error)) from error
    if not re.fullmatch(r"[0-9]+", entries["size"]):
        raise SiteError(f"{path} has an invalid size")
    if not re.fullmatch(r"[0-9a-f]{64}", entries["sha256"]):
        raise SiteError(f"{path} has an invalid SHA-256 digest")
    return entries


def publish_channel(source: Path, output: Path, channel: str) -> None:
    config = CHANNELS[channel]
    manifest_path = source / str(config["manifest"])
    entries = parse_manifest(manifest_path)
    if entries["url"] != config["url"]:
        raise SiteError(f"{channel} manifest has an unexpected download URL")
    candidates = []
    binary_pattern = config["binary"]
    assert isinstance(binary_pattern, re.Pattern)
    for path in source.iterdir():
        match = binary_pattern.fullmatch(path.name)
        if match:
            candidates.append((path, match.group(1)))
    if len(candidates) != 1:
        raise SiteError(f"expected one {channel} EFI asset, found {len(candidates)}")
    binary, filename_version = candidates[0]
    if filename_version != entries["version"]:
        raise SiteError(f"{channel} EFI filename does not match its manifest version")
    data = binary.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if entries["size"] != str(len(data)) or entries["sha256"] != digest:
        raise SiteError(f"{channel} manifest does not match its EFI asset")
    try:
        UPDATER["validate_efi"](data, entries["version"])
    except UPDATER["UpdateError"] as error:
        raise SiteError(str(error)) from error
    destination = output / str(config["directory"])
    destination.mkdir(parents=True, exist_ok=True)
    (destination / "TinyArmOS-latest-BOOTAA64.EFI").write_bytes(data)
    (destination / "TinyArmOS-update.txt").write_text(
        f"version={entries['version']}\n"
        f"size={len(data)}\n"
        f"sha256={digest}\n"
        f"url={config['url']}\n",
        encoding="ascii",
    )


def build_site(main: Path, output: Path, nightly: Optional[Path] = None) -> None:
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)
    publish_channel(main, output, "main")
    has_nightly = nightly is not None and (
        nightly / str(CHANNELS["nightly"]["manifest"])
    ).is_file()
    if has_nightly:
        assert nightly is not None
        publish_channel(nightly, output, "nightly")
    nightly_link = (
        '<li><a href="nightly/TinyArmOS-update.txt">Nightly beta manifest</a></li>'
        if has_nightly
        else ""
    )
    (output / "index.html").write_text(
        "<!doctype html>\n"
        '<meta charset="utf-8">\n'
        "<title>TinyArmOS update channels</title>\n"
        "<h1>TinyArmOS update channels</h1>\n"
        '<ul><li><a href="TinyArmOS-update.txt">Main manifest</a></li>'
        f"{nightly_link}</ul>\n",
        encoding="utf-8",
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--main", required=True, type=Path)
    parser.add_argument("--nightly", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    try:
        build_site(args.main, args.output, args.nightly)
    except (OSError, SiteError) as error:
        raise SystemExit(f"make_update_site: {error}") from error


if __name__ == "__main__":
    main()
