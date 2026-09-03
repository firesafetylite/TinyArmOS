from __future__ import annotations

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src" / "uefi.c").read_text(encoding="utf-8")
README = (ROOT / "README.md").read_text(encoding="utf-8")


def source_block(start: str, end: str) -> str:
    return SOURCE.split(start, 1)[1].split(end, 1)[0]


class ShellSourceTests(unittest.TestCase):
    def test_release_version(self) -> None:
        self.assertIn('#define TINYARMOS_VERSION "0.1.1"', SOURCE)

    def test_main_help_documents_every_canonical_command(self) -> None:
        help_text = source_block(
            "static void command_help(void)", "static void command_info(void)"
        )
        commands = [
            "help",
            "clear",
            "scroll",
            "scroll clear",
            "echo [TEXT]",
            "info",
            "uptime",
            "count",
            "pwd",
            "ls [PATH]",
            "tree [PATH]",
            "sysfiles",
            "apps",
            "home",
            "root",
            "up",
            "back",
            "go [PLACE|PATH]",
            "cd [PATH|-]",
            "open [PATH]",
            "cat PATH",
            "write PATH TEXT",
            "append PATH TEXT",
            "touch PATH",
            "mkdir PATH",
            "rm PATH",
            "rm -rf PATH",
            "rmdir PATH",
            "cp SOURCE DEST",
            "mv SOURCE DEST",
            "stat PATH",
            "df",
            "sync",
            "fsck",
            "fault PATH",
            "doom",
            "settings",
            "protect [status|unlock|lock]",
            "update [check]",
            "recovery",
            "reboot",
            "shutdown",
        ]
        for command in commands:
            with self.subTest(command=command):
                self.assertIn(command, help_text)

    def test_recovery_help_documents_every_recovery_command(self) -> None:
        help_text = source_block(
            "static void recovery_help(void)", "static void recovery_agent(void)"
        )
        commands = [
            "help",
            "scan",
            "repair",
            "rollback",
            "restore",
            "unlock",
            "lock",
            "pwd",
            "ls [PATH]",
            "cd [PATH|-]",
            "cat PATH",
            "stat PATH",
            "tree [PATH]",
            "reset",
            "continue",
            "reboot",
            "shutdown",
        ]
        for command in commands:
            with self.subTest(command=command):
                self.assertIn(command, help_text)

    def test_redundant_shell_aliases_are_not_dispatched(self) -> None:
        dispatch = source_block(
            "static void run_command(char *line)", "__attribute__((used))"
        )
        aliases = [
            "?",
            "cls",
            "version",
            "history",
            "where",
            "dir",
            "list",
            "system",
            "view",
            "rename",
            "freedoom",
            "run doom",
            "poweroff",
        ]
        for alias in aliases:
            with self.subTest(alias=alias):
                pattern = rf"(?:streq|starts_with)\(command, \"{re.escape(alias)}(?: )?\"\)"
                self.assertIsNone(re.search(pattern, dispatch))
        self.assertNotIn('starts_with(command, "run ")', dispatch)

    def test_file_views_use_semantic_accent_colors(self) -> None:
        listing = source_block(
            "static void fs_list(UINTN directory)",
            "static void fs_tree_node(UINTN node, UINTN depth)",
        )
        tree = source_block(
            "static void fs_tree_node(UINTN node, UINTN depth)",
            "static void fs_tree(UINTN node)",
        )
        for view in (listing, tree):
            self.assertIn("settings_use_accent_color();", view)
            self.assertIn("settings_use_default_color();", view)
        self.assertIn('print("  [system]");', listing)

    def test_settings_is_full_screen_and_auto_saves(self) -> None:
        settings_ui = source_block(
            "static void settings_show(const char *notice)",
            '#include "update.inc"',
        )
        self.assertIn("Changes save automatically", settings_ui)
        self.assertIn("Return to shell", settings_ui)
        self.assertIn("gST->ConOut->ClearScreen", settings_ui)
        self.assertIn("settings_save_notice()", settings_ui)
        self.assertNotIn("Save and exit", settings_ui)

    def test_readme_keeps_user_disclaimer(self) -> None:
        self.assertIn(
            "DISCLAIMER: TinyArmOS is a project fully managed by ChatGPT codex",
            README,
        )


if __name__ == "__main__":
    unittest.main()
