#!/usr/bin/env python3
"""End-to-end native BIOS + OS smoke test. Only uses a private disposable disk."""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import re
import select
import shutil
import socket
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
ANSI = re.compile(rb"\x1b\[[0-9;?]*[A-Za-z]")


class NativeMachine:
    def __init__(self, directory: Path, qemu: str):
        self.directory = directory
        self.qmp_path = directory / "qmp.sock"
        shutil.copy2(ROOT / "build/native/TinyGPT.img", directory / "disk.img")
        command = [qemu, "-machine", "virt,secure=off,virtualization=off", "-cpu", "cortex-a53",
                   "-accel", "tcg", "-m", "256M", "-smp", "1", "-bios", str(ROOT / "build/native/TinyGPT-BIOS.bin"),
                   "-display", "none", "-serial", "stdio", "-monitor", "none", "-nic", "none",
                   "-qmp", f"unix:{self.qmp_path},server=on,wait=off", "-global", "virtio-mmio.force-legacy=false",
                   "-drive", f"file={directory}/disk.img,if=none,id=disk,format=raw",
                   "-device", "virtio-blk-device,drive=disk", "-device", "ramfb", "-device", "virtio-keyboard-device"]
        self.log = (directory / "serial.log").open("wb")
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.control = None

    def wait(self, needle: str, timeout: int = 30) -> str:
        data = b""
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if select.select([self.process.stdout], [], [], 0.1)[0]:
                chunk = os.read(self.process.stdout.fileno(), 65536)
                if not chunk:
                    break
                data += chunk
                self.log.write(chunk)
                self.log.flush()
                plain = ANSI.sub(b"", data).decode("ascii", errors="replace")
                if needle in plain:
                    return plain
                if "Firmware exception" in plain or len(data) > 1024 * 1024:
                    break
        raise AssertionError(f"Waiting for {needle!r}: {ANSI.sub(b'', data).decode(errors='replace')}")

    def read_for(self, seconds: float) -> str:
        data = b""
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            if select.select([self.process.stdout], [], [], 0.05)[0]:
                chunk = os.read(self.process.stdout.fileno(), 65536)
                if not chunk:
                    break
                data += chunk
                self.log.write(chunk)
                self.log.flush()
        return ANSI.sub(b"", data).decode("ascii", errors="replace")

    def send(self, command: str, needle: str = "tinygpt:/> ") -> str:
        self.process.stdin.write(command.encode() + b"\n")
        self.process.stdin.flush()
        return self.wait(needle)

    def qmp(self, execute: str, arguments: dict) -> dict:
        if self.control is None:
            self.control = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.control.settimeout(10)
            self.control.connect(str(self.qmp_path))
            self.reader = self.control.makefile("rwb", buffering=0)
            self.reader.readline()
            self.reader.write(b'{"execute":"qmp_capabilities"}\n')
            while "return" not in json.loads(self.reader.readline()):
                pass
        self.reader.write(json.dumps({"execute": execute, "arguments": arguments}).encode() + b"\n")
        while True:
            response = json.loads(self.reader.readline())
            if "error" in response:
                raise AssertionError(response)
            if "return" in response:
                return response

    def close(self) -> None:
        if self.process.poll() is None:
            self.process.kill()
            self.process.wait()
        self.log.close()
        if self.control:
            self.reader.close()
            self.control.close()
        self.process.stdin.close()
        self.process.stdout.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qemu", default=os.environ.get("QEMU_AARCH64", "qemu-system-aarch64"))
    parser.add_argument("--artifacts", type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="tinygpt-native-test-") as directory:
        machine = NativeMachine(Path(directory), args.qemu)
        try:
            machine.wait("Username (1-23 letters, digits, _ or -): ")
            machine.send("tester", "New password (8-64 characters): ")
            machine.send("testpass123", "Repeat password: ")
            machine.send("testpass123")
            assert "saved 17 bytes" in machine.send("write /home/native.txt native-disk-write")
            assert "native-disk-write" in machine.send("cat /home/native.txt")
            machine.send("settings", "Select: ")
            machine.send("1", "Color number: ")
            machine.send("12", "Select: ")
            machine.send("2", "Color number: ")
            machine.send("10", "Select: ")
            machine.send("3", "Background number: ")
            machine.send("2", "Select: ")
            machine.send("8", "Select: ")
            machine.send("2", "Username (blank to cancel): ")
            machine.send("guest", "Role: ")
            machine.send("1", "Password: ")
            machine.send("testpass123", "New password (8-64 characters): ")
            machine.send("guestpass123", "Repeat password: ")
            assert "Account added" in machine.send("guestpass123", "Press Enter to return")
            machine.send("", "Select: ")
            machine.send("0", "Select: ")
            machine.send("0")
            machine.send("tree /")
            machine.qmp("human-monitor-command", {"command-line": "sendkey up 1400"})
            machine.wait("-- SCROLLBACK:")
            repeated = machine.read_for(1.8)
            assert repeated.count("-- SCROLLBACK:") >= 3, "Holding Up did not repeat scrolling"
            machine.read_for(0.2)  # Drain any in-flight redraw after release.
            assert "-- SCROLLBACK:" not in machine.read_for(0.25), "Scrolling continued after key release"
            screen = Path(directory) / "scrollback.ppm"
            machine.qmp("screendump", {"filename": str(screen)})
            pixels = screen.read_bytes()
            assert pixels.startswith(b"P6\n640 480\n255\n")
            assert b"\xff\x55\x55" in pixels and b"\x55\xff\x55" in pixels and b"\x00\x00\xaa" in pixels
            machine.qmp("human-monitor-command", {"command-line": "sendkey end"})
            machine.wait("tinygpt:/>")
            machine.send("rm -rf /system", "Password: ")
            machine.send("testpass123")
            machine.send("reboot", "preos> ")
            listing = machine.send("ls /", "preos> ")
            assert "<DIR>  system" not in listing
            machine.send("repair 2", "Username: ")
            machine.send("tester", "Password: ")
            assert "target partition is bootable" in machine.send("testpass123", "preos> ")
            machine.send("boot 2", "Username: ")
            machine.send("guest", "Password: ")
            machine.send("guestpass123")
            assert "native-disk-write" in machine.send("cat /home/native.txt")
            assert "administrator access required" in machine.send("rm -rf /system")
            machine.send("logout", "Username: ")
            machine.send("tester", "Password: ")
            machine.send("testpass123")
            machine.send("rm -rf /", "Password: ")
            machine.send("testpass123")
            machine.send("reboot", "preos> ")
            machine.send("repair 2", "Username: ")
            machine.send("tester", "Password: ")
            assert "target partition is bootable" in machine.send("testpass123", "preos> ")
            machine.process.stdin.write(b"shutdown\n")
            machine.process.stdin.flush()
            machine.process.wait(timeout=10)
            assert machine.process.returncode == 0
            print("PASS: native BIOS -> disk OS, login/accounts, writes, graphics/keyboard, held-key scrolling/release, colored scrollback, deletion, repair, root wipe, restart")
            if args.artifacts:
                args.artifacts.mkdir(parents=True, exist_ok=True)
                shutil.copy2(screen, args.artifacts / screen.name)
                shutil.copy2(Path(directory) / "serial.log", args.artifacts / "native-smoke.log")
        finally:
            machine.close()


if __name__ == "__main__":
    main()
