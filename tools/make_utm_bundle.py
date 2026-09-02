#!/usr/bin/env python3
"""Package the boot disk as a double-clickable legacy-compatible UTM bundle."""

from __future__ import annotations

import plistlib
import shutil
import sys
from pathlib import Path


def main() -> None:
    if len(sys.argv) not in (3, 5):
        print(
            f"usage: {sys.argv[0]} DISK.img OUTPUT.utm [UEFI_CODE.fd UEFI_VARS.fd]",
            file=sys.stderr,
        )
        raise SystemExit(2)
    disk = Path(sys.argv[1]).resolve()
    bundle = Path(sys.argv[2]).resolve()
    firmware = Path(sys.argv[3]).resolve() if len(sys.argv) == 5 else None
    variables = Path(sys.argv[4]).resolve() if len(sys.argv) == 5 else None
    for image in (firmware, variables):
        if image is not None and not image.is_file():
            raise FileNotFoundError(image)
    if bundle.exists():
        shutil.rmtree(bundle)
    images = bundle / "Images"
    images.mkdir(parents=True)
    shutil.copyfile(disk, images / "disk-0.img")
    if firmware is not None and variables is not None:
        shutil.copyfile(firmware, images / "tinyarmos-uefi-code.fd")
        shutil.copyfile(variables, images / "tinyarmos-uefi-vars.fd")

    disk_drive = {
        "ImagePath": "disk-0.img",
        "ImageType": "disk",
        "InterfaceType": "virtio",
        "Removable": False,
    }
    if firmware is not None:
        drives = [
            {
                "ImagePath": "tinyarmos-uefi-code.fd",
                "ImageType": "disk",
                "InterfaceType": "pflash",
                "Removable": False,
            },
            {
                "ImagePath": "tinyarmos-uefi-vars.fd",
                "ImageType": "disk",
                "InterfaceType": "pflash",
                "Removable": False,
            },
            disk_drive,
        ]
    else:
        drives = [disk_drive]

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
        "Drives": drives,
        "Info": {
            "Icon": "terminal",
            "IconCustom": False,
            "Notes": "Tiny ARM64 UEFI shell with MiniFS2, native Freedoom, and GitHub updates.",
        },
        "Input": {"InputLegacy": False},
        "Networking": {
            "NetworkEnabled": True,
            "NetworkMode": "emulated",
            "NetworkCard": "virtio-net-pci",
        },
        "Printing": {},
        "Sharing": {"ClipboardSharing": False, "DirectorySharing": False},
        "Sound": {"SoundEnabled": False},
        "System": {
            "Architecture": "aarch64",
            "Target": "virt",
            "CPU": "default",
            "CPUFlags": [],
            "CPUCount": 1,
            "Memory": 256,
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
