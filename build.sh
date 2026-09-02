#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build

if command -v zig >/dev/null 2>&1; then
  ZIG=(zig)
elif python3 -c 'import ziglang' >/dev/null 2>&1; then
  ZIG=(python3 -m ziglang)
else
  echo "Zig is required to rebuild the ARM64 EFI file." >&2
  echo "Install it with:  brew install zig" >&2
  echo "or:               python3 -m pip install ziglang" >&2
  exit 1
fi

"${ZIG[@]}" cc \
  -target aarch64-windows-msvc \
  -std=c11 -Os -ffreestanding -fno-stack-protector -fshort-wchar -nostdlib \
  -Wall -Wextra -Werror \
  -Wl,--subsystem=efi_application \
  -o build/BOOTAA64.EFI src/uefi.c

python3 tools/make_utm_image.py build/BOOTAA64.EFI build/TinyArmOS-UTM.img assets/freedoom1.wad
firmware="${TINYARMOS_UEFI_FIRMWARE:-build/TinyArmOS-QEMU_EFI.fd}"
variables="${TINYARMOS_UEFI_VARIABLES:-${firmware%.fd}-vars.fd}"
if [[ -f "$firmware" && -f "$variables" ]]; then
  python3 tools/make_utm_bundle.py build/TinyArmOS-UTM.img build/TinyArmOS.utm "$firmware" "$variables"
elif [[ -f "$firmware" || -f "$variables" ]]; then
  echo "Both UEFI code and variable images are required; refusing a partial firmware bundle." >&2
  exit 1
elif [[ "${TINYARMOS_REQUIRE_UEFI_FIRMWARE:-0}" == 1 ]]; then
  echo "The HTTP/TLS-capable UEFI firmware is required for this build." >&2
  exit 1
else
  echo "Warning: custom UEFI firmware is absent; this development bundle cannot use network updates." >&2
  python3 tools/make_utm_bundle.py build/TinyArmOS-UTM.img build/TinyArmOS.utm
fi

echo
echo "Build complete:"
ls -lh build/BOOTAA64.EFI build/TinyArmOS-UTM.img
printf 'UTM bundle: %s\n' "build/TinyArmOS.utm"
