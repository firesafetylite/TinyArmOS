"""Production native font rendering and navigation-key repeat regressions."""
from pathlib import Path
import hashlib
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeConsoleTests(unittest.TestCase):
    def compile_run(self, source):
        compiler = shutil.which("cc")
        if not compiler:
            self.skipTest("Requires a host C compiler")
        with tempfile.TemporaryDirectory(prefix="tinygpt-native-console-") as directory:
            path = Path(directory)
            (path / "test.c").write_text(source)
            result = subprocess.run([compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                                     "-Wno-unused-function", "-I", str(ROOT / "firmware/native"),
                                     str(path / "test.c"), "-o", str(path / "test")], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            subprocess.run([str(path / "test")], check=True, timeout=10)

    def test_old_font_data_is_unchanged(self):
        header = (ROOT / "third_party/console-font/font8x19.h").read_text()
        rows = re.findall(r"\{ ((?:0x[0-9a-f]{2}(?:, )?){19}) \}", header)
        self.assertEqual(len(rows), 95)
        data = bytes(int(x, 16) for row in rows for x in re.findall(r"0x([0-9a-f]{2})", row))
        self.assertEqual(hashlib.sha256(data).hexdigest(), "69c5ee38b8ac1a38297533e24389540ab05300aedfce5dca46bdba3004c2524c")

    def test_font_orientation_height_colors_and_scroll_bounds(self):
        graphics = (ROOT / "firmware/native/graphics.inc").read_text().split("static EFI_GRAPHICS_OUTPUT_MODE_INFORMATION", 1)[0]
        self.compile_run(r'''
#include <assert.h>
#include <stdint.h>
typedef uint64_t UINTN;
typedef uint32_t UINT32;
typedef int32_t INT32;
typedef uint16_t CHAR16;
static struct { UINT32 *framebuffer; UINT32 width,height; } api, *native_api=&api;
static struct { INT32 Attribute,CursorColumn,CursorRow; } native_mode;
static UINT32 pixels[640*480+2];
''' + graphics + r'''
int main(void) {
    pixels[0]=pixels[640*480+1]=0xdeadbeef;
    api.framebuffer=pixels+1;api.width=640;api.height=480;native_mode.Attribute=0x1c;
    native_graphics_clear();
    native_console_character('F');
    static const unsigned char expected[19]={0,0,0,0xfe,0x66,0x62,0x60,0x64,0x7c,0x64,0x60,0x60,0x60,0x60,0xf0,0,0,0,0};
    assert(NATIVE_FONT_HEIGHT==19 && NATIVE_FONT_WIDTH==8 && native_text_top()==2);
    for (unsigned y=0;y<19;y++) for (unsigned x=0;x<8;x++)
        assert(api.framebuffer[(native_text_top()+y)*640+x]==((expected[y]&(0x80U>>x)) ? 0xff5555U : 0x0000aaU));
    assert(native_mode.CursorColumn==1);
    native_console_character(0x20ac); /* Unknown code points are safely substituted. */
    for (unsigned row=0;row<25;row++) for (unsigned pixel=0;pixel<640*19;pixel++)
        api.framebuffer[native_text_top()*640+row*640*19+pixel]=row+1;
    native_mode.Attribute=0x4a;
    native_graphics_scroll();
    for (unsigned row=0;row<25;row++) for (unsigned pixel=0;pixel<640*19;pixel++)
        assert(api.framebuffer[native_text_top()*640+row*640*19+pixel]==(row<24 ? row+2 : 0xaa0000U));
    assert(api.framebuffer[0]==0x0000aa && api.framebuffer[640*480-1]==0x0000aa);
    native_mode.CursorColumn=79;native_mode.CursorRow=24;native_console_character('Z');
    assert(native_mode.CursorColumn==0 && native_mode.CursorRow==24);
    assert(pixels[0]==0xdeadbeef && pixels[640*480+1]==0xdeadbeef);
    return 0;
}
''')

    def test_arrow_hold_repeat_release_and_timing(self):
        self.compile_run(r'''
#include <assert.h>
#include "key_repeat.h"
int main(void) {
    NativeKeyRepeat state={0};uint16_t scan=0,character=99;
    native_repeat_event(&state,1,1,0,1000);
    assert(!native_repeat_poll(&state,399,1000,&scan,&character));
    assert(native_repeat_poll(&state,400,1000,&scan,&character) && scan==1 && character==0);
    assert(!native_repeat_poll(&state,449,1000,&scan,&character));
    assert(native_repeat_poll(&state,450,1000,&scan,&character));
    assert(native_repeat_poll(&state,5000,1000,&scan,&character));
    assert(!native_repeat_poll(&state,5000,1000,&scan,&character)); /* No catch-up flood. */
    native_repeat_event(&state,1,0,5000,1000);
    assert(!native_repeat_poll(&state,9000,1000,&scan,&character));
    native_repeat_event(&state,1,1,10000,1000);
    native_repeat_event(&state,2,1,10100,1000);
    native_repeat_event(&state,1,0,10200,1000); /* Release another key, keep Down held. */
    assert(native_repeat_poll(&state,10500,1000,&scan,&character) && scan==2);
    native_repeat_event(&state,2,2,10540,1000); /* Host repeat must not double fire. */
    assert(!native_repeat_poll(&state,10550,1000,&scan,&character));
    assert(native_repeat_poll(&state,10590,1000,&scan,&character));
    native_repeat_event(&state,2,0,10600,1000);
    for (unsigned key=5;key<256;key++) native_repeat_event(&state,key,1,11000,1000);
    assert(!native_repeat_poll(&state,20000,1000,&scan,&character));
    for (unsigned arrow=1;arrow<=4;arrow++) {
        native_repeat_event(&state,arrow,1,21000,1000);
        assert(native_repeat_poll(&state,21400,1000,&scan,&character) && scan==arrow);
        native_repeat_event(&state,arrow,0,21401,1000);
        assert(!native_repeat_poll(&state,22000,1000,&scan,&character));
    }
    native_repeat_event(&state,1,1,UINT64_MAX-199,1000);
    assert(!native_repeat_poll(&state,199,1000,&scan,&character));
    assert(native_repeat_poll(&state,200,1000,&scan,&character));
    native_repeat_event(&state,1,0,201,1000);
    assert(!native_repeat_poll(&state,300,1000,&scan,&character));
    return 0;
}
''')


if __name__ == "__main__":
    unittest.main()
