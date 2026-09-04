# Third-Party Notices

TinyGPT includes the following third-party components. Their copyrights remain with their respective authors and contributors.

## EDK2 firmware

- Project: [TianoCore EDK2](https://github.com/tianocore/edk2)
- Version: `edk2-stable202508`, commit `d46aa46c8361194521391aa581593e556c707c6e`
- Optional developer artifact: custom ARM64 UEFI firmware built separately from `TinyGPT.img`
- License: BSD-2-Clause-Patent

The optional firmware build enables and configures EDK2 networking, HTTP/TLS, VirtIO RNG, and a pinned CA trust store for updater testing. It is not included in the maintained image distribution. The complete EDK2 license is preserved in `firmware/licenses/EDK2-LICENSE.txt`.

## OpenSSL

- Project: [OpenSSL](https://github.com/openssl/openssl)
- Commit: `aea7aaf2abb04789f5868cbabec406ea43aa84bf`, pinned by EDK2
- Optional developer artifact: cryptographic and TLS code linked into separately built custom UEFI firmware
- License: Apache License 2.0

The complete OpenSSL license is preserved in `firmware/licenses/OPENSSL-LICENSE.txt`.

## libfdt

- Project: [pylibfdt/libfdt](https://github.com/devicetree-org/pylibfdt)
- Commit: `cfff805481bdea27f900c32698171286542b8d3c`, pinned by EDK2
- Optional developer artifact: Flattened Device Tree support linked into separately built custom UEFI firmware
- License: BSD-2-Clause

The copyright notices and complete libfdt license are preserved in `firmware/licenses/LIBFDT-LICENSE.txt`.

## PureDOOM

- Project: [PureDOOM](https://github.com/Daivuk/PureDOOM)
- Included files: `third_party/PureDOOM/PureDOOM.h` and `third_party/PureDOOM/LICENSE`
- License: GNU General Public License, version 2 (GPL-2.0)
- Copyright notices: retained in the vendored source

PureDOOM incorporates the Doom engine code originally released by id Software. Its source notices are retained. The integrated TinyGPT executable is distributed under GPL-2.0-only; see the top-level [LICENSE](LICENSE).

## Freedoom

- Project: [Freedoom](https://freedoom.github.io/)
- Version: 0.13.0, Phase 1
- Included asset: `assets/freedoom1.wad`
- License: BSD 3-Clause
- Copyright: © 2001–2024 Freedoom contributors

The complete license, documentation, contributor credits, and music credits supplied with the IWAD are preserved in:

- `assets/FREEDOOM-COPYING.txt`
- `assets/FREEDOOM-README.html`
- `assets/FREEDOOM-CREDITS.txt`
- `assets/FREEDOOM-CREDITS-MUSIC.txt`

Freedoom is an independent free-content game and is not affiliated with or endorsed by id Software.
