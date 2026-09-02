# Changelog

Notable changes to TinyArmOS are documented here. This project follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

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

[0.1]: https://github.com/firesafetylite/TinyArmOS/releases/tag/v0.1
