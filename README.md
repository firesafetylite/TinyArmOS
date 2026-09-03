
# (DISCLAIMER: TinyArmOS is a project fully managed by ChatGPT codex, everything in this github besides this message your seeing was created and is overseen by chatgpt, if you have any issues email me at 8minecraft.19@gmail.com)

# TinyArmOS v0.1.4

[![Main CI](https://github.com/firesafetylite/TinyArmOS/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/firesafetylite/TinyArmOS/actions/workflows/ci.yml)
[![Nightly](https://github.com/firesafetylite/TinyArmOS/actions/workflows/nightly.yml/badge.svg?branch=nightly)](https://github.com/firesafetylite/TinyArmOS/actions/workflows/nightly.yml)
[![Release](https://img.shields.io/github/v/release/firesafetylite/TinyArmOS?display_name=tag)](https://github.com/firesafetylite/TinyArmOS/releases/latest)
[![License: GPL-2.0-only](https://img.shields.io/badge/license-GPL--2.0--only-blue.svg)](LICENSE)

TinyArmOS is a lightweight, freestanding ARM64 UEFI shell OS. It includes a persistent hierarchical MiniFS2 filesystem, a pre-OS recovery environment, protected system nodes, persistent shell settings, in-OS GitHub updates, a host update fallback, and native Freedoom. It is built from scratch and is not based on Unix or Linux.

## Download and run

Open the [latest release](https://github.com/firesafetylite/TinyArmOS/releases/latest) and download its `.img` asset. New releases name it `TinyArmOS-vVERSION.img`; older releases may use `-UTM.img`. This 64 MiB GPT/FAT32 disk image is the sole maintained boot distribution. The former `.utm.zip` bundle is deprecated and is no longer generated. Release EFI and manifest files remain machine-facing update infrastructure rather than separate supported boot distributions. Verify downloads with `SHA256SUMS`.

To use the image with UTM on Apple silicon, create an emulated ARM64 **Other** virtual machine with UEFI boot and at least 256 MiB RAM, import the `.img` as a non-removable VirtIO drive, and use Shared Network with a `virtio-net-pci` adapter if networking is needed. Then start the VM; ARM64 UEFI discovers `EFI/BOOT/BOOTAA64.EFI` automatically.

## Release channels

- **Nightly beta:** active development lives on the [`nightly`](https://github.com/firesafetylite/TinyArmOS/tree/nightly) branch. Every push overwrites the same rolling [nightly prerelease](https://github.com/firesafetylite/TinyArmOS/releases/tag/nightly) and `TinyArmOS-nightly.img`; it does not create another release. Nightly builds identify themselves as `TinyArmOS nightly` instead of displaying a version number.
- **Main:** `main` is the stable line. Nightly changes are promoted to `main` only with explicit project-owner approval for a major release; version tags on `main` publish stable releases.

The update commands default to `main`, so users must deliberately opt into the nightly beta channel.

The GPT image contains two isolated partitions:

```text
1  TINYRECOV  FAT16 EFI System Partition
   EFI/BOOT/BOOTAA64.EFI   protected pre-OS environment
   BOOTORD.CFG             saved default boot partition
2  TINYARMOS  FAT32 system/data partition
   TINYOS.NEW              one-shot first-boot installation marker
   DOOMU.WAD               Freedoom Phase 1
   TINYFS0.BIN/TINYFS1.BIN MiniFS2 snapshots after first boot
```

Normal TinyArmOS filesystem commands operate only on `TINYARMOS`; they do not expose `TINYRECOV`.

If UEFI opens its own shell, enter:

```text
fs0:\EFI\BOOT\BOOTAA64.EFI
```

## Pre-OS recovery environment

Verified firmware startup checks:

1. ARM64 UEFI firmware and timer
2. The TinyArmOS boot volume
3. The newest valid MiniFS2 system snapshot
4. Filesystem structure and every active file checksum
5. Whether the TinyArmOS operating system is bootable

This environment runs before the normal TinyArmOS shell. Press **Enter** during the two-second startup prompt to interrupt boot and open its partition menu. Use Up/Down and Enter to choose, **S** to save the selected default, or **R** to enter recovery immediately. In recovery, `order 1` selects TinyArmOS System and `order 2` selects Pre-OS Recovery as the persistent default. It also opens automatically when TinyArmOS is missing, its system snapshot cannot be mounted, or integrity verification fails. Startup never silently repairs a damaged installation.

The former recovery methods and management commands now exist only in this pre-OS environment; there is no `bootmgr` command inside TinyArmOS. The environment remains available after an OS wipe and always provides 256 lines of scrollback with Up/Down line scrolling, PageUp/PageDown paging, Home for the oldest output, and End or Esc to return live.

```text
partitions                     list bootable partitions
order N                        set partition 1 or 2 as default
scan                           verify MiniFS2 and both snapshots
repair                         repair or reconstruct system files
rollback                       load the previous valid snapshot
pwd/ls/cd                      navigate files before boot
cat                             inspect a file
stat/tree                      inspect metadata and directory trees
reset                          reset MiniFS2 after confirmation
scroll / scroll clear          inspect or clear 256-line scrollback
boot                           verify and start TinyArmOS
reboot/shutdown                restart or power off
```

MiniFS2 alternates between two checksummed snapshots. If the active installation is unusable, use `rollback`, `repair`, or `reset`, then `boot`.

## MiniFS2

MiniFS2 provides:

- Persistent storage across reboots
- Absolute, relative, `~`, `cd`, and `cd -` navigation
- Friendly `go`, `open`, `home`, `root`, `up`, and `back` navigation
- 256-line shell scrollback with arrow-key and PageUp/PageDown navigation
- Nested directories and directory-aware `cp`/`mv`
- 96 fixed nodes with up to 8191 data bytes per file
- Protected `/system`, `/apps`, and `/lost+found` trees
- Per-node FNV-1a integrity checksums
- Two alternating whole-filesystem snapshots
- Automatic boot-time verification with explicit pre-OS repair

Filesystem commands:

```text
pwd
ls [PATH]
tree [PATH]
sysfiles
go [home|root|system|apps|tmp|PATH]
home / root / up / back
open [PATH]
apps
cd [PATH|-]
cat PATH
write PATH TEXT
append PATH TEXT
touch PATH
mkdir PATH
rm PATH
rm -rf PATH
rmdir PATH
cp SOURCE DEST
mv SOURCE DEST
stat PATH
df
sync
fsck
fault PATH
```

`rm -rf` recursively removes files and directories and refuses an in-use working-directory tree. Exact `rm -rf /` is protected: first run `protect unlock` and type `UNLOCK`. It then empties the entire `TINYARMOS` system partition—including snapshots, user files, settings, Freedoom, and every other entry—and powers off.

The separate `TINYRECOV` partition is never part of that wipe, so its pre-OS environment and saved boot order remain intact. The next boot detects the empty system partition and enters recovery automatically; `repair` or `reset` can reconstruct a bootable MiniFS2 installation. This is logical deletion rather than secure media erasure; VM snapshots, host backups, or forensic recovery may still restore data.

Example:

```text
mkdir /home/projects
write /home/projects/hello.txt hello-arm64
cat /home/projects/hello.txt
cp /home/projects/hello.txt /tmp
sync
```

## Critical system files and apps

Run `sysfiles` to inspect both protected trees, or use `go system` and `go apps` for simpler navigation:

```text
/system/boot       loader and pre-OS handoff records
/system/kernel     core, ABI, and memory records
/system/firmware   UEFI interfaces and protocols
/system/config     boot, shell, and protection policy
/system/drivers    GOP, input, network, storage, and timer records
/system/runtime    MiniFS2, mounts, and snapshots
/system/security   integrity policy and protected paths
/apps/doom         Freedoom app, controls, data link, and license
/apps/shell        native shell app metadata
```

Use `open /system/manifest.txt`, `open /apps/doom/app.info`, or `open /apps/doom/controls.txt`. These files are readable while locked.

Protected trees are locked at every boot. To modify one intentionally:

```text
protect status
protect unlock
# Type the exact confirmation: UNLOCK
protect lock
```

The unlock applies only to the current boot. Creation, writing, copying, moving, renaming, `rm`, and ordinary `rm -rf PATH` operations enforce protection through ancestor directories, including `/apps`. Exact `rm -rf /` also requires this temporary unlock before it can wipe the system partition.

## Freedoom

Run:

```text
doom
```

Controls:

```text
WASD        move and strafe
Arrow keys  turn and move
F           fire
E           use/open
Enter       select
Esc         menu
Q or F12    return to TinyArmOS
```

Freedoom runs natively through UEFI Graphics Output Protocol and Simple Text Input; no Linux layer is involved. Sound and music are disabled in this lightweight port. Doom can be launched once per boot.

The engine is [PureDOOM](https://github.com/Daivuk/PureDOOM). The legally redistributable Freedoom 0.13.0 Phase 1 IWAD is kept under `assets/` with its license and full credits.

## Settings

Run `settings` to open the dedicated full-screen settings UI. It configures:

- Default shell text color
- Accent color used for prompts, directories, and protected-node labels
- Whether the current path appears in the prompt
- Whether the shell starts in `/` or `/home`
- Whether 256-line scrollback is enabled

Every change is validated and saved automatically to `/home/.tinyarmrc`, while the UI remains open for additional changes. Choose **0** only when you are ready to return to the shell. Settings survive reboots and OS updates. Black foreground text is excluded because the console background is black. Restore Defaults is available inside the menu.

The selected accent color also provides semantic filesystem highlighting: directory entries and tree branches stand out from regular files, and `[system]` labels identify protected nodes.

## Shell scrollback

The shell keeps the latest 256 lines of commands and output:

```text
Up/Down     scroll one line at a time
PageUp      scroll one page toward older output
PageDown    scroll one page toward newer output
Home        jump to the oldest available page
End or Esc  return to the live prompt
scroll       show scrollback status and controls
scroll clear erase retained scrollback
```

Typing while viewing old output automatically returns to the live prompt.

## Other commands

```text
help
clear
scroll [clear]
echo TEXT
info
uptime
count
settings
protect [status|unlock|lock]
update [check] [main|nightly]
reboot
shutdown
```

## Updates

From the TinyArmOS shell, choose the stable or beta channel:

```text
update check              check the main channel (default)
update main               install from the main channel
update check nightly      check the rolling nightly beta
update nightly            install from the rolling nightly beta
```

The updater compares an internal compatibility version and the EFI digest, so a changed nightly build is detected even though the OS displays only `nightly`. It refuses channel changes that would downgrade the installed compatibility version.

The in-OS updater requires VM firmware that exposes UEFI HTTP/TLS. That firmware is platform hardware and is not contained in the maintained disk image; stock VM firmware may therefore report that the service is unavailable. When available, the updater uses IPv4 DHCP and automatically continues through the UTM Shared Network fallback, validates the redirect-free GitHub Pages manifest and SHA-256 checksum, confirms the download is an ARM64 EFI application, rejects rollback versions, and keeps `EFI/BOOT/BOOTAA64.BAK` as a recovery copy. Reboot after installation.

For the supported image-only fallback, shut down the VM, download the `tinyarmos` host command from the release, and run:

```bash
chmod +x tinyarmos
./tinyarmos update --channel main /path/to/TinyArmOS.img
./tinyarmos update --channel nightly /path/to/TinyArmOS.img  # opt-in beta
```

The host command uses the host's maintained HTTPS stack and updates only `EFI/BOOT/BOOTAA64.EFI` on the recovery partition, preserving the separate system partition, MiniFS snapshots, user files, and Freedoom. Keep the image detached from running virtual machines while updating it. Explicit updates of historical `.utm` bundles remain supported for migration, but no new bundle is produced.

## Build from source

Prerequisites are Bash, Make, Python 3.9 or newer, and Zig 0.14.1. The following keeps the pinned Zig toolchain outside the repository:

```bash
python3 -m venv /tmp/tinyarmos-venv
/tmp/tinyarmos-venv/bin/pip install ziglang==0.14.1
PATH="/tmp/tinyarmos-venv/bin:$PATH" make
```

Alternatively, install Zig 0.14.1 directly and run `make`. `build.sh` compiles the ARM64 EFI executable, builds an isolated recovery ESP plus a FAT32 system partition in one 64 MiB GPT disk, embeds Freedoom, and generates `build/BOOTAA64.EFI` plus the canonical `build/TinyArmOS.img`. The image builder and host update command use only Python's standard library.

CI checks that the EFI output is an AArch64 PE32+ application, validates the GPT/FAT32 image, and confirms that the image contains the exact built EFI executable and Freedoom data. These structural checks do not replace a firmware boot test; when changing boot or runtime behavior, also attach the generated image to an ARM64 UEFI VM and test it when practical.

The optional `tools/build_uefi_firmware.sh` workflow remains available to developers testing the in-OS HTTP/TLS updater. Its custom pflash files are not part of the maintained image distribution, and the project no longer generates a UTM configuration bundle.

Use `make clean` to remove generated output. Everything under `build/` is intentionally ignored by Git.

## Licensing

Copyright © 2026 firesafetylite. TinyArmOS as an integrated work is licensed under the [GNU General Public License version 2 only](LICENSE) (`GPL-2.0-only`) because it incorporates PureDOOM.

PureDOOM is distributed under GPL-2.0 and Freedoom 0.13.0 assets are distributed under the BSD 3-Clause license. The optional developer firmware builder uses EDK2 under BSD-2-Clause-Patent, libfdt under BSD-2-Clause, and OpenSSL under Apache-2.0; those firmware components are not included in `TinyArmOS.img`. Original license and attribution files are preserved under `third_party/PureDOOM/`, `assets/`, and `firmware/licenses/`. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for details.
