#!/usr/bin/env bash
# Build independent ARM64 firmware. Never replaces an existing VM or its EDK II image.
set -euo pipefail
cd "$(dirname "$0")/.."
out="${1:-build/bios}"
mkdir -p "$out"
if command -v zig >/dev/null 2>&1; then
  ZIG=(zig)
elif python3 -c 'import ziglang' >/dev/null 2>&1; then
  ZIG=(python3 -m ziglang)
else
  echo 'Zig 0.14.1 is required; use the same environment as build.sh.' >&2
  exit 1
fi
"${ZIG[@]}" cc -target aarch64-freestanding-none -std=c11 -Os \
  -ffreestanding -fno-builtin -fno-stack-protector -mgeneral-regs-only -mstrict-align \
  -nostdlib -static -Wall -Wextra -Werror \
  -Wl,-T,firmware/bios/link.ld -Wl,--build-id=none \
  firmware/bios/start.S firmware/bios/main.c firmware/bios/virtio.c \
  firmware/bios/storage.c firmware/bios/elf.c -o "$out/TinyGPT-BIOS.elf"
python3 tools/make_bios_rom.py "$out/TinyGPT-BIOS.elf" "$out/TinyGPT-BIOS.bin"
printf '\nExperimental standalone firmware (not a drop-in TinyGPT EFI replacement):\n'
ls -lh "$out/TinyGPT-BIOS.elf" "$out/TinyGPT-BIOS.bin"
