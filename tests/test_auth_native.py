"""Run the actual authentication implementation, not just a Python model."""
from __future__ import annotations

import hashlib
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class NativeAuthenticationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        compiler = shutil.which("cc")
        if not compiler:
            raise unittest.SkipTest("Native authentication tests require a host C compiler")
        cls.temp = tempfile.TemporaryDirectory(prefix="tinygpt-auth-")
        cls.addClassCleanup(cls.temp.cleanup)
        cls.executable = pathlib.Path(cls.temp.name) / "auth-tests"
        subprocess.run(
            [compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
             "-Wno-unused-function", "-Wno-unused-variable", "-fshort-wchar",
             str(ROOT / "tests/auth_harness.c"), "-o", str(cls.executable)],
            check=True, capture_output=True, text=True,
        )

    def test_production_setup_login_roles_storage_errors_and_input(self) -> None:
        result = subprocess.run([str(self.executable)], check=True,
                                capture_output=True, text=True, timeout=30)
        self.assertIn("production authentication tests passed", result.stdout)

    def test_production_kdf_matches_independent_sha256_vectors(self) -> None:
        salt = bytes(range(16))
        for password in ("correct horse", "a" * 64, ""):
            with self.subTest(password_length=len(password)):
                raw = password.encode("ascii")
                expected = hashlib.sha256(salt + raw).digest()
                for _ in range(1, 4096):
                    expected = hashlib.sha256(expected + salt + raw).digest()
                actual = subprocess.run([str(self.executable), password], check=True,
                                        capture_output=True, text=True, timeout=30)
                self.assertEqual(actual.stdout.strip(), expected.hex())


if __name__ == "__main__":
    unittest.main()
