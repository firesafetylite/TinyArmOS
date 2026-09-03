
# (DISCLAIMER: TinyArmOS is a project fully managed by ChatGPT codex, everything in this github besides this message your seeing was created and is overseen by chatgpt, if you have any issues email me at 8minecraft.19@gmail.com)

# TinyArmOS v0.1.2

[![CI](https://github.com/firesafetylite/TinyArmOS/actions/workflows/ci.yml/badge.svg)](https://github.com/firesafetylite/TinyArmOS/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/firesafetylite/TinyArmOS?display_name=tag)](https://github.com/firesafetylite/TinyArmOS/releases/latest)
[![License: GPL-2.0-only](https://img.shields.io/badge/license-GPL--2.0--only-blue.svg)](LICENSE)

TinyArmOS is a lightweight, freestanding ARM64 UEFI shell OS. It includes a persistent hierarchical MiniFS2 filesystem, verified startup, an integrated Recovery Agent, protected system nodes, persistent shell settings, in-OS GitHub updates, a host update fallback, and native Freedoom. It is built from scratch and is not based on Unix or Linux.

## Download and run in UTM

For a ready-to-run build, open the [latest release](https://github.com/firesafetylite/TinyArmOS/releases/latest), download `TinyArmOS-v0.1.2-UTM.utm.zip`, unzip it, then double-click the `.utm` bundle and press Run. The bundle is configured for ARM64 `virt`, custom UEFI firmware, 256 MiB RAM, VirtIO networking and RNG, and a graphical console. UTM on Apple silicon is the primary run environment. The archive also contains the `tinyarmos` host command as an update fallback.

The release also provides `TinyArmOS-v0.1.2-UTM.img`, a standalone 64 MiB GPT disk image for compatible ARM64 UEFI virtual machines, and `TinyArmOS-v0.1.2-BOOTAA64.EFI` for custom EFI setups. Verify downloads with the release's `SHA256SUMS` file.

The disk image's FAT32 EFI System Partition contains:

```text
EFI/BOOT/BOOTAA64.EFI
DOOMU.WAD                  Freedoom Phase 1
```

If UEFI opens its own shell, enter:

```text
fs0:\EFI\BOOT\BOOTAA64.EFI
```

## Boot and recovery

Verified startup checks:

1. ARM64 firmware and timer
2. Writable UEFI storage
3. The newest valid MiniFS2 snapshot
4. Filesystem metadata and per-node checksums
5. The interactive shell

Press **R** during the two-second boot prompt, or run `recovery`. The Recovery Agent supports scanning, repair, rollback, formatting, and read-only filesystem navigation. The redundant `restore` action is folded into `repair`; protection controls remain only in the normal shell, where filesystem writes are available.

```text
scan                           verify MiniFS2 and both snapshots
repair                         repair metadata and restore protected files
rollback                       load the previous valid snapshot
pwd/ls/cd                      navigate files
cat                             inspect a file
stat/tree                      inspect metadata and directory trees
reset                          format MiniFS2 after confirmation
continue                       return to the shell
reboot/shutdown                restart or power off
```

MiniFS2 alternates between two checksummed snapshots. If the newest snapshot is incomplete or corrupt, boot selects the other valid copy.

## MiniFS2

MiniFS2 provides:

- Persistent storage across reboots
- Absolute, relative, `~`, `cd`, and `cd -` navigation
- Friendly `go`, `open`, `home`, `root`, `up`, and `back` navigation
- 256-line shell scrollback with arrow-key and PageUp/PageDown navigation
- Nested directories and directory-aware `cp`/`mv`
- 96 fixed nodes with up to 8191 data bytes per file
- Protected `/system`, `/apps`, `/recovery`, and `/lost+found` trees
- Per-node FNV-1a integrity checksums
- Two alternating whole-filesystem snapshots
- Automatic boot-time verification and repair

Filesystem commands:

```text
pwd
ls [PATH]
tree [PATH]
sysfiles
go [home|root|system|apps|recovery|tmp|PATH]
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

`rm -rf` recursively removes files and directories and refuses an in-use working-directory tree. Root removal is supported: `rm -rf /` logically removes every MiniFS2 file and directory beneath `/`, including user files and settings. Because root contains protected nodes, it requires `protect unlock` followed by the exact `ERASE ROOT` confirmation. The EFI bootloader and Freedoom data on the separate FAT32 volume are not removed. This is not secure erasure: the alternate snapshot remains available to `rollback`, and protected system files are reconstructed by repair or at the next boot.

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
/system/boot       loader and startup records
/system/kernel     core, ABI, and memory records
/system/firmware   UEFI interfaces and protocols
/system/config     boot, shell, and protection policy
/system/drivers    GOP, input, network, storage, and timer records
/system/runtime    MiniFS2, mounts, and snapshots
/system/security   integrity policy and protected paths
/apps/doom         Freedoom app, controls, data link, and license
/apps/shell        native shell app metadata
/apps/recovery     Recovery Agent app metadata
/recovery          repair policy and snapshot information
```

Use `open /system/manifest.txt`, `open /apps/doom/app.info`, or `open /apps/doom/controls.txt`. These files are readable while locked.

Protected trees are locked at every boot. To modify one intentionally:

```text
protect status
protect unlock
# Type the exact confirmation: UNLOCK
protect lock
```

The unlock applies only to the current boot. Creation, writing, copying, moving, renaming, `rm`, and `rm -rf` all enforce protection through ancestor directories, including `/apps`.

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
update [check]
recovery
reboot
shutdown
```

## Updates

From the TinyArmOS shell, run:

```text
update check   check the latest GitHub Release
update         download, verify, and install it
```

The in-OS updater uses the bundled firmware's UEFI HTTP/TLS service with IPv4 DHCP and automatically continues through the UTM Shared Network fallback when DHCP times out. It reads the redirect-free GitHub Pages update channel, validates a strict manifest and SHA-256 checksum, confirms the download is an ARM64 EFI application, rejects rollback versions, and keeps `EFI/BOOT/BOOTAA64.BAK` as a recovery copy. Reboot after installation.

If the VM cannot reach the update channel, shut it down and use the included host fallback:

```bash
./tinyarmos update TinyArmOS-v0.1.2.utm
```

The host command uses the host's maintained HTTPS stack and updates only `EFI/BOOT/BOOTAA64.EFI`, preserving MiniFS snapshots, user files, and Freedoom. Keep the VM stopped while using it.

## Build from source

Prerequisites are Bash, Make, Python 3.9 or newer, and Zig 0.14.1. The following keeps the pinned Zig toolchain outside the repository:

```bash
python3 -m venv /tmp/tinyarmos-venv
/tmp/tinyarmos-venv/bin/pip install ziglang==0.14.1
PATH="/tmp/tinyarmos-venv/bin:$PATH" make
```

Alternatively, install Zig 0.14.1 directly and run `make`. `build.sh` compiles the ARM64 EFI executable, embeds Freedoom in a 64 MiB GPT/FAT32 disk, and generates `build/BOOTAA64.EFI`, `build/TinyArmOS-UTM.img`, and `build/TinyArmOS.utm`. The image builder, bundle builder, and host update command use only Python's standard library.

CI checks that the EFI output is an AArch64 PE32+ application, validates the GPT/FAT32 signatures and UTM configuration, and confirms that the standalone and bundled disk images match. These structural checks do not replace a firmware boot test; when changing boot or runtime behavior, also test the generated bundle in UTM when practical.

Use `make clean` to remove generated output. Everything under `build/` is intentionally ignored by Git.

## Licensing

Copyright © 2026 firesafetylite. TinyArmOS as an integrated work is licensed under the [GNU General Public License version 2 only](LICENSE) (`GPL-2.0-only`) because it incorporates PureDOOM.

PureDOOM is distributed under GPL-2.0, Freedoom 0.13.0 assets are distributed under the BSD 3-Clause license, EDK2 firmware is BSD-2-Clause-Patent, linked libfdt code is BSD-2-Clause, and linked OpenSSL code is Apache-2.0. Original license and attribution files are preserved under `third_party/PureDOOM/`, `assets/`, and `firmware/licenses/`. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for details.
