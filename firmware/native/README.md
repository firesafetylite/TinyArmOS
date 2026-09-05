# TinyGPT native firmware and system

This target boots the real TinyGPT application without EDK II, an EFI executable,
or UEFI firmware services. TinyGPT-owned adapters implement the subset of the
old interface-shaped C types that the shared application uses. These are not a
general UEFI implementation. `firmware/bios` also retains its smaller read-only
bring-up target; that target is not this full system.

## Build and validate

Requires Zig 0.14.1 (`pip install ziglang==0.14.1`), Python, and QEMU for tests:

```sh
make native
QEMU_AARCH64=qemu-system-aarch64 make native-test
make test
```

Outputs in `build/native`:

- `TinyGPT-BIOS.bin`: 64 MiB flash image, with independent firmware recovery and
  an embedded copy of the native system for explicitly authorized repair.
- `TINYGPT.ELF`: separately disk-loaded OS, native ABI 3.
- `TinyGPT.img`: new 128 MiB GPT/FAT disk. Do **not** replace an existing user disk
  with this factory image.

The BIOS validates ELF load ranges and ABI, loads `TINYGPT.ELF` from the selected
system partition, and passes TinyGPT-owned hardware callbacks. Esc/R during
startup enters firmware recovery. Missing system files no longer silently
regenerate during startup; administrator-authorized repair restores them.

Tested platform (do not blindly swap an existing UTM PFlash):

```sh
qemu-system-aarch64 \
  -machine virt,secure=off,virtualization=off -cpu cortex-a53 -accel tcg \
  -m 256M -smp 1 -bios build/native/TinyGPT-BIOS.bin \
  -global virtio-mmio.force-legacy=false \
  -drive file=build/native/TinyGPT.img,if=none,id=disk,format=raw \
  -device virtio-blk-device,drive=disk -device ramfb \
  -device virtio-keyboard-device -serial stdio
```

The graphical text console is 640×480 with an 80×25 terminal using the original
8×19 console font (unscaled glyph rows); UART is available in parallel. Holding
an arrow starts navigation/scrollback repeat after 400 ms, then repeats at 20 Hz.
Queued releases are processed before any repeat; printable keys are not synthesized. Graphics output for application code is supplied through the native
framebuffer adapter. Keyboard input uses modern VirtIO-MMIO. This target does
not drive VirtIO PCI storage, USB keyboards, or the old virtio-ramfb PCI setup.

Writable block access requires negotiated flush support. FatFs provides FAT16/
FAT32 and long filenames; the shared application retains its direct-file journal,
mirrored GPT checks, protected recovery partition, account database, permissions,
Settings, and editor. The memory layout requires 256 MiB RAM, one CPU, and the
specified QEMU board. No MMU isolation or verified boot is provided.

`tools/test_native.py` boots only a disposable disk and exercises actual native
boot, setup/login, Settings account creation, persistent writes, graphics and
VirtIO keyboard input, custom-colored scrollback, missing-system recovery,
authenticated repair, standard-user denial, root wipe, reboot, and shutdown.
Doom gameplay and real UTM migration are separate validations; the smoke test
does not claim to cover them. Neither the test nor the builder edits the real VM.

## Migration safety and remaining limitations

Stop the guest cleanly and make a fresh **complete VM backup** before changing
configuration, flash, or disk. Install matching BIOS and ELF artifacts together;
`tools/native_image.py:install_system` can prepare an updated disk in memory while
verifying that bytes outside the selected system partition remain unchanged.
Preserve existing data and recovery/accounts, and verify disk read-back before
starting the converted VM. Remove EDK II and variable-store drives from the active
boot configuration, disable automatic UEFI firmware, and use the hardware above.
Retain the complete old VM backup for rollback.

Native guest HTTP/TLS and guest-downloaded upgrades are not implemented. Native
builds reject EFI update payloads; use paired host-built firmware/system updates.
The legacy EFI build/release tooling remains for existing EFI installations and
is not involved in building or booting this native target. Account salts use the
existing non-cryptographic fallback because the native platform has no RNG
protocol yet; local authentication is not disk encryption or offline tamper
protection. FAT and account redundancy do not guarantee arbitrary power-loss
atomicity. Native payloads and firmware are privileged and not sandboxed.

Third-party source retains its notices: FatFs R0.16 from
https://elm-chan.org/fsw/ff/ (writable FAT, LFN, CP437; named volume limit extended
from 10 to 16), and the original 8×19 font bitmaps under BSD-2-Clause-Patent
([font provenance and license](../../third_party/console-font/README.md)). Only
font data is reused, not EDK II firmware code or services. The binary font notice
is copied to `build/native/CONSOLE-FONT-LICENSE.txt`. Existing PureDOOM/Freedoom
licenses remain applicable to the shared game code/assets.
