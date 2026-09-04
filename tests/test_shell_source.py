from __future__ import annotations

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src" / "uefi.c").read_text(encoding="utf-8")
UPDATE_SOURCE = (ROOT / "src" / "update.inc").read_text(encoding="utf-8")
PARTITION_SOURCE = (ROOT / "src" / "partition.inc").read_text(encoding="utf-8")
EDITOR_SOURCE = (ROOT / "src" / "editor.inc").read_text(encoding="utf-8")
README = (ROOT / "README.md").read_text(encoding="utf-8")
BUILD_SCRIPT = (ROOT / "build.sh").read_text(encoding="utf-8")
IMAGE_SOURCE = (ROOT / "tools" / "make_image.py").read_text(encoding="utf-8")
RELEASE_WORKFLOW = (ROOT / ".github" / "workflows" / "release.yml").read_text(
    encoding="utf-8"
)
NIGHTLY_WORKFLOW = (ROOT / ".github" / "workflows" / "nightly.yml").read_text(
    encoding="utf-8"
)
PAGES_WORKFLOW = (ROOT / ".github" / "workflows" / "pages.yml").read_text(
    encoding="utf-8"
)


def source_block(start: str, end: str) -> str:
    return SOURCE.split(start, 1)[1].split(end, 1)[0]


class ShellSourceTests(unittest.TestCase):
    def test_release_version(self) -> None:
        self.assertIn('#define TINYARMOS_VERSION "0.1.5"', SOURCE)

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
            "partitions",
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
            "textedit [PATH]",
            "doom",
            "settings",
            "protect [status|unlock|lock]",
            "update [check] [main|nightly]",
            "reboot",
            "shutdown",
        ]
        for command in commands:
            with self.subTest(command=command):
                self.assertIn(command, help_text)

    def test_pre_os_help_documents_every_recovery_command(self) -> None:
        help_text = source_block(
            "static void pre_os_help(void)",
            "static void pre_os_environment(void)",
        )
        commands = [
            "help",
            "partitions",
            "partition add MIB NAME",
            "partition name N NAME",
            "use N",
            "order N",
            "scan N",
            "repair N",
            "rollback N",
            "pwd",
            "ls [PATH]",
            "cd [PATH|-]",
            "cat PATH",
            "stat PATH",
            "tree [PATH]",
            "reset N",
            "scroll",
            "scroll clear",
            "boot",
            "reboot",
            "shutdown",
        ]
        for command in commands:
            with self.subTest(command=command):
                self.assertIn(command, help_text)

    def test_startup_and_pre_os_environment_share_the_integrity_scan(self) -> None:
        scan = source_block(
            "static int fs_scan_integrity(int verbose)",
            "static int fs_commit(void)",
        )
        boot = source_block(
            "static int boot_screen(EFI_HANDLE imageHandle)",
            "static void pre_os_help(void)",
        )
        pre_os = source_block(
            "static void pre_os_environment(void)", "static void command_help(void)"
        )
        self.assertIn("fs_check(0, verbose)", scan)
        self.assertIn("storage_probe_slots();", scan)
        self.assertIn("fs_scan_integrity(0);", boot)
        self.assertLess(
            boot.index("fs_scan_integrity(0);"),
            boot.index('boot_stage(5, "TinyArmOS operating system"'),
        )
        self.assertIn("fs_scan_integrity(1);", pre_os)

    def test_r_routes_to_pre_os_before_the_normal_shell(self) -> None:
        boot = source_block(
            "static int boot_screen(EFI_HANDLE imageHandle)",
            "static void pre_os_help(void)",
        )
        pre_os = source_block(
            "static void pre_os_environment(void)", "static void command_help(void)"
        )
        entry = source_block("EFI_STATUS EFIAPI EfiMain", "for (;;) {")
        menu = source_block("static void pre_os_draw_boot_menu", "static int boot_screen")
        self.assertIn("Up/Down select, Enter boot, S save default, R recovery", menu)
        self.assertIn("Press Enter to interrupt boot and open the partition menu", menu)
        self.assertIn("boot_order_save(selected)", menu)
        self.assertIn("ClearScreen", menu)
        self.assertIn("pre_os_draw_boot_menu(selected", menu)
        self.assertNotIn("Selected partition ", menu)
        self.assertIn("targetPartition = pre_os_boot_prompt();", boot)
        self.assertIn("if (targetPartition == 1U) return 1;", boot)
        self.assertIn("=== TinyArmOS Pre-OS Environment ===", pre_os)
        self.assertIn("TinyArmOS has not started", pre_os)
        self.assertIn("if (!gScrollbackEnabled) scrollback_enable();", pre_os)
        self.assertLess(entry.index("pre_os_environment();"), entry.index("settings_load();"))

    def test_missing_os_routes_to_pre_os_and_can_be_repaired(self) -> None:
        boot = source_block(
            "static int boot_screen(EFI_HANDLE imageHandle)",
            "static void pre_os_help(void)",
        )
        repair = source_block(
            "static int pre_os_repair(UINTN partition)",
            "static void pre_os_print_partitions",
        )
        self.assertIn("osMissing = storage_os_missing();", boot)
        self.assertIn("storage_path_exists(gFactoryInstallPath)", boot)
        self.assertIn("else if (!mounted && !snapshotFiles)", boot)
        self.assertIn("OS MISSING - OPENING PRE-OS ENVIRONMENT", boot)
        self.assertNotIn("fs_check(1, 0)", boot)
        self.assertIn("partition == 1U", repair)
        self.assertIn("storage_activate_partition(partition)", repair)
        self.assertIn("fs_check(1, 1);", repair)
        self.assertIn("fs_format();", repair)
        self.assertIn("fs_commit()", repair)
        self.assertIn("storage_clear_os_missing()", repair)

    def test_pre_os_environment_uses_all_scrollback_navigation_keys(self) -> None:
        reader = source_block(
            "static void read_line(char *line, UINTN capacity)",
            "static const char *settings_color_name",
        )
        for scan_code in (1, 2, 5, 6, 9, 10, 23):
            with self.subTest(scan_code=scan_code):
                self.assertIn(f"key.ScanCode == {scan_code}", reader)
        self.assertIn("#define SCROLLBACK_LINES 256", SOURCE)

    def test_pre_os_recovery_commands_are_restricted(self) -> None:
        dispatch = source_block(
            "static void pre_os_environment(void)", "static void command_help(void)"
        )
        for removed in ("restore", "unlock", "lock", "protect"):
            with self.subTest(command=removed):
                self.assertNotIn(f'streq(line, "{removed}")', dispatch)
        self.assertNotIn('starts_with(line, "protect ")', dispatch)
        self.assertNotIn("command_protect(line);", dispatch)

    def test_legacy_recovery_and_bootmgr_apps_are_removed_from_minifs(self) -> None:
        restore = source_block(
            "static int fs_restore_system(void)", "static void fs_format(void)"
        )
        migration = source_block(
            "static int fs_remove_legacy_manager_trees(void)",
            "static int fs_restore_system(void)",
        )
        self.assertIn('fs_find_child(FS_ROOT, "recovery")', migration)
        self.assertIn('fs_find_child((UINTN)apps, "recovery")', migration)
        self.assertIn('fs_find_child((UINTN)apps, "bootmgr")', migration)
        self.assertIn("fs_remove_recursive", migration)
        self.assertIn("gCwd = FS_ROOT", migration)
        self.assertIn("gPreviousCwd = FS_ROOT", migration)
        self.assertNotIn('fs_ensure_dir(FS_ROOT, "recovery"', restore)
        self.assertNotIn('fs_ensure_dir((UINTN)apps, "recovery"', restore)
        self.assertNotIn('fs_ensure_dir((UINTN)apps, "bootmgr"', restore)
        self.assertIn('fs_find_child((UINTN)boot, "boot-manager.info")', restore)
        self.assertIn("pre-os.info", restore)
        boot = source_block(
            "static int boot_screen(EFI_HANDLE imageHandle)",
            "static void pre_os_help(void)",
        )
        migration_commit = boot.split("fs_restore_system())", 1)[1]
        self.assertIn("fs_commit();", migration_commit)

    def test_recovery_is_pre_os_only_not_an_in_os_command(self) -> None:
        for removed in ("Recovery Agent", "recovery_agent", "recovery_help", "TinyArmOS Boot Manager"):
            with self.subTest(removed=removed):
                self.assertNotIn(removed, SOURCE)
        dispatch = source_block(
            "static void run_command(char *line)", "__attribute__((used))"
        )
        self.assertNotIn('streq(command, "bootmgr")', dispatch)
        self.assertNotIn('streq(command, "recovery")', dispatch)
        shell_help = source_block("static void command_help(void)", "static void command_info(void)")
        self.assertNotIn("bootmgr", shell_help)

    def test_shell_partitions_is_read_only_and_points_to_pre_os(self) -> None:
        dispatch = source_block(
            "static void run_command(char *line)", "__attribute__((used))"
        )
        command = dispatch.split('streq(command, "partitions")', 1)[1].split(
            '} else if (streq(command, "pwd"))', 1
        )[0]
        self.assertIn("pre_os_print_partitions(gActivePartition)", command)
        self.assertIn("read-only from TinyArmOS", command)
        self.assertIn("press R", command)
        self.assertNotIn("partition_add", command)
        self.assertNotIn("partition_rename", command)

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
            "edit",
            "freedoom",
            "run doom",
            "poweroff",
            "recovery",
            "bootmgr",
        ]
        for alias in aliases:
            with self.subTest(alias=alias):
                pattern = rf"(?:streq|starts_with)\(command, \"{re.escape(alias)}(?: )?\"\)"
                self.assertIsNone(re.search(pattern, dispatch))
        self.assertNotIn('starts_with(command, "run ")', dispatch)

    def test_exact_root_recursive_remove_destroys_os_but_leaves_pre_os(self) -> None:
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
        self.assertIn("The pre-OS environment remains", root_branch)
        self.assertIn("rootRequest && !gProtectionUnlocked", remove_dispatch)
        self.assertIn("use 'protect unlock'", remove_dispatch)
        self.assertNotIn("read_line(", root_branch)
        self.assertNotIn("fs_commit()", root_branch)
        self.assertNotIn("ERASE ROOT", root_branch)

    def test_os_wipe_uses_dedicated_volume_identity_and_recursive_delete(self) -> None:
        self.assertIn("return gActivePartition >= 2U", SOURCE)
        self.assertIn('storage_volume_has_label(gBootVolumeRoot, "TINYARMOS")', SOURCE)
        wipe = source_block(
            "static int storage_wipe_directory(",
            "static int storage_delete_path(",
        )
        self.assertIn("storage_collect_entries(directory", wipe)
        self.assertIn("storage_wipe_directory(child", wipe)
        self.assertIn('char16_equals_ascii(entries[index].name, "BOOTAA64.EFI")', wipe)
        self.assertIn("if (keepManager)", wipe)
        self.assertIn("storage_clear_read_only(child)", wipe)
        self.assertIn("child->Delete(child)", wipe)
        self.assertLess(
            wipe.index("storage_collect_entries(directory"),
            wipe.index("child->Delete(child)"),
        )

    def test_partition_management_is_targeted_and_protects_recovery(self) -> None:
        pre_os = source_block(
            "static void pre_os_environment(void)", "static void command_help(void)"
        )
        for command in ("scan", "repair", "rollback", "reset"):
            with self.subTest(command=command):
                self.assertIn(f'starts_with(line, "{command} ")', pre_os)
                self.assertIn(
                    f'print("{command}: provide a non-protected partition number', pre_os
                )
        self.assertIn("if (partition == 1U)", SOURCE)
        self.assertIn("partition < 2U", PARTITION_SOURCE)
        self.assertIn("partition_add(mebibytes", pre_os)
        self.assertIn("partition_rename(partition", pre_os)
        self.assertIn("gFatPartitionGuid", PARTITION_SOURCE)
        self.assertIn("partition_format_fat16", PARTITION_SOURCE)
        self.assertIn('memory_copy(sector + 32U, "TINYOS  NEW", 11U)', PARTITION_SOURCE)
        self.assertIn("sector[32U + 11U] = 0x20", PARTITION_SOURCE)
        self.assertLess(
            PARTITION_SOURCE.index("disk->backupEntriesLba"),
            PARTITION_SOURCE.index("disk->primaryEntriesLba", PARTITION_SOURCE.index("static int partition_disk_commit")),
        )

    def test_image_reserves_append_only_partition_space(self) -> None:
        self.assertIn("IMAGE_BYTES = 128 * 1024 * 1024", IMAGE_SOURCE)
        self.assertIn("SYSTEM_LAST = 131038", IMAGE_SOURCE)
        self.assertIn("format_system_fat32(image, SYSTEM_LAST", IMAGE_SOURCE)

    def test_split_layout_wipes_the_entire_system_partition(self) -> None:
        wipe = source_block(
            "static int storage_wipe_os(",
            "static int storage_read_slot(",
        )
        self.assertIn("if (!gLegacySinglePartition)", wipe)
        self.assertIn("storage_wipe_directory(gVolumeRoot, 99U", wipe)
        self.assertIn("remainingCount", wipe)
        self.assertNotIn("storage_set_os_missing", wipe.split("if (!gLegacySinglePartition)", 1)[1].split("if (!gDedicatedStorage)", 1)[0])

    def test_legacy_shared_esp_wipe_preserves_unrelated_files(self) -> None:
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
            "gFactoryInstallPath",
            "gOsMissingPath",
            "gBootPath",
        ):
            with self.subTest(path=owned_path):
                self.assertIn(owned_path, wipe)
        self.assertIn("storage_delete_owned_startup", wipe)
        self.assertIn("storage_set_os_missing", wipe)
        self.assertNotIn("storage_delete_path(gLoadedImagePath", wipe)
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

    def test_text_editor_is_a_protected_aware_full_screen_app(self) -> None:
        restore = source_block("static int fs_restore_system(void)", "static void fs_format(void)")
        dispatch = source_block("static void run_command(char *line)", "__attribute__((used))")
        self.assertIn('#include "editor.inc"', SOURCE)
        self.assertIn('fs_ensure_dir((UINTN)apps, "editor", FS_PROTECTED)', restore)
        self.assertIn("editor    command: textedit [PATH]", restore)
        self.assertIn("TinyArmOS Text Editor", restore)
        self.assertIn("!streq(gNodes[previousEditorInfo].data, editorAppInfo)", restore)
        self.assertIn('streq(command, "textedit")', dispatch)
        self.assertIn("command_textedit(command)", dispatch)
        self.assertNotIn('streq(command, "edit")', dispatch)
        self.assertNotIn('starts_with(command, "edit ")', dispatch)
        self.assertIn("static char gEditorBuffer[FS_DATA_BYTES]", EDITOR_SOURCE)
        self.assertIn("fs_is_protected", EDITOR_SOURCE)
        self.assertIn("!gProtectionUnlocked", EDITOR_SOURCE)
        self.assertIn("F2/Ctrl+S Save", EDITOR_SOURCE)
        self.assertIn("Unsaved changes", EDITOR_SOURCE)
        self.assertIn("editor_save", EDITOR_SOURCE)
        self.assertIn("fs_commit()", EDITOR_SOURCE)
        self.assertIn("gEditorSaveBackup", EDITOR_SOURCE)
        self.assertIn("gGeneration = previousGeneration", EDITOR_SOURCE)
        self.assertIn("*node = -1", EDITOR_SOURCE)
        self.assertIn("original file restored", EDITOR_SOURCE)
        self.assertIn("EDITOR_SCAN_DELETE", EDITOR_SOURCE)
        self.assertIn("static void editor_visual_position", EDITOR_SOURCE)
        self.assertIn("static int editor_visual_offset", EDITOR_SOURCE)
        self.assertIn("static void editor_visual_row_text", EDITOR_SOURCE)
        self.assertIn("Soft wrap is on; Up/Down moves and scrolls wrapped rows.", EDITOR_SOURCE)
        self.assertIn("Arrow Keys Move/Scroll", EDITOR_SOURCE)
        self.assertNotIn("EDITOR_SCAN_HOME", EDITOR_SOURCE)
        self.assertNotIn("EDITOR_SCAN_END", EDITOR_SOURCE)
        self.assertNotIn("EDITOR_SCAN_PAGE_UP", EDITOR_SOURCE)
        self.assertNotIn("EDITOR_SCAN_PAGE_DOWN", EDITOR_SOURCE)
        self.assertIn("static int editor_file_picker", EDITOR_SOURCE)
        self.assertIn("static int editor_new_file_modal", EDITOR_SOURCE)
        self.assertIn("editor_file_picker(requestedPath", EDITOR_SOURCE)
        self.assertIn("[ New text file ]", EDITOR_SOURCE)
        self.assertIn("Left/Backspace/B Parent", EDITOR_SOURCE)
        self.assertIn("Left/Esc Back to File Picker", EDITOR_SOURCE)
        self.assertIn("key.ScanCode == EDITOR_SCAN_LEFT", EDITOR_SOURCE)
        self.assertIn("key.UnicodeChar == 'B'", EDITOR_SOURCE)
        self.assertNotIn("read_line(", EDITOR_SOURCE)
        editor_draw = EDITOR_SOURCE.split("static void editor_draw", 1)[1].split(
            "static int editor_save", 1
        )[0]
        picker_draw = EDITOR_SOURCE.split("static void editor_picker_draw", 1)[1].split(
            "static void editor_new_file_draw", 1
        )[0]
        modal_draw = EDITOR_SOURCE.split("static void editor_new_file_draw", 1)[1].split(
            "static int editor_new_file_modal", 1
        )[0]
        self.assertNotIn("ClearScreen", editor_draw)
        self.assertNotIn("ClearScreen", picker_draw)
        self.assertNotIn("ClearScreen", modal_draw)
        self.assertEqual(EDITOR_SOURCE.count("ClearScreen"), 2)
        self.assertIn("gEditorScreenValid", EDITOR_SOURCE)
        self.assertIn("if (unchanged) return", EDITOR_SOURCE)
        self.assertIn("static void editor_wait_key", EDITOR_SOURCE)
        self.assertEqual(EDITOR_SOURCE.count("editor_wait_key(&key);"), 3)

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

    def test_update_command_routes_main_and_nightly_channels(self) -> None:
        dispatch = source_block(
            "static void run_command(char *line)", "__attribute__((used))"
        )
        for command in (
            "update main",
            "update nightly",
            "update check main",
            "update check nightly",
        ):
            with self.subTest(command=command):
                self.assertIn(f'streq(command, "{command}")', dispatch)
        self.assertIn("command_update(checkOnly, nightly);", dispatch)
        self.assertIn("UPDATE_MAIN_MANIFEST_URL", UPDATE_SOURCE)
        self.assertIn("UPDATE_NIGHTLY_MANIFEST_URL", UPDATE_SOURCE)
        self.assertIn("nightly/TinyArmOS-update.txt", UPDATE_SOURCE)
        self.assertIn("update_digest_equal(currentDigest, manifest.digest)", UPDATE_SOURCE)
        self.assertIn("TINYARMOS_DISPLAY_VERSION", UPDATE_SOURCE)
        self.assertIn('TINYARMOS_BUILD_CHANNEL "main"', SOURCE)

    def test_nightly_pipeline_keeps_main_and_beta_channels_separate(self) -> None:
        self.assertIn("branches:\n      - nightly", NIGHTLY_WORKFLOW)
        self.assertIn("gh release create nightly", NIGHTLY_WORKFLOW)
        self.assertIn("TinyArmOS-nightly.img", NIGHTLY_WORKFLOW)
        self.assertIn("TinyArmOS-nightly-BOOTAA64.EFI", NIGHTLY_WORKFLOW)
        self.assertNotIn("TinyArmOS-v${VERSION}-nightly", NIGHTLY_WORKFLOW)
        self.assertIn("TINYARMOS_BUILD_CHANNEL: nightly", NIGHTLY_WORKFLOW)
        self.assertIn('--title "TinyArmOS nightly"', NIGHTLY_WORKFLOW)
        self.assertIn("--prerelease", NIGHTLY_WORKFLOW)
        self.assertIn("git merge-base --is-ancestor", RELEASE_WORKFLOW)
        self.assertIn("TinyArmOS-nightly-update.txt", PAGES_WORKFLOW)
        self.assertIn("--main release-main --nightly release-nightly", PAGES_WORKFLOW)

    def test_img_is_the_only_maintained_boot_distribution(self) -> None:
        self.assertIn(
            "tools/make_image.py build/BOOTAA64.EFI build/TinyArmOS.img",
            BUILD_SCRIPT,
        )
        self.assertNotIn("make_utm_bundle.py", BUILD_SCRIPT)
        self.assertIn(
            'cp build/TinyArmOS.img "dist/TinyArmOS-${tag}.img"',
            RELEASE_WORKFLOW,
        )
        self.assertNotIn("UTM.utm.zip", RELEASE_WORKFLOW)
        self.assertFalse((ROOT / "tools" / "make_utm_bundle.py").exists())

    def test_readme_keeps_user_disclaimer(self) -> None:
        self.assertIn(
            "DISCLAIMER: TinyArmOS is a project fully managed by ChatGPT codex",
            README,
        )


if __name__ == "__main__":
    unittest.main()
