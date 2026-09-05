"""Execute production scrollback capture/redraw and theme helpers against a console model."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ScrollbackNativeTests(unittest.TestCase):
    def test_scrolling_preserves_styles_theme_and_live_output(self) -> None:
        compiler = shutil.which("cc")
        if not compiler:
            self.skipTest("Native scrollback tests require a host C compiler")
        source = (ROOT / "src/uefi.c").read_text()
        history = "static void console_write_raw(" + source.split("static void console_write_raw(", 1)[1].split("static void print(", 1)[0]
        colors = "static UINTN settings_text_attribute(" + source.split("static UINTN settings_text_attribute(", 1)[1].split("static void settings_parse_config_line(", 1)[0]
        declarations = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint16_t CHAR16;
typedef unsigned long long UINTN;
typedef int EFI_STATUS;
#define EFI_SUCCESS 0
#define SCROLLBACK_LINES 256
#define SCROLLBACK_COLUMNS 160
#define SCROLLBACK_DEFAULT 0U
#define SCROLLBACK_ACCENT 1U
#define SCROLLBACK_LITERAL 0x100U
static char gScrollback[SCROLLBACK_LINES][SCROLLBACK_COLUMNS];
static UINT16 gScrollbackStyles[SCROLLBACK_LINES][SCROLLBACK_COLUMNS];
static UINT8 gScrollbackWrapped[SCROLLBACK_LINES], gScrollbackEnabled, gConsoleColorRole;
static UINTN gScrollbackCount,gScrollbackLength,gScrollbackOffset,gConsoleColumns,gConsoleRows;
static struct { UINT8 textColor,accentColor,backgroundColor,showPromptPath,startupHome,scrollback; } gSettings;
static struct { int Mode,Attribute,CursorColumn,CursorRow; } mode;
typedef struct Console Console;
struct Console {
    EFI_STATUS (*SetAttribute)(Console *, UINTN);
    EFI_STATUS (*ClearScreen)(Console *);
    EFI_STATUS (*SetCursorPosition)(Console *, UINTN, UINTN);
    EFI_STATUS (*QueryMode)(Console *, UINTN, UINTN *, UINTN *);
    __typeof__(mode) *Mode;
};
static struct { Console *ConOut; } table, *gST=&table;
static UINT8 screen[25][80];
static char text[25][80];
static void memory_zero(void *p, UINTN bytes) { memset(p,0,(size_t)bytes); }
static void memory_copy(void *p, const void *s, UINTN bytes) { memcpy(p,s,(size_t)bytes); }
static EFI_STATUS attr(Console *c, UINTN a) { (void)c; mode.Attribute=(int)a; return 0; }
static EFI_STATUS clear(Console *c) { (void)c; memset(screen,mode.Attribute,sizeof(screen)); memset(text,' ',sizeof(text)); mode.CursorColumn=mode.CursorRow=0; return 0; }
static EFI_STATUS cursor(Console *c, UINTN x, UINTN y) { (void)c; assert(x<80 && y<25); mode.CursorColumn=(int)x;mode.CursorRow=(int)y; return 0; }
static EFI_STATUS query(Console *c, UINTN m, UINTN *x, UINTN *y) { (void)c;(void)m;*x=80;*y=25;return 0; }
static void out16(CHAR16 *value) {
    while (*value) {
        unsigned ch=*value++;
        if (ch=='\r') mode.CursorColumn=0;
        else if (ch=='\n') mode.CursorRow++;
        else {
            text[mode.CursorRow][mode.CursorColumn]=(char)ch;
            screen[mode.CursorRow][mode.CursorColumn]=(UINT8)mode.Attribute;
            if (++mode.CursorColumn==80) { mode.CursorColumn=0;mode.CursorRow++; }
        }
        if (mode.CursorRow>=25) {
            memmove(screen,screen[1],sizeof(screen)-sizeof(screen[0]));
            memmove(text,text[1],sizeof(text)-sizeof(text[0]));
            memset(screen[24],mode.Attribute,80); memset(text[24],' ',80); mode.CursorRow=24;
        }
    }
}
static void settings_use_default_color(void);
static void settings_use_accent_color(void);
'''
        assertions = r'''
static void emit(const char *value) { scrollback_capture(value);console_write_raw(value); }
int main(void) {
    Console console={attr,clear,cursor,query,&mode}; table.ConOut=&console;
    gSettings.textColor=12; gSettings.accentColor=10;gSettings.backgroundColor=4;
    gSettings.showPromptPath=1;gSettings.startupHome=1;gSettings.scrollback=1;
    settings_use_default_color();scrollback_enable();
    for (unsigned i=0;i<40;i++) {
        settings_use_accent_color();emit("A"); settings_use_default_color();emit("B\n");
    }
    __typeof__(gSettings) saved=gSettings;
    scrollback_move(-1,5);
    assert(text[0][0]=='A' && screen[0][0]==0x4a);
    assert(text[0][1]=='B' && screen[0][1]==0x4c);
    assert(screen[0][79]==0x4c); /* Clearing retains the chosen background. */
    assert(mode.Attribute==0x4c && gConsoleColorRole==SCROLLBACK_DEFAULT);
    assert(!memcmp(&saved,&gSettings,sizeof(saved)));
    gScrollbackOffset=0;scrollback_render();
    assert(mode.Attribute==0x4c);
    assert(gScrollbackStyles[0][0]==SCROLLBACK_ACCENT);
    assert(gScrollbackStyles[0][1]==SCROLLBACK_DEFAULT);
    /* Old history follows a newly selected theme instead of reverting colors. */
    gSettings.textColor=14;gSettings.accentColor=13;gSettings.backgroundColor=1;
    settings_use_default_color();scrollback_move(-1,5);
    assert(screen[0][0]==0x1d && screen[0][1]==0x1e && mode.Attribute==0x1e);
    /* Even equal-colored roles must remain distinguishable for future themes. */
    scrollback_reset();gSettings.textColor=gSettings.accentColor=7;
    settings_use_accent_color();emit("A");settings_use_default_color();emit("B");
    gSettings.accentColor=10;gSettings.textColor=12;settings_use_default_color();
    scrollback_render();assert(screen[0][0]==0x1a && screen[0][1]==0x1c);
    /* Style and text remain aligned when the 256-line buffer evicts history. */
    scrollback_reset();
    for (unsigned i=0;i<SCROLLBACK_LINES+7;i++) {
        settings_use_accent_color();emit("A");settings_use_default_color();emit("B\n");
    }
    assert(gScrollbackCount==SCROLLBACK_LINES);
    for (unsigned i=0;i<SCROLLBACK_LINES-1;i++) {
        assert(gScrollback[i][0]=='A' && gScrollbackStyles[i][0]==SCROLLBACK_ACCENT);
        assert(gScrollback[i][1]=='B' && gScrollbackStyles[i][1]==SCROLLBACK_DEFAULT);
    }
    /* Wrapping/backspace and recycled rows cannot inherit stale styling/text. */
    scrollback_reset();settings_use_accent_color();
    for (unsigned i=0;i<80;i++) scrollback_capture_char('A');
    settings_use_default_color();scrollback_capture_char('B');
    assert(gScrollbackWrapped[1] && gScrollbackStyles[1][0]==SCROLLBACK_DEFAULT);
    scrollback_capture_char('\b');scrollback_capture_char('\b');
    assert(gScrollbackCount==1);
    scrollback_capture_char('\n');
    assert(gScrollback[1][0]==0 && gScrollbackStyles[1][0]==0);
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix="tinygpt-scrollback-") as directory:
            path = Path(directory)
            (path / "scrollback.c").write_text(declarations + history + colors + assertions)
            subprocess.run([compiler, "-std=gnu11", "-O2", "-Wall", "-Wextra", "-Werror",
                            "-Wno-unused-function", str(path / "scrollback.c"), "-o", str(path / "scrollback")],
                           check=True, capture_output=True, text=True)
            subprocess.run([str(path / "scrollback")], check=True, timeout=10)


if __name__ == "__main__":
    unittest.main()
