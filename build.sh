#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
# Remove outputs from the retired UTM bundle build so they cannot be mistaken
# for maintained artifacts after upgrading an existing checkout.
rm -rf build/TinyArmOS.utm
rm -f build/TinyArmOS-UTM.img

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

python3 tools/make_image.py build/BOOTAA64.EFI build/TinyArmOS.img assets/freedoom1.wad

echo
echo "Build complete:"
ls -lh build/BOOTAA64.EFI build/TinyArmOS.img
