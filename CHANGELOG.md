# Changelog

Notable changes to TinyArmOS are documented here. This project follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- A rolling `nightly` beta branch and prerelease pipeline that overwrites one unversioned nightly release on every push while leaving `main` stable until an explicitly approved major release.
- User-selectable `main` and `nightly` channels in both the in-OS updater and the `tinyarmos` host updater; nightly OS builds display `TinyArmOS nightly` rather than a version number.
- Separate, checksum-validated GitHub Pages manifests for the main and nightly EFI update channels.

### Changed

- Deprecated the `.utm.zip` distribution and made `TinyArmOS.img` the sole maintained boot image; builds and releases no longer generate or validate a UTM bundle.
- Changed host-update auto-discovery to prefer `.img` files while retaining explicit legacy `.utm` targets for migration, and compare EFI digests so same-version nightly builds remain discoverable.
- Kept custom HTTP/TLS firmware as an optional developer build instead of a release-build dependency.
- Replaced the dedicated Recovery Agent and its MiniFS trees with the TinyArmOS Boot Manager, available through the `bootmgr` command, the boot-time **R** hotkey, and automatic integrity-error routing.
- Made 256-line scrollback and all line, page, oldest, and live-output navigation controls available inside Boot Manager regardless of the normal shell setting.
- Migrated legacy `/recovery` and `/apps/recovery` trees to protected `/apps/bootmgr` metadata when restoring persisted MiniFS snapshots.

## [0.1.4] - 2026-09-02

### Changed

- Make exact `rm -rf /` an immediate OS-destruction command without an unlock or confirmation: it removes MiniFS2, both snapshots, the EFI loader and updater copies, and Freedoom data before powering off.
- Empty dedicated `TINYARMOS` EFI volumes while limiting custom/shared ESP cleanup to TinyArmOS-owned files, and report any partial firmware deletion failure without creating a recovery snapshot.

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

- A standard-library `tinyarmos update` host command that checks GitHub Releases, verifies the ARM64 EFI asset, preserves MiniFS data, and creates a disk backup.
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

[Unreleased]: https://github.com/firesafetylite/TinyArmOS/compare/v0.1.4...HEAD
[0.1.4]: https://github.com/firesafetylite/TinyArmOS/compare/v0.1.3...v0.1.4
[0.1.3]: https://github.com/firesafetylite/TinyArmOS/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/firesafetylite/TinyArmOS/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/firesafetylite/TinyArmOS/compare/v0.1...v0.1.1
[0.1]: https://github.com/firesafetylite/TinyArmOS/releases/tag/v0.1
