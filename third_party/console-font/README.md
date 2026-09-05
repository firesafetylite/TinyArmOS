# Original console font

These are the unmodified printable ASCII (U+0020–U+007E) 8×19 glyph bitmaps
from the former UEFI console font. Only font data is reused: TinyGPT still owns
its reset, recovery, console renderer and native OS boot. No EDK II firmware
code, service or executable is added to the boot chain.

Source: `MdeModulePkg/Universal/Console/GraphicsConsoleDxe/LaffStd.c` at
https://github.com/tianocore/edk2/blob/d46aa46c8361194521391aa581593e556c707c6e/MdeModulePkg/Universal/Console/GraphicsConsoleDxe/LaffStd.c
(the revision of the previous firmware).

Each glyph contains 19 top-to-bottom rows, with the leftmost pixel in bit 7.
The native renderer uses the original pixel rows without vertical stretching.

License: [BSD-2-Clause-Patent](LICENSE.txt).

Upstream source SHA-256: `2a93235bda38792c96bb501e3dd0313cdee2f19696cb40254f1eb234ddf72d06`.
Printable glyph bytes SHA-256: `69c5ee38b8ac1a38297533e24389540ab05300aedfce5dca46bdba3004c2524c`.
