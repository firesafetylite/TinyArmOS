#!/usr/bin/env python3
"""Package the boot disk as a double-clickable legacy-compatible UTM bundle."""

from __future__ import annotations

import plistlib
import shutil
import sys
from pathlib import Path


def main() -> None:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} DISK.img OUTPUT.utm", file=sys.stderr)
        raise SystemExit(2)
    disk = Path(sys.argv[1]).resolve()
    bundle = Path(sys.argv[2]).resolve()
    if bundle.exists():
        shutil.rmtree(bundle)
    images = bundle / "Images"
    images.mkdir(parents=True)
    shutil.copyfile(disk, images / "disk-0.img")

    # UTM's current releases migrate this stable version-2 QEMU schema on import.
    config = {
        "ConfigurationVersion": 2,
        "Debug": {},
        "Display": {
            "ConsoleOnly": False,
            "DisplayCard": "virtio-ramfb",
            "DisplayFitScreen": False,
            "DisplayRetina": False,
            "DisplayUpscaler": "nearest",
            "DisplayDownscaler": "linear",
            "ConsoleFont": "Menlo-Regular",
            "ConsoleFontSize": 14,
            "ConsoleTheme": "Default",
        },
        "Drives": [
            {
                "ImagePath": "disk-0.img",
                "ImageType": "disk",
                "InterfaceType": "virtio",
                "Removable": False,
            }
        ],
        "Info": {
            "Icon": "terminal",
            "IconCustom": False,
            "Notes": "Tiny ARM64 shell with MiniFS2, recovery snapshots, and native Freedoom.",
        },
        "Input": {"InputLegacy": False},
        "Networking": {"NetworkEnabled": False},
        "Printing": {},
        "Sharing": {"ClipboardSharing": False, "DirectorySharing": False},
        "Sound": {"SoundEnabled": False},
        "System": {
            "Architecture": "aarch64",
            "Target": "virt",
            "CPU": "default",
            "CPUFlags": [],
            "CPUCount": 1,
            "Memory": 128,
            "JITCacheSize": 0,
            "ForceMulticore": False,
            "BootDevice": "",
            "BootUefi": True,
            "RngEnabled": True,
            "UseHypervisor": False,
            "RTCUseLocalTime": False,
            "ForcePS2Controller": False,
            "AddArgs": [],
            "SystemUUID": "E056959D-53D4-4C21-9FF4-61726D363475",
        },
    }
    with (bundle / "config.plist").open("wb") as handle:
        plistlib.dump(config, handle, fmt=plistlib.FMT_XML, sort_keys=False)
    print(f"Created {bundle}")


if __name__ == "__main__":
    main()
