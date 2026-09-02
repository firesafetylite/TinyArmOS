#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
output="${1:-$repo_root/build/TinyArmOS-QEMU_EFI.fd}"
case "$output" in
  /*) ;;
  *) output="$(pwd)/$output" ;;
esac
work="${TINYARMOS_EDK2_WORK:-${TMPDIR:-/tmp}/tinyarmos-edk2-build}"
edk2_ref="edk2-stable202508"
edk2_commit="d46aa46c8361194521391aa581593e556c707c6e"

for command in git make python3 aarch64-linux-gnu-gcc; do
  command -v "$command" >/dev/null || {
    echo "$command is required to build the TinyArmOS UEFI firmware." >&2
    exit 1
  }
done

rm -rf "$work"
mkdir -p "$work"
git clone --quiet --depth 1 --branch "$edk2_ref" --recurse-submodules --shallow-submodules \
  https://github.com/tianocore/edk2.git "$work/edk2"
cd "$work/edk2"
test "$(git rev-parse HEAD)" = "$edk2_commit"

python3 "$repo_root/tools/make_uefi_ca_bundle.py" \
  "$work/TinyArmOS-ca-certs.esl" \
  OvmfPkg/Library/TlsAuthConfigLib/TinyArmCaCerts.inc \
  "$repo_root/firmware/certs/usertrust-rsa-certification-authority.pem" \
  "$repo_root/firmware/certs/usertrust-ecc-certification-authority.pem" \
  "$repo_root/firmware/certs/isrg-root-x1.pem"

python3 - ArmVirtPkg/ArmVirtQemu.dsc <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()
old = "[LibraryClasses.common.UEFI_DRIVER]\n  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf\n"
new = old + "  RngLib|MdePkg/Library/DxeRngLib/DxeRngLib.inf\n"
if old not in text:
    raise SystemExit("EDK2 UEFI driver library section did not match the pinned source")
path.write_text(text.replace(old, new, 1))
PY

python3 - OvmfPkg/Library/TlsAuthConfigLib/TlsAuthConfigLib.c <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()
text = text.replace(
    "#include <Library/BaseLib.h>\n#include <Library/DebugLib.h>",
    "#include <Library/BaseLib.h>\n#include <Library/BaseMemoryLib.h>\n#include <Library/DebugLib.h>",
    1,
)
text = text.replace(
    "#include <Library/UefiRuntimeServicesTableLib.h>\n",
    "#include <Library/UefiRuntimeServicesTableLib.h>\n\n#include \"TinyArmCaCerts.inc\"\n",
    1,
)
text = text.replace(
    "  VOID                  *HttpsCaCerts;\n",
    "  VOID                  *HttpsCaCerts;\n  BOOLEAN               UseBuiltInCaCerts;\n",
    1,
)
old = '''  Status = QemuFwCfgFindFile (
             "etc/edk2/https/cacerts",
             &HttpsCaCertsItem,
             &HttpsCaCertsSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a:%a: not touching CA cert list\\n",
      gEfiCallerBaseName,
      __func__
      ));
    return;
  }
'''
new = '''  UseBuiltInCaCerts = FALSE;
  Status = QemuFwCfgFindFile (
             "etc/edk2/https/cacerts",
             &HttpsCaCertsItem,
             &HttpsCaCertsSize
             );
  if (EFI_ERROR (Status)) {
    HttpsCaCertsSize  = sizeof (mTinyArmCaCerts);
    UseBuiltInCaCerts = TRUE;
    DEBUG ((
      DEBUG_VERBOSE,
      "%a:%a: using TinyArmOS built-in CA cert list\\n",
      gEfiCallerBaseName,
      __func__
      ));
  }
'''
if old not in text:
    raise SystemExit("EDK2 CA lookup block did not match the pinned source")
text = text.replace(old, new, 1)
text = text.replace(
    "  QemuFwCfgSelectItem (HttpsCaCertsItem);\n  QemuFwCfgReadBytes (HttpsCaCertsSize, HttpsCaCerts);\n",
    "  if (UseBuiltInCaCerts) {\n"
    "    CopyMem (HttpsCaCerts, mTinyArmCaCerts, HttpsCaCertsSize);\n"
    "  } else {\n"
    "    QemuFwCfgSelectItem (HttpsCaCertsItem);\n"
    "    QemuFwCfgReadBytes (HttpsCaCertsSize, HttpsCaCerts);\n"
    "  }\n",
    1,
)
path.write_text(text)
PY

touch -d "@$(git show -s --format=%ct HEAD)" OvmfPkg/Library/TlsAuthConfigLib/TinyArmCaCerts.inc
make -C BaseTools -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
export PYTHON_COMMAND=python3
set +u
# shellcheck disable=SC1091
source edksetup.sh BaseTools
set -u
export GCC5_AARCH64_PREFIX="${GCC5_AARCH64_PREFIX:-aarch64-linux-gnu-}"
export SOURCE_DATE_EPOCH="$(git show -s --format=%ct HEAD)"
build -a AARCH64 -t GCC5 -b RELEASE -p ArmVirtPkg/ArmVirtQemu.dsc \
  -D FIRMWARE_VER=TinyArmOS-EDK2-202508 \
  -D NETWORK_HTTP_ENABLE=TRUE \
  -D NETWORK_HTTP_BOOT_ENABLE=FALSE \
  -D NETWORK_TLS_ENABLE=TRUE \
  -D NETWORK_ALLOW_HTTP_CONNECTIONS=FALSE \
  -D NETWORK_IP6_ENABLE=FALSE \
  -D NETWORK_PXE_BOOT_ENABLE=FALSE

firmware=Build/ArmVirtQemu-AARCH64/RELEASE_GCC5/FV/QEMU_EFI.fd
variables=Build/ArmVirtQemu-AARCH64/RELEASE_GCC5/FV/QEMU_VARS.fd
ffs=Build/ArmVirtQemu-AARCH64/RELEASE_GCC5/FV/Ffs
test -s "$firmware"
test -s "$variables"
for module in VirtioNetDxe VirtioRngDxe Dhcp4Dxe DnsDxe HttpDxe TlsDxe TlsAuthConfigDxe; do
  find "$ffs" -maxdepth 1 -type d -name "*${module}" -print -quit | grep -q . || {
    echo "Required firmware module is missing: $module" >&2
    exit 1
  }
done
mkdir -p "$(dirname "$output")"
cp "$firmware" "$output"
truncate -s 67108864 "$output"
cp "$variables" "${output%.fd}-vars.fd"
truncate -s 67108864 "${output%.fd}-vars.fd"
cp "$work/TinyArmOS-ca-certs.esl" "${output%.fd}-ca-certs.esl"
sha256sum "$output" "${output%.fd}-vars.fd" "${output%.fd}-ca-certs.esl"
