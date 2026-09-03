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
        self.assertIn('#define TINYARMOS_VERSION "0.1.4"', SOURCE)

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
            "bootmgr",
            "reboot",
            "shutdown",
        ]
        for command in commands:
            with self.subTest(command=command):
                self.assertIn(command, help_text)

    def test_boot_manager_help_documents_every_command(self) -> None:
        help_text = source_block(
            "static void boot_manager_help(void)",
            "static void boot_manager_shell(void)",
        )
        commands = [
            "help",
            "scan",
            "repair",
            "rollback",
            "pwd",
            "ls [PATH]",
            "cd [PATH|-]",
            "cat PATH",
            "stat PATH",
            "tree [PATH]",
            "reset",
            "scroll",
            "scroll clear",
            "continue",
            "reboot",
            "shutdown",
        ]
        for command in commands:
            with self.subTest(command=command):
                self.assertIn(command, help_text)

    def test_startup_runs_the_same_integrity_scan_as_boot_manager(self) -> None:
        scan = source_block(
            "static int fs_scan_integrity(int verbose)",
            "static int fs_commit(void)",
        )
        boot = source_block(
            "static int boot_screen(EFI_HANDLE imageHandle)",
            "static void boot_manager_help(void)",
        )
        manager = source_block(
            "static void boot_manager_shell(void)", "static void command_help(void)"
        )
        self.assertIn("fs_check(0, verbose)", scan)
        self.assertIn("storage_probe_slots();", scan)
        self.assertIn("fs_scan_integrity(0);", boot)
        self.assertLess(
            boot.index("fs_scan_integrity(0);"),
            boot.index('boot_stage(5, "interactive shell", 1);'),
        )
        self.assertIn("fs_scan_integrity(1);", manager)

    def test_boot_manager_routing_and_scrollback_lifecycle(self) -> None:
        boot = source_block(
            "static int boot_screen(EFI_HANDLE imageHandle)",
            "static void boot_manager_help(void)",
        )
        manager = source_block(
            "static void boot_manager_shell(void)", "static void command_help(void)"
        )
        entry = source_block("EFI_STATUS EFIAPI EfiMain", "for (;;) {")
        self.assertIn("Press R for TinyArmOS Boot Manager (2 seconds)", boot)
        self.assertIn("return errors != 0;", boot)
        self.assertIn("=== TinyArmOS Boot Manager ===", manager)
        self.assertIn("if (!gScrollbackEnabled) scrollback_enable();", manager)
        self.assertLess(
            entry.index("settings_apply_runtime();"),
            entry.index("boot_manager_shell();"),
        )

    def test_boot_manager_uses_all_scrollback_navigation_keys(self) -> None:
        reader = source_block(
            "static void read_line(char *line, UINTN capacity)",
            "static const char *settings_color_name",
        )
        for scan_code in (1, 2, 5, 6, 9, 10, 23):
            with self.subTest(scan_code=scan_code):
                self.assertIn(f"key.ScanCode == {scan_code}", reader)
        self.assertIn("#define SCROLLBACK_LINES 256", SOURCE)

    def test_boot_manager_commands_are_simplified(self) -> None:
        dispatch = source_block(
            "static void boot_manager_shell(void)", "static void command_help(void)"
        )
        for removed in ("restore", "unlock", "lock", "protect"):
            with self.subTest(command=removed):
                self.assertNotIn(f'streq(line, "{removed}")', dispatch)
        self.assertNotIn('starts_with(line, "protect ")', dispatch)
        self.assertNotIn("command_protect(line);", dispatch)

    def test_legacy_manager_trees_are_migrated_before_bootmgr_metadata(self) -> None:
        restore = source_block(
            "static int fs_restore_system(void)", "static void fs_format(void)"
        )
        migration = source_block(
            "static int fs_remove_legacy_manager_trees(void)",
            "static int fs_restore_system(void)",
        )
        self.assertIn('fs_find_child(FS_ROOT, "recovery")', migration)
        self.assertIn('fs_find_child((UINTN)apps, "recovery")', migration)
        self.assertIn("fs_remove_recursive", migration)
        self.assertIn("gCwd = FS_ROOT", migration)
        self.assertIn("gPreviousCwd = FS_ROOT", migration)
        self.assertLess(
            SOURCE.index("fs_remove_legacy_manager_trees();"),
            SOURCE.index('fs_ensure_dir((UINTN)apps, "bootmgr"'),
        )
        self.assertNotIn('fs_ensure_dir(FS_ROOT, "recovery"', restore)
        self.assertNotIn('fs_ensure_dir((UINTN)apps, "recovery"', restore)
        self.assertIn('fs_ensure_dir((UINTN)apps, "bootmgr"', restore)
        self.assertIn("boot-manager.info", restore)
        boot = source_block(
            "static int boot_screen(EFI_HANDLE imageHandle)",
            "static void boot_manager_help(void)",
        )
        migration_commit = boot.split("else if (fs_restore_system())", 1)[1]
        self.assertIn("fs_commit();", migration_commit)

    def test_removed_agent_branding_and_shell_command_are_absent(self) -> None:
        for removed in ("Recovery Agent", "recovery_agent", "recovery_help"):
            with self.subTest(removed=removed):
                self.assertNotIn(removed, SOURCE)
        dispatch = source_block(
            "static void run_command(char *line)", "__attribute__((used))"
        )
        self.assertIn('streq(command, "bootmgr")', dispatch)
        self.assertNotIn('streq(command, "recovery")', dispatch)

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
            "recovery",
        ]
        for alias in aliases:
            with self.subTest(alias=alias):
                pattern = rf"(?:streq|starts_with)\(command, \"{re.escape(alias)}(?: )?\"\)"
                self.assertIsNone(re.search(pattern, dispatch))
        self.assertNotIn('starts_with(command, "run ")', dispatch)

    def test_exact_root_recursive_remove_destroys_the_os(self) -> None:
        remove_dispatch = source_block(
            '} else if (starts_with(command, "rm ")',
            '} else if (starts_with(command, "cp ")',
        )
        self.assertIn('rootRequest = recursive && streq(path, "/")', remove_dispatch)
        self.assertIn("rootRequest ? (int)FS_ROOT", remove_dispatch)
        root_branch = remove_dispatch.split("else if (rootRequest)", 1)[1].split(
            "} else if ((UINTN)node == FS_ROOT)", 1
        )[0]
        self.assertIn("gCwd = FS_ROOT;", root_branch)
        self.assertIn("gPreviousCwd = FS_ROOT;", root_branch)
        self.assertIn("fs_remove_recursive(FS_ROOT);", root_branch)
        self.assertIn("storage_wipe_os(&removed, &failures);", root_branch)
        self.assertIn("gStorageReady = 0;", root_branch)
        self.assertIn("ResetSystem(EfiResetShutdown", root_branch)
        self.assertNotIn("gProtectionUnlocked", root_branch)
        self.assertNotIn("read_line(", root_branch)
        self.assertNotIn("fs_commit()", root_branch)
        self.assertNotIn("ERASE ROOT", root_branch)

    def test_os_wipe_uses_dedicated_volume_identity_and_recursive_delete(self) -> None:
        self.assertIn('char16_equals_ascii(information->VolumeLabel, "TINYARMOS")', SOURCE)
        self.assertIn("storage_path_exists(gLoadedImagePath)", SOURCE)
        wipe = source_block(
            "static int storage_wipe_directory(",
            "static int storage_delete_path(",
        )
        self.assertIn("storage_collect_entries(directory", wipe)
        self.assertIn("storage_wipe_directory(child", wipe)
        self.assertIn("storage_clear_read_only(child)", wipe)
        self.assertIn("child->Delete(child)", wipe)
        self.assertLess(
            wipe.index("storage_collect_entries(directory"),
            wipe.index("child->Delete(child)"),
        )

    def test_os_wipe_preserves_unrelated_files_on_shared_esp(self) -> None:
        wipe = source_block(
            "static int storage_wipe_owned_files(",
            "static int storage_wipe_os(",
        )
        for owned_path in (
            "gSlot0Path",
            "gSlot1Path",
            "gDoomWadPath",
            "gDoomConfigPath",
            "gBootBackupPath",
            "gBootStagePath",
            "gLoadedImagePath",
        ):
            with self.subTest(path=owned_path):
                self.assertIn(owned_path, wipe)
        self.assertIn("storage_delete_owned_startup", wipe)
        self.assertNotIn("storage_wipe_directory(gVolumeRoot", wipe)

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
