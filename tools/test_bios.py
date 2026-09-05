#!/usr/bin/env python3
"""Boot independent firmware under QEMU TCG using only synthetic read-only disks."""
from __future__ import annotations
import argparse
import hashlib
import os
from pathlib import Path
import select
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests"))
from bios_fixture import disk_fixture


class Machine:
    def __init__(self, qemu: str, rom: Path, disk: Path | None = None):
        command = [qemu, "-machine", "virt,secure=off,virtualization=off", "-cpu", "cortex-a53",
                   "-accel", "tcg", "-m", "128M", "-smp", "1", "-bios", str(rom),
                   "-display", "none", "-serial", "stdio", "-monitor", "none", "-nic", "none"]
        if disk:
            command += ["-global", "virtio-mmio.force-legacy=false", "-drive",
                        f"file={disk},if=none,id=testdisk,format=raw,readonly=on",
                        "-device", "virtio-blk-device,drive=testdisk"]
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.STDOUT)

    def prompt(self) -> str:
        data = b""
        deadline = time.monotonic() + 15
        while b"bios> " not in data and time.monotonic() < deadline:
            if select.select([self.process.stdout], [], [], 0.2)[0]:
                chunk = os.read(self.process.stdout.fileno(), 65536)
                if not chunk:
                    break
                data += chunk
                if len(data) > 128 * 1024:
                    raise AssertionError("Unbounded firmware output")
        text = data.decode("ascii", errors="replace")
        if b"bios> " not in data or "Firmware exception" in text:
            raise AssertionError(f"Firmware failed to reach its prompt:\n{text}")
        return text

    def command(self, text: str) -> str:
        self.process.stdin.write(text.encode("ascii") + b"\n")
        self.process.stdin.flush()
        return self.prompt()

    def shutdown(self) -> None:
        self.process.stdin.write(b"shutdown\n")
        self.process.stdin.flush()
        output, _ = self.process.communicate(timeout=10)
        assert self.process.returncode == 0, output

    def close(self) -> None:
        if self.process.poll() is None:
            self.process.kill()
            self.process.wait()
        for stream in (self.process.stdin, self.process.stdout):
            if stream and not stream.closed:
                stream.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qemu", default=os.environ.get("QEMU_AARCH64", "qemu-system-aarch64"))
    parser.add_argument("--rom", type=Path, default=ROOT / "build/bios/TinyGPT-BIOS.bin")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="tinygpt-bios-test-") as temp:
        disk = disk_fixture(Path(temp))
        before = hashlib.sha256(disk.read_bytes()).digest()
        machine = Machine(args.qemu, args.rom, disk)
        try:
            assert "TinyGPT native reset reached" in machine.prompt()
            assert "262144" in machine.command("info")
            assert "TinyGPT Recovery" in machine.command("partitions")
            assert "NATIVE.ELF" in machine.command("ls 1 /EFI/BOOT")
            assert "fs0:" in machine.command("cat 1 /STARTUP.NSH")
            assert "DOOMU.WAD" in machine.command("ls 2 /")
            assert "Initialize TinyGPT" in machine.command("cat 2 /TINYGPT.NEW")
            assert "Boot refused" in machine.command("boot 1 /STARTUP.NSH")
            assert "NATIVE_PAYLOAD_OK" in machine.command("boot 1 /EFI/BOOT/NATIVE.ELF")
            assert "Path unavailable" in machine.command("cat 1 /../STARTUP.NSH")
            assert "Input too long" in machine.command("a" * 200)
            machine.process.stdin.write(b"\x1b")
            machine.process.stdin.flush()
            assert "=== TinyGPT BIOS" in machine.prompt()
            machine.process.stdin.write(b"reboot\n")
            machine.process.stdin.flush()
            assert "TinyGPT native reset reached" in machine.prompt()
            machine.shutdown()
        finally:
            machine.close()
        assert hashlib.sha256(disk.read_bytes()).digest() == before, "Firmware changed its read-only fixture"
        print("PASS: reset, native ELF handoff, Esc, FAT16/FAT32, read-only disk, reboot, shutdown")
        with disk.open("r+b") as image:
            image.seek(512 + 16)
            image.write(b"\0" * 4)  # Invalid primary-header CRC must fail closed.
        machine = Machine(args.qemu, args.rom, disk)
        try:
            machine.prompt()
            assert "Invalid or unreadable primary GPT" in machine.command("partitions")
            machine.shutdown()
        finally:
            machine.close()
        machine = Machine(args.qemu, args.rom)
        try:
            assert "TinyGPT native reset reached" in machine.prompt()
            assert "No supported disk" in machine.command("partitions")
            machine.shutdown()
        finally:
            machine.close()
        print("PASS: corrupt GPT rejected; recovery boots without any disk or EDK II image")


if __name__ == "__main__":
    main()
