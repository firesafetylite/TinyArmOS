# Contributing to TinyArmOS

Thanks for helping improve TinyArmOS. Keep changes focused, explain their user-visible effect, and avoid committing generated files from `build/`.

## Before opening a change

1. Search existing issues and open one before undertaking a large design change.
2. Build with the supported toolchain:

   ```bash
   python3 -m venv /tmp/tinyarmos-venv
   /tmp/tinyarmos-venv/bin/pip install ziglang==0.14.1
   PATH="/tmp/tinyarmos-venv/bin:$PATH" make
   ```

3. Confirm that `build/BOOTAA64.EFI`, `build/TinyArmOS-UTM.img`, and `build/TinyArmOS.utm/` were produced. When possible, boot the bundle in UTM and describe what you exercised.
4. Run `make clean` after local testing; build outputs are deliberately ignored.

## Pull requests

- Use a clear title and describe motivation, implementation, and validation.
- Keep unrelated refactors separate.
- Update documentation and `CHANGELOG.md` when behavior changes.
- Preserve the license and attribution files for PureDOOM and Freedoom.
- By contributing, you agree that your contribution is licensed under GPL-2.0-only, the project's license.

Report vulnerabilities privately as described in [SECURITY.md](SECURITY.md), not in a public issue. All participants must follow the [Code of Conduct](CODE_OF_CONDUCT.md).
