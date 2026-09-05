"""Exercise the production FAT path builders with a small metadata fixture."""
from __future__ import annotations

import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class NativePathTests(unittest.TestCase):
    def test_nested_node_and_child_paths_keep_filename_validation(self) -> None:
        compiler = shutil.which("cc")
        if not compiler:
            self.skipTest("Native path tests require a host C compiler")
        source = (ROOT / "src/uefi.c").read_text()
        helpers = source.split("static void wide_copy(", 1)[1].split(
            "static int storage_create_directory(", 1)[0]
        harness = r'''
#include <assert.h>
#include <stdint.h>
typedef uint8_t UINT8;
typedef uint16_t CHAR16;
typedef unsigned long long UINTN;
#define FS_MAX_NODES 96U
#define FS_NAME_BYTES 32U
#define FS_ROOT 0U
static const CHAR16 gDirectRootPath[] = {'\\','T','I','N','Y','G','P','T','F','S','\\','R','O','O','T',0};
static struct { int used; UINTN parent; char name[FS_NAME_BYTES]; } gNodes[FS_MAX_NODES] = {
    {1, 0, ""}, {1, 0, "home"}, {1, 1, "hello.txt"}
};
''' + "static void wide_copy(" + helpers + r'''
int main(void) {
    CHAR16 path[260] = {0};
    assert(storage_node_path(0, path));
    assert(wide_path_equal(path, gDirectRootPath));
    assert(storage_node_path(2, path));
    assert(wide_path_equal(path, (const CHAR16 *)L"\\TINYGPTFS\\ROOT\\home\\hello.txt"));
    assert(storage_direct_node_path_valid(path));
    assert(storage_child_path(1, "new.txt", path));
    assert(wide_path_equal(path, (const CHAR16 *)L"\\TINYGPTFS\\ROOT\\home\\new.txt"));
    assert(!storage_child_path(1, "bad\\name", path));
    assert(!storage_node_path(95, path));
    CHAR16 full[] = {'a', 'b', 0};
    assert(!wide_append_separator(full, 3));
    assert(!wide_append_ascii(full, "c", 3));
    CHAR16 unterminated[] = {'a', 'b'};
    assert(!wide_append_ascii(unterminated, "", 2));
    gNodes[1].parent = 2;
    assert(!storage_node_path(2, path));
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix="tinygpt-paths-") as directory:
            path = pathlib.Path(directory)
            (path / "paths.c").write_text(harness)
            subprocess.run([compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                            "-fshort-wchar", str(path / "paths.c"), "-o", str(path / "paths")],
                           check=True, capture_output=True, text=True)
            subprocess.run([str(path / "paths")], check=True,
                           capture_output=True, text=True, timeout=10)


if __name__ == "__main__":
    unittest.main()
