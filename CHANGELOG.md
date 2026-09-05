# Changelog

Notable changes to TinyGPT are documented here. This project follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Changed

- Keep the RAM-resident shell running after `rm -rf /` erases the system partition instead of powering off automatically.

## [0.1.6] - 2026-09-04

### Added

- A persistent Settings option for previewing and changing the OS console background color.
- A native full-screen `textedit [PATH]` application for viewing, creating, and editing MiniFS2 ASCII text with cursor navigation, guarded saves, and unsaved-change protection.
- A full-screen interactive file picker and new-file modal when `textedit` is launched without a path.
- `/apps/editor` metadata and controls in the protected application registry.
- A read-only `partitions` command in the regular shell that lists canonical partition numbers and directs all management to pre-OS recovery.

### Changed

- Renamed the project, runtime identity, generated artifacts, host tooling, update channels, and GitHub references to TinyGPT.
- Rewrote the README around a clearer quick-start, architecture overview, command reference, update guide, and build workflow.
- Simplified root-wipe messaging in the built-in help screen, runtime output, and README.
- Replace the short-lived `edit` command with the canonical `textedit` app command.
- Eliminate Text Editor flicker by caching screen rows and repainting only content that changed instead of clearing the console per keypress.
- Add Left Arrow, Backspace, and **B** parent-directory navigation to the Text Editor picker, plus Left Arrow/Esc return controls in its new-file modal.
- Soft-wrap editor text at the screen edge; Up/Down now traverses wrapped rows and scrolls automatically, while Home/End/PageUp/PageDown editor bindings are removed.
- Redraw the interrupted-startup partition selector in place instead of appending a new output line for every selection.
- Document and validate the persistent UTM PFlash arrangement that keeps HTTP/TLS-capable firmware independent from TinyGPT disk updates.
- Seed newly created partitions for automatic first-boot TinyGPT initialization, eliminating the misleading unavailable-OS result after their required firmware reboot.
- Harden release automation by centralizing Pages deployment on `main`, disabling persisted checkout credentials, and replacing rolling-nightly assets without a delete-all availability window.

### Removed

- Redundant shell navigation and discovery shortcuts: `home`, `root`, `up`, `back`, `go`, `open`, `sysfiles`, and `apps`; use `cd`, `ls`, `tree`, and `cat` instead.
- The redundant `touch` command; `write PATH` now documents creation of an empty file when text is omitted.
- The nonessential `count`, manual `sync`, and production diagnostic `fault` commands; filesystem mutations already save snapshots automatically, while `fsck` remains available for integrity checks.
- Dead loaded-image path bookkeeping.

### Fixed

- Restored the guest and host update channels after the GitHub rename by publishing TinyGPT release assets and Pages endpoints consistently.

## [0.1.5] - 2026-09-03

### Added

- A rolling `nightly` beta branch and prerelease pipeline that overwrites one unversioned nightly release on every push while leaving `main` stable until an explicitly approved stable release.
- User-selectable `main` and `nightly` channels in both the in-OS updater and the `tinygpt` host updater; nightly OS builds display `TinyGPT nightly` rather than a version number.
- Separate, checksum-validated GitHub Pages manifests for the main and nightly EFI update channels.
- Pre-OS `partition add MIB NAME` and `partition name N NAME` commands with mirrored GPT updates, FAT formatting/labels, persistent registration, and hard protection for recovery partition 1.
- Explicit partition targets for `scan N`, `repair N`, `rollback N`, and `reset N`; `repair N` installs a fresh TinyGPT filesystem when the target is empty.
- Arbitrary registered targets in the boot menu, `order N`, `use N`, and `boot N`.

### Changed

- Split the image into an isolated `TINYRECOV` EFI partition and a separate `TINYGPT` system/data partition, with a firmware-style boot menu and persistent partition default.
- Expanded the maintained image from 64 MiB to 128 MiB without moving the original partition extents, leaving aligned unallocated space for additional partitions.
- Standardized GPT numbering so partition 1 is protected recovery, partition 2 is the initial system, and added partitions are numbered 3 and above.
- Changed protected `rm -rf /` to require `protect unlock` and empty the entire system partition without touching pre-OS recovery.
- Deprecated the `.utm.zip` distribution and made `TinyGPT.img` the sole maintained boot image; builds and releases no longer generate or validate a UTM bundle.
- Changed host-update auto-discovery to prefer `.img` files while retaining explicit legacy `.utm` targets for migration, and compare EFI digests so same-version nightly builds remain discoverable.
- Kept custom HTTP/TLS firmware as an optional developer build instead of a release-build dependency.
- Moved all recovery and management commands into a pre-OS environment that runs before the TinyGPT shell; **R**, a missing OS, or failed integrity verification enters it.
- Removed the in-OS `bootmgr` command and retired `/recovery`, `/apps/recovery`, and `/apps/bootmgr` metadata during snapshot migration.
- Kept the pre-OS entry point on its isolated partition after `rm -rf /`; an empty or invalid system partition opens recovery automatically instead of silently recreating the OS.
- Made 256-line scrollback and all line, page, oldest, and live-output navigation controls available inside the pre-OS environment regardless of the normal shell setting.

## [0.1.4] - 2026-09-02

### Changed

- Make exact `rm -rf /` an immediate OS-destruction command without an unlock or confirmation: it removes MiniFS2, both snapshots, the EFI loader and updater copies, and Freedoom data before powering off.
- Empty dedicated `TINYGPT` EFI volumes while limiting custom/shared ESP cleanup to TinyGPT-owned files, and report any partial firmware deletion failure without creating a recovery snapshot.

## [0.1.3] - 2026-09-02

### Changed

- Startup verification now runs the same `scan` integrity path as Recovery, validating MiniFS2 structure, active file checksums, and persistent snapshot payloads before the shell boots.

### Fixed

- Recognize the literal `/` target directly for `rm -rf /` before general path resolution.
- Clarify after an in-OS installation that the old shell remains loaded until reboot.

## [0.1.2] - 2026-09-02

### Added

- Guarded logical `rm -rf /` support with protected-node unlocking, exact destructive confirmation, and recoverable snapshots.

### Changed

- Simplified Recovery Agent commands by folding `restore` into `repair` and removing protection controls that had no Recovery-side write operation to govern.

## [0.1.1] - 2026-09-02

### Added

- A persistent `settings` menu for default and accent colors, prompt paths, startup directory, and scrollback.
- Complete built-in command documentation in the normal shell and Recovery Agent help screens.

### Changed

- Turned `settings` into a dedicated full-screen UI that auto-saves each change and stays open until explicitly closed.
- Applied the configured accent color to directory entries, tree branches, and protected-node labels.
- Removed redundant command aliases while retaining one canonical command for each action.

### Fixed

- `update` and `update check` now retry the UTM Shared Network fallback automatically in the same command after DHCP times out.

## [0.1] - 2026-09-02

### Added

- A standard-library `tinygpt update` host command that checks GitHub Releases, verifies the ARM64 EFI asset, preserves MiniFS data, and creates a disk backup.
- Unit coverage for version checks, EFI validation, checksum parsing, FAT32 replacement, data preservation, and UTM target resolution.
- Pinned EDK2 firmware in UTM releases with IPv4, DNS, HTTP, TLS, VirtIO networking, and VirtIO RNG.
- A redirect-free GitHub Pages channel for the latest strict update manifest and checksum-verified ARM64 EFI image.
- UTM Shared Network address and DNS fallback when firmware DHCP cannot obtain a lease.
- Firmware-backed UEFI HTTP/TLS updater with `update` and `update check` commands.
- GitHub Release manifests, bounded HTTPS redirects, SHA-256 verification, ARM64 PE validation, and bootloader backup/restore.
- Enabled VirtIO networking in generated UTM bundles.
- Up/Down arrow keys scroll shell output one line at a time alongside PageUp/PageDown paging.
- Verified ARM64 UEFI startup and interactive recovery workflow.
- Persistent MiniFS2 filesystem with alternating checksummed snapshots, protected system trees, repair, and rollback.
- Native Freedoom 0.13.0 Phase 1 powered by the integrated PureDOOM engine.
- Shell navigation, file-management commands, and 256-line scrollback.
- Source builders for a bootable GPT/FAT32 image and UTM bundle.

### Changed

- Kept the in-OS UEFI HTTP/TLS `update` command and added the host command as a fallback instead of replacing guest updates.
- Increased generated UTM bundles to 256 MiB RAM for firmware TLS workloads.
- Release builds now require and validate both custom pflash images and all updater firmware drivers.

### Security

- Enabled ECDHE and protocol-backed entropy for UEFI TLS, with a pinned three-root trust store.
- Reject malformed release versions and downgrade manifests before downloading an update.

[Unreleased]: https://github.com/firesafetylite/TinyGPT/compare/v0.1.6...HEAD
[0.1.6]: https://github.com/firesafetylite/TinyGPT/compare/v0.1.5...v0.1.6
[0.1.5]: https://github.com/firesafetylite/TinyGPT/compare/v0.1.4...v0.1.5
[0.1.4]: https://github.com/firesafetylite/TinyGPT/compare/v0.1.3...v0.1.4
[0.1.3]: https://github.com/firesafetylite/TinyGPT/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/firesafetylite/TinyGPT/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/firesafetylite/TinyGPT/compare/v0.1...v0.1.1
[0.1]: https://github.com/firesafetylite/TinyGPT/releases/tag/v0.1
