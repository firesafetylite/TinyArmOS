# Standalone TinyGPT BIOS — experimental bring-up

**This is an independent ARM64 firmware prototype, not yet a replacement for the
current TinyGPT VM firmware.** It does not contain, build, load, or call EDK II.
It starts at the virtual CPU reset vector; its recovery console lives in PFlash
and starts even when no disk is attached. It is not a renamed UEFI application.

The existing OS and full pre-OS environment in `src/uefi.c` still depend on UEFI.
This prototype cannot run `BOOTAA64.EFI`. Do **not** replace the working VM's
`TinyGPT-QEMU_EFI.fd` with this image: that would lose the current graphical
console, PCI storage access, login, repair tools, and ability to boot TinyGPT.
The old firmware build is deliberately retained until a functional migration is
ready. No VM configuration or disk is changed by the new build/test commands.

## Implemented and tested

- AArch64 reset vector, RAM/stack initialization, exception vectors, serial console.
- Firmware-resident `help` / Esc recovery menu; `info`, `reboot`, `shutdown`.
- A synchronous, timeout-bounded **read-only** VirtIO 1.0 MMIO block driver.
- Primary GPT/header/entry CRC validation, extent and overlap checks.
- Bounded FAT16/FAT32 directory listing and file reads using short (8.3) names.
- ELF64 AArch64 native payload validation and boot handoff; EFI/PE is rejected.
- Automated TCG boots with no disk, corrupt GPT, both FAT formats, Esc/reboot,
  and a tiny native payload. Test disks are synthetic and attached read-only.
- Host C tests execute the actual GPT/FAT and ELF parsers against malformed inputs.

Example commands:

```text
bios> partitions
bios> ls 1 /EFI/BOOT
bios> cat 2 /TINYGPT.NEW
bios> boot 1 /EFI/BOOT/NATIVE.ELF
```

The ELF in the smoke test is only a UART hello/return fixture, **not a port of the
TinyGPT OS**. A returning payload must obey the AArch64 C calling convention;
untrusted payloads run privileged and have no sandbox.

## Build and isolated tests

Zig 0.14.1 and Python 3.9+ are sufficient to build. There are no EDK II source,
BaseTools, host cross-GCC, UEFI library, or certificate-bundle dependencies.

```sh
PATH="/tmp/tinygpt-venv/bin:$PATH" make bios
# With qemu-system-aarch64 installed:
PATH="/tmp/tinygpt-venv/bin:$PATH" make bios-test
make test
```

Outputs are deliberately separate from the working EFI build:

```text
build/bios/TinyGPT-BIOS.elf   linked firmware/debug symbols
build/bios/TinyGPT-BIOS.bin   64 MiB PFlash image
```

On macOS, UTM's QEMU framework can optionally supply an isolated TCG emulator
without opening or modifying a registered UTM VM:

```sh
cc tools/utm_qemu_runner.c \
  -Wl,-rpath,/Applications/UTM.app/Contents/Frameworks -o /tmp/tinygpt-qemu
PATH="/tmp/tinygpt-venv/bin:$PATH" QEMU_AARCH64=/tmp/tinygpt-qemu make bios-test
```

This adapter was tested against UTM's QEMU 10.0.2 lifecycle exports. They are not
a stable API; use the ordinary QEMU executable if a future UTM changes them.

For manual diskless testing (the console is your terminal):

```sh
qemu-system-aarch64 \
  -machine virt,secure=off,virtualization=off -cpu cortex-a53 -accel tcg \
  -m 128M -smp 1 -bios build/bios/TinyGPT-BIOS.bin \
  -display none -serial stdio -monitor none -nic none
```

A test disk requires exactly one `virtio-blk-device`, **not** `virtio-blk-pci`,
with `-global virtio-mmio.force-legacy=false`. Always use a disposable fixture
and `readonly=on`; never attach a running VM's disk. The firmware has no block
write path, but a native payload can access hardware directly.

## Current platform and native payload contract

Supported/tested platform: QEMU `virt`, secure and virtualization extensions off,
one Cortex-A53 CPU, 128 MiB RAM, PL011 UART at `0x09000000`, modern VirtIO MMIO at
`0x0a000000` with 32 transports at a `0x200` stride. No device-tree discovery or
PCI enumeration is implemented. These assumptions do not describe the existing
UTM VM's VirtIO-PCI graphical configuration.

- Firmware executes from flash at zero. Writable state starts at `0x40100000`;
  the stack top is `0x40800000`. MMU/caches remain disabled; IRQs are masked.
- Native ELF payloads must be little-endian AArch64 `ET_EXEC`, with identical
  virtual/physical load addresses inside `[0x41000000, 0x44000000)`.
- Up to 16 non-overlapping LOAD segments; at most 8 MiB of file data. Every
  file span, segment size, alignment and entry point is validated before copying.
- Staging occupies `[0x45000000, 0x45800000)`, outside firmware/payload memory.
- Entry runs at EL1 with x0 pointing to five read-only 64-bit words: magic
  `0x534f494254504754`, ABI version 1, UART address, RAM base, guaranteed RAM size.
  There are no UEFI boot/runtime services. This is a development ABI, not a
  stable TinyGPT OS ABI, Linux boot protocol, or general EFI loader.
- Power controls use QEMU's PSCI HVC interface. Other platforms are unsupported.

## Required before replacing the working VM firmware

1. Port the current pre-OS and OS code off UEFI (or implement TinyGPT-owned
   compatibility services); separate disk-loaded OS code from firmware recovery.
2. Implement and test VirtIO PCI storage, safe writable FAT/flush behavior, full
   partition management and repair, keyboard input and graphical console.
3. Restore the account authorization and recovery policy without depending on
   disk contents that may be missing. There is currently **no firmware password
   gate, Secure Boot, disk encryption, or payload authentication**.
4. Implement a native system-install/update path; HTTPS/TLS is also currently
   supplied by EDK II and is not present here.
5. Test migration in a separate cloned UTM VM, including missing/corrupt system
   files, disk loss, accounts, reboot, and failed writes. Only then replace the
   original VM's firmware, with known-good firmware/disk/variable-store backups.

Deleting or rebuilding the guest system must never be an automatic firmware
startup action. This prototype is read-only and does not recreate `/system`.
