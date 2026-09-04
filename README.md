
# (DISCLAIMER: TinyArmOS is a project fully managed by ChatGPT codex, everything in this github besides this message your seeing was created and is overseen by chatgpt, if you have any issues email me at 8minecraft.19@gmail.com)

# TinyGPT v0.1.6

[![Main CI](https://github.com/firesafetylite/TinyGPT/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/firesafetylite/TinyGPT/actions/workflows/ci.yml)
[![Nightly](https://github.com/firesafetylite/TinyGPT/actions/workflows/nightly.yml/badge.svg?branch=nightly)](https://github.com/firesafetylite/TinyGPT/actions/workflows/nightly.yml)
[![Release](https://img.shields.io/github/v/release/firesafetylite/TinyGPT?display_name=tag)](https://github.com/firesafetylite/TinyGPT/releases/latest)
[![License: GPL-2.0-only](https://img.shields.io/badge/license-GPL--2.0--only-blue.svg)](LICENSE)

**TinyGPT is a small, freestanding ARM64 operating system that boots directly through UEFI.** It provides a persistent shell, the MiniFS2 filesystem, an isolated pre-OS recovery environment, an interactive text editor, firmware-backed updates, and a native Freedoom port. It is built from scratch in C and does not use a Unix or Linux kernel.

> TinyGPT is the project name; the operating system is not a language model. The disk image does use a standard GUID Partition Table (GPT).

## Highlights

- Boots as a native AArch64 UEFI application
- Ships as one 128 MiB GPT/FAT disk image
- Stores data in two alternating, checksummed MiniFS2 snapshots
- Verifies the filesystem and operating system before every normal boot
- Keeps recovery tools on a protected partition that survives an OS wipe
- Supports multiple bootable system/data partitions
- Includes protected `/system`, `/apps`, and `/lost+found` trees
- Provides a full-screen text editor, persistent settings, and 256-line scrollback
- Updates through UEFI HTTP/TLS or the host-side `tinygpt` command
- Runs Freedoom natively through UEFI graphics and keyboard protocols

## Download and boot

Download `TinyGPT-vVERSION.img` from the [latest release](https://github.com/firesafetylite/TinyGPT/releases/latest), then verify it with the release's `SHA256SUMS` file. The `.img` file is the only maintained boot distribution; old `.utm` bundles are supported only as explicit migration targets by the host updater.

### UTM on Apple silicon

1. Create an **Emulate** virtual machine.
2. Choose **Other** as the operating system and **ARM64** as the architecture.
3. Enable UEFI boot and allocate at least 256 MiB of RAM.
4. Import `TinyGPT-vVERSION.img` as a non-removable VirtIO drive.
5. For networking, use **Shared Network** with a `virtio-net-pci` adapter.
6. Start the VM. UEFI should load `EFI/BOOT/BOOTAA64.EFI` automatically.

If the firmware opens its own shell instead, run:

```text
fs0:\EFI\BOOT\BOOTAA64.EFI
```

## Disk layout

The maintained image has two initial partitions and unallocated expansion space:

| GPT partition | Label | Format | Purpose |
| ---: | --- | --- | --- |
| 1 | `TINYRECOV` | FAT16 ESP | Protected bootloader and pre-OS recovery environment |
| 2 | `TINYGPT` | FAT32 | Initial TinyGPT system, MiniFS2 snapshots, settings, and Freedoom data |
| 3+ | User supplied | FAT16 | Additional system/data partitions created from recovery |

Important files include:

```text
Partition 1: TINYRECOV
└── EFI/BOOT/BOOTAA64.EFI

Partition 2: TINYGPT
├── TINYGPT.NEW              first-boot installation marker
├── DOOMU.WAD                Freedoom Phase 1
└── TINYFS0.BIN/TINYFS1.BIN alternating MiniFS2 snapshots after first boot
```

Partition 1 is never exposed through normal filesystem commands. Partition 2 retains its original extent, while the remaining image space is available to `partition add`.

## Boot verification and recovery

Before launching the shell, TinyGPT verifies:

1. ARM64 UEFI firmware and the platform timer
2. The TinyGPT boot volume
3. The newest valid MiniFS2 snapshot
4. Filesystem structure and active-file checksums
5. The installed operating-system state

Press **Enter** during the two-second startup window to open the partition selector. Use Up/Down to choose a target, Enter to boot, **S** to save the default, or **R** to enter recovery. Recovery also opens automatically when the selected system is missing, cannot be mounted, or fails integrity checks. It never silently repairs a damaged installation.

The pre-OS environment supports:

```text
partitions                     list registered GPT partitions
partition add MIB NAME         create a named FAT partition (minimum 4 MiB)
partition name N NAME          rename a non-protected partition
use N                          select a partition for file navigation
order N                        save a partition as the default
scan N                         verify a partition and both snapshots
repair N                       repair or install TinyGPT on a partition
rollback N                     load a partition's previous snapshot
pwd / ls / cd                  navigate the selected partition
cat / stat / tree              inspect files and metadata
reset N                        reset and reinstall after confirmation
scroll / scroll clear          inspect or clear recovery scrollback
boot [N]                       verify and start a partition
reboot / shutdown              restart or power off
```

Names may contain 1–11 letters, digits, underscores, or hyphens. They are normalized to uppercase and must be unique. A newly created partition is activated after one required firmware reboot, then TinyGPT initializes it automatically.

The regular shell's `partitions` command is read-only. Partition creation, naming, repair, reset, rollback, boot-order changes, and recovery-partition access remain pre-OS-only.

## MiniFS2 and the shell

MiniFS2 is a persistent hierarchical filesystem designed for TinyGPT. It supports absolute and relative paths, `~`, `cd -`, nested directories, per-node FNV-1a checksums, 96 fixed nodes, and files up to 8191 bytes. Every saved state alternates between two complete snapshots.

### Filesystem commands

```text
pwd
ls [PATH]
tree [PATH]
cd [PATH|-]
cat PATH
write PATH [TEXT]
append PATH TEXT
mkdir PATH
rm PATH
rm -rf PATH
rmdir PATH
cp SOURCE DEST
mv SOURCE DEST
stat PATH
df
fsck
textedit [PATH]
```

Example:

```text
mkdir /home/projects
write /home/projects/hello.txt hello-arm64
cat /home/projects/hello.txt
cp /home/projects/hello.txt /tmp
```

### Protected system trees

The `/system`, `/apps`, and `/lost+found` trees are locked at every boot. They remain readable while locked.

```text
protect status
protect unlock
# Type: UNLOCK
protect lock
```

Unlocking lasts only for the current boot. Creation, writes, copies, moves, renames, and removals all enforce ancestor protection.

### Text Editor

Open a file directly or launch the interactive picker:

```text
textedit /home/notes/todo.txt
textedit
```

The editor soft-wraps without inserting extra newlines. Arrow keys move through characters and wrapped display rows, Backspace/Delete removes text, and Enter inserts a line break. Press **F2** or **Ctrl+S** to save; press Esc twice to discard unsaved changes. Protected files open read-only until protection is unlocked.

### Settings

Run `settings` for the full-screen configuration interface. It controls:

- Default text and accent colors
- OS console background color with live preview
- Whether the current path appears in the prompt
- Whether the shell starts in `/` or `/home`
- Whether 256-line scrollback is enabled

Changes save automatically to `/home/.tinygptrc` and survive reboots and OS updates.

### Scrollback

```text
Up/Down     scroll one line
PageUp      move one page toward older output
PageDown    move one page toward newer output
Home        jump to the oldest page
End or Esc  return to the live prompt
scroll      show status and controls
scroll clear erase retained scrollback
```

Typing while viewing older output returns to the live prompt.

### Other shell commands

```text
help
clear
scroll [clear]
echo [TEXT]
info
uptime
partitions
textedit [PATH]
settings
protect [status|unlock|lock]
update [check] [main|nightly]
doom
reboot
shutdown
```

## Freedoom

Run `doom` to launch Freedoom 0.13.0 Phase 1 through the integrated [PureDOOM](https://github.com/Daivuk/PureDOOM) engine.

```text
WASD        move and strafe
Arrow keys  turn and move
F           fire
E           use/open
Enter       select
Esc         menu
Q or F12    return to TinyGPT
```

Graphics and input use UEFI protocols directly; no Linux layer is involved. Sound and music are disabled, and Doom can be launched once per boot.

## Release channels and updates

| Channel | Branch | Purpose | Published artifact |
| --- | --- | --- | --- |
| Stable | [`main`](https://github.com/firesafetylite/TinyGPT/tree/main) | Versioned releases | `TinyGPT-vVERSION.img` |
| Nightly beta | [`nightly`](https://github.com/firesafetylite/TinyGPT/tree/nightly) | Rolling development build | `TinyGPT-nightly.img` |

Each nightly push replaces the existing [`nightly` prerelease](https://github.com/firesafetylite/TinyGPT/releases/tag/nightly). Stable releases are tagged from `main`. Update commands default to `main`; opting into nightly must be explicit.

### In-OS updater

```text
update check
update main
update check nightly
update nightly
```

The updater validates channel compatibility, the manifest, SHA-256 digest, file size, and ARM64 EFI structure before replacing the bootloader. It retains `EFI/BOOT/BOOTAA64.BAK` and requires a reboot to start the installed version.

Guest updates require firmware that exposes UEFI HTTP/TLS. For UTM, keep `TinyGPT-QEMU_EFI.fd` attached as read-only PFlash and `TinyGPT-QEMU_EFI-vars.fd` as writable PFlash variables, with Emulated networking and VirtIO RNG enabled. These firmware files are separate from `TinyGPT.img`; replacing the disk does not replace them.

### Host-side fallback

If guest firmware does not provide HTTP/TLS, shut down the VM, detach the image from all running VMs, download `tinygpt` from the release, and run:

```bash
chmod +x tinygpt
./tinygpt update --channel main /path/to/TinyGPT.img
./tinygpt update --channel nightly /path/to/TinyGPT.img
```

The host command uses Python's HTTPS stack and changes only `EFI/BOOT/BOOTAA64.EFI` on the recovery partition. MiniFS2 snapshots, settings, user files, other partitions, and Freedoom remain intact. A backup is created before replacement.

## Build and test

Requirements:

- Bash and Make
- Python 3.9 or newer
- Zig 0.14.1

Use a pinned Zig installation without modifying the repository:

```bash
python3 -m venv /tmp/tinygpt-venv
/tmp/tinygpt-venv/bin/pip install ziglang==0.14.1
PATH="/tmp/tinygpt-venv/bin:$PATH" make test
PATH="/tmp/tinygpt-venv/bin:$PATH" make
```

A successful build produces:

```text
build/BOOTAA64.EFI
build/TinyGPT.img
```

`build.sh` compiles the freestanding C source, creates the recovery and system partitions, embeds Freedoom, and writes the canonical 128 MiB image. The image builder and host updater use only Python's standard library.

CI checks the Python tools and source invariants, verifies that the EFI output is an AArch64 PE32+ application, validates the GPT/FAT layout, and confirms the embedded EFI and Freedoom data. Changes to boot or runtime behavior should also be tested in an ARM64 UEFI VM when practical.

The optional `tools/build_uefi_firmware.sh` script builds the custom HTTP/TLS-capable developer firmware. Run `make clean` to remove generated output; everything under `build/` is ignored by Git.

## Repository map

```text
src/                         freestanding TinyGPT source
assets/                      Freedoom IWAD, license, and credits
tools/make_image.py          GPT/FAT image builder
tools/make_update_site.py    stable/nightly update-site builder
tools/build_uefi_firmware.sh optional developer firmware builder
tinygpt                      host-side image updater
tests/                       source and updater tests
.github/workflows/           CI, nightly, release, firmware, and Pages automation
```

See [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md), and [CHANGELOG.md](CHANGELOG.md) for project policies and release history.

## License

Copyright © 2026 firesafetylite.

TinyGPT is licensed as an integrated work under [GPL-2.0-only](LICENSE) because it incorporates PureDOOM. PureDOOM is GPL-2.0, and Freedoom 0.13.0 assets are BSD-3-Clause. The optional EDK2-based developer firmware also incorporates components under BSD-2-Clause-Patent, BSD-2-Clause, and Apache-2.0; those firmware components are not included in `TinyGPT.img`.

Original attribution and license files remain under `third_party/PureDOOM/`, `assets/`, and `firmware/licenses/`. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for details.
