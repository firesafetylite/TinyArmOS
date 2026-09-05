from __future__ import annotations

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src" / "uefi.c").read_text(encoding="utf-8")
UPDATE_SOURCE = (ROOT / "src" / "update.inc").read_text(encoding="utf-8")
PARTITION_SOURCE = (ROOT / "src" / "partition.inc").read_text(encoding="utf-8")
EDITOR_SOURCE = (ROOT / "src" / "editor.inc").read_text(encoding="utf-8")
AUTH_SOURCE = (ROOT / "src" / "auth.inc").read_text(encoding="utf-8")
ACCOUNT_SETTINGS_SOURCE = (ROOT / "src" / "account_settings.inc").read_text(encoding="utf-8")
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
        self.assertIn('#define TINYGPT_VERSION "0.1.6"', SOURCE)

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
            "partitions",
            "pwd",
            "ls [PATH]",
            "tree [PATH]",
            "cd [PATH|-]",
            "cat PATH",
            "write PATH [TEXT]",
            "append PATH TEXT",
            "mkdir PATH",
            "rm PATH",
            "rm -rf PATH",
            "rmdir PATH",
            "cp SOURCE DEST",
            "mv SOURCE DEST",
            "stat PATH",
            "df",
            "fsck",
            "textedit [PATH]",
            "doom",
            "settings",
            "logout",
            "update [check] [main|nightly]",
            "reboot",
            "shutdown",
        ]
        for command in commands:
            with self.subTest(command=command):
                self.assertIn(command, help_text)
        self.assertIn(
            "rm -rf PATH          recursively remove a directory tree\\n\"\n"
            "        \"  rmdir PATH",
            help_text,
        )

    def test_pre_os_help_documents_every_recovery_command(self) -> None:
        help_text = source_block(
            "static void pre_os_help(void)",
            "static void pre_os_environment(void)",
        )
        commands = [
            "help",
            "partitions",
            "partition add MIB NAME",
            "partition delete N",
            "partition name N NAME",
            "use N",
            "order N",
            "scan N",
            "repair N",
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
        scan = source_block("static int fs_scan_integrity(int verbose)", "static int fs_commit(void)")
        boot = source_block("static int boot_screen(EFI_HANDLE imageHandle)", "static void pre_os_help(void)")
        pre_os = source_block("static void pre_os_environment(void)", "static void command_help(void)")
        self.assertIn("storage_marker_valid()", scan)
        self.assertIn("storage_transaction_pending()", scan)
        self.assertIn("fs_check(0, verbose)", scan)
        self.assertIn("fs_scan_integrity(0);", boot)
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
        self.assertIn("Press Enter for the partition menu, or Esc / R for firmware recovery", menu)
        self.assertIn("boot_order_save(selected)", menu)
        self.assertIn("ClearScreen", menu)
        self.assertIn("pre_os_draw_boot_menu(selected", menu)
        self.assertNotIn("Selected partition ", menu)
        self.assertIn("targetPartition = pre_os_boot_prompt();", boot)
        self.assertIn("if (targetPartition == 1U) return 1;", boot)
        self.assertIn("=== TinyGPT Pre-OS Environment ===", pre_os)
        self.assertIn("TinyGPT has not started", pre_os)
        self.assertIn("if (!gScrollbackEnabled) scrollback_enable();", pre_os)
        self.assertLess(entry.index("pre_os_environment();"), entry.index("settings_load();"))

    def test_missing_os_routes_to_pre_os_and_can_be_repaired(self) -> None:
        boot = source_block("static int boot_screen(EFI_HANDLE imageHandle)", "static void pre_os_help(void)")
        repair = source_block("static int pre_os_repair(UINTN partition)", "static void pre_os_print_partitions")
        self.assertIn("osMissing = storage_os_missing();", boot)
        self.assertIn("storage_path_exists(gFactoryInstallPath)", boot)
        self.assertIn("'T','I','N','Y','G','P','T','.','N','E','W'", SOURCE)
        self.assertIn("storage_install_empty()", boot)
        self.assertIn("OS MISSING - OPENING PRE-OS ENVIRONMENT", boot)
        self.assertIn("storage_activate_partition(partition)", repair)
        self.assertIn("storage_install_empty()", repair)
        self.assertIn("fs_check(1, 1)", repair)
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
        restore = source_block("static int fs_restore_system(void)", "static void fs_format(void)")
        migration = source_block("static int fs_remove_legacy_manager_trees(void)", "static int fs_restore_system(void)")
        self.assertIn('fs_find_child(FS_ROOT, "recovery")', migration)
        self.assertIn('fs_find_child((UINTN)apps, "recovery")', migration)
        self.assertIn('fs_find_child((UINTN)apps, "bootmgr")', migration)
        self.assertNotIn('fs_ensure_dir(FS_ROOT, "recovery"', restore)
        self.assertIn("pre-os.info", restore)

    def test_recovery_is_pre_os_only_not_an_in_os_command(self) -> None:
        for removed in ("Recovery Agent", "recovery_agent", "recovery_help", "TinyGPT Boot Manager"):
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
        self.assertIn("read-only from TinyGPT", command)
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
            "count",
            "sysfiles",
            "apps",
            "home",
            "root",
            "up",
            "back",
            "go",
            "open",
            "touch",
            "sync",
            "fault",
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

    def test_dead_loaded_image_tracking_is_removed(self) -> None:
        self.assertNotIn("gLoadedImagePath", SOURCE)
        self.assertNotIn("storage_capture_loaded_path", SOURCE)

    def test_runtime_settings_are_applied_once_before_the_shell(self) -> None:
        entry = SOURCE.split("EFI_STATUS EFIAPI EfiMain", 1)[1]
        self.assertEqual(entry.count("settings_apply_runtime();"), 1)

    def test_exact_root_recursive_remove_destroys_os_but_keeps_shell_running(self) -> None:
        remove_dispatch = source_block('} else if (starts_with(command, "rm ")', '} else if (starts_with(command, "cp ")')
        root_branch = remove_dispatch.split("else if (rootRequest)", 1)[1].split("} else if ((UINTN)node == FS_ROOT)", 1)[0]
        self.assertIn('rootRequest = recursive && streq(path, "/")', remove_dispatch)
        self.assertIn("storage_wipe_os(&removed, &failures)", root_branch)
        self.assertIn("Storage remains open", root_branch)
        self.assertIn("gStorageReady = 0;", root_branch)
        self.assertLess(root_branch.index("if (!complete)"), root_branch.index("gStorageReady = 0;"))
        self.assertNotIn("The pre-OS environment remains", root_branch)
        self.assertNotIn("The running shell will continue from RAM", root_branch)
        self.assertNotIn("ResetSystem(", root_branch)

    def test_os_wipe_uses_dedicated_volume_identity_and_recursive_delete(self) -> None:
        self.assertIn("return gActivePartition >= 2U", SOURCE)
        self.assertIn('storage_volume_has_label(gBootVolumeRoot, "TINYGPT")', SOURCE)
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
        for command in ("scan", "repair", "reset"):
            with self.subTest(command=command):
                self.assertIn(f'starts_with(line, "{command} ")', pre_os)
                self.assertIn(
                    f'print("{command}: provide a non-protected partition number', pre_os
                )
        self.assertIn("if (partition == 1U)", SOURCE)
        self.assertIn("partition < 2U", PARTITION_SOURCE)
        self.assertIn("partition_add(mebibytes", pre_os)
        self.assertIn("partition_rename(partition", pre_os)
        self.assertIn("partition_delete(partition", pre_os)
        self.assertIn("gFatPartitionGuid", PARTITION_SOURCE)
        self.assertIn("partition_format_fat16", PARTITION_SOURCE)
        repair = source_block("static int pre_os_repair(", "static void pre_os_print_partitions(")
        self.assertIn("legacy import failed; snapshots were left unchanged", repair)
        self.assertNotIn("fs_format();", repair)
        self.assertIn('memory_copy(sector + 32U, "TINYGPT NEW", 11U)', PARTITION_SOURCE)
        self.assertIn("sector[32U + 11U] = 0x20", PARTITION_SOURCE)
        self.assertLess(
            PARTITION_SOURCE.index("disk->backupEntriesLba"),
            PARTITION_SOURCE.index("disk->primaryEntriesLba", PARTITION_SOURCE.index("static int partition_disk_commit")),
        )

    def test_partition_delete_is_pre_os_only_and_password_authorized(self) -> None:
        help_text = source_block("static void pre_os_help(void)", "static void pre_os_environment(void)")
        pre_os = source_block("static void pre_os_environment(void)", "static void command_help(void)")
        shell = source_block("static void run_command(char *line)", "__attribute__((used))")
        self.assertIn("partition delete N", help_text)
        self.assertIn('starts_with(line, "partition delete ")', pre_os)
        self.assertIn('auth_pre_os_authorize_admin("partition delete")', pre_os)
        self.assertNotIn("DELETE PARTITION", pre_os)
        self.assertNotIn("streq(answer, expected)", pre_os)
        self.assertIn("not secure erasure", pre_os)
        self.assertIn("Reboot required", pre_os)
        self.assertNotIn("partition_delete", shell)

    def test_partition_delete_checks_ownership_and_clears_only_the_entry(self) -> None:
        validator = PARTITION_SOURCE.split("static int partition_entry_is_managed_fat", 1)[1].split(
            "static int partition_delete_validate", 1
        )[0]
        deletion = PARTITION_SOURCE.split("static int partition_delete(UINTN partition)", 1)[1].split(
            "static int partition_add", 1
        )[0]
        self.assertIn("partition < 2U", validator)
        self.assertIn("gPartitionNames[partition - 1U][0]", validator)
        self.assertIn("gFatPartitionGuid", validator)
        self.assertIn("partition_gpt_name_matches", validator)
        self.assertIn("partition_fat_label_matches", validator)
        self.assertIn("partition_table_extents_valid", validator)
        self.assertIn("boot_order_save(fallback)", deletion)
        self.assertLess(deletion.index("boot_order_save(fallback)"), deletion.index("partition_disk_commit"))
        self.assertLess(deletion.index("removedRoot->Close"), deletion.index("partition_disk_commit"))
        self.assertIn("gPartitionRoots[partition - 1U] = (EFI_FILE_PROTOCOL *)0", deletion)
        self.assertIn("memory_zero(entry, GPT_ENTRY_SIZE)", deletion)
        self.assertLess(deletion.index("partition_disk_commit"), deletion.index("partition_registry_save"))
        self.assertIn("gActivePartition == partition", deletion)
        self.assertIn("memory_zero(gNodes", deletion)

    def test_mirrored_gpt_commit_verifies_backup_before_primary(self) -> None:
        opened = PARTITION_SOURCE.split("static int partition_disk_open", 1)[1].split(
            "static void partition_refresh_header_crc", 1
        )[0]
        commit = PARTITION_SOURCE.split("static int partition_disk_commit", 1)[1].split(
            "static int partition_label_valid", 1
        )[0]
        self.assertIn("disk->backupEntriesLba", opened)
        self.assertIn("!partition_bytes_equal(disk->entries, disk->originalEntries", opened)
        self.assertLess(commit.index("disk->backupEntriesLba"), commit.index("disk->primaryEntriesLba"))
        self.assertLess(commit.index("FlushBlocks"), commit.index("disk->primaryEntriesLba"))
        self.assertIn("partition_copy_matches", commit)
        self.assertIn("gPartitionError = 31U", commit)

    def test_partition_add_uses_validated_aligned_first_fit(self) -> None:
        first_fit = PARTITION_SOURCE.split("static int partition_first_fit", 1)[1].split(
            "static int partition_gpt_name_matches", 1
        )[0]
        add = PARTITION_SOURCE.split("static int partition_add", 1)[1].split(
            "static const char *partition_error_text", 1
        )[0]
        self.assertIn("partition_table_extents_valid", first_fit)
        self.assertIn("partition_align_lba", first_fit)
        self.assertIn("sectors <= nextStart - cursor", first_fit)
        self.assertIn("sectors - 1U <= lastUsable - cursor", first_fit)
        self.assertIn("partition_first_fit(&disk, sectors, &start)", add)
        self.assertIn("for (index = 1U;", add)
        self.assertNotIn("for (index = 2U;", add)
        self.assertNotIn("maxEnd", add)

    def test_definite_delete_commit_failure_requires_reboot_after_handle_close(self) -> None:
        deletion = PARTITION_SOURCE.split("static int partition_delete(UINTN partition)", 1)[1].split(
            "static int partition_add", 1
        )[0]
        errors = PARTITION_SOURCE.split("static const char *partition_error_text", 1)[1].split(
            "static int partition_update_fat_label", 1
        )[0]
        failure = deletion.split("if (!partition_disk_commit(&disk))", 1)[1].split(
            "partition_disk_close(&disk);", 1
        )[1].split("return 0;", 1)[0]
        self.assertIn("gPartitionRebootRequired = 1U", failure)
        self.assertIn("target handle is closed; reboot required", errors)

    def test_obsolete_pre_os_rollback_command_is_removed(self) -> None:
        help_text = source_block("static void pre_os_help(void)", "static void pre_os_environment(void)")
        pre_os = source_block("static void pre_os_environment(void)", "static void command_help(void)")
        self.assertNotIn("rollback N", help_text)
        self.assertNotIn('starts_with(line, "rollback ")', pre_os)
        self.assertNotIn("storage_rollback", SOURCE)
        self.assertNotIn("rollback N", README)

    def test_image_reserves_partition_expansion_space(self) -> None:
        self.assertIn("IMAGE_BYTES = 128 * 1024 * 1024", IMAGE_SOURCE)
        self.assertIn("SYSTEM_LAST = 131038", IMAGE_SOURCE)
        self.assertIn("format_system_fat32(image, SYSTEM_LAST", IMAGE_SOURCE)

    def test_split_layout_wipes_the_entire_system_partition(self) -> None:
        wipe = source_block("static int storage_wipe_os(", "static int storage_mount_latest(")
        self.assertIn("if (gLegacySinglePartition) return storage_wipe_owned_files", wipe)
        self.assertIn("storage_wipe_directory(gVolumeRoot, 99U", wipe)
        self.assertIn("remainingCount", wipe)

    def test_legacy_shared_esp_wipe_preserves_unrelated_files(self) -> None:
        wipe = source_block("static int storage_wipe_owned_files(", "static int storage_wipe_os(")
        for owned_path in ("gDirectNamespacePath", "gSlot0Path", "gSlot1Path", "gLegacyRetiredPath", "gDoomWadPath", "gDoomConfigPath", "gBootBackupPath", "gBootStagePath", "gFactoryInstallPath"):
            with self.subTest(path=owned_path): self.assertIn(owned_path, wipe)
        self.assertIn("storage_delete_owned_startup", wipe)
        self.assertIn("storage_set_os_missing", wipe)
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
        self.assertIn('streq(command, "textedit")', dispatch)
        self.assertIn("static char gEditorBuffer[FS_DATA_BYTES]", EDITOR_SOURCE)
        self.assertIn("storage_read_node", EDITOR_SOURCE)
        self.assertIn("fs_write_file", EDITOR_SOURCE)
        self.assertNotIn("gEditorSaveBackup", EDITOR_SOURCE)
        self.assertIn("persistence not claimed", EDITOR_SOURCE)
        self.assertIn("fs_is_protected", EDITOR_SOURCE)
        self.assertIn("static int editor_file_picker", EDITOR_SOURCE)
        self.assertEqual(EDITOR_SOURCE.count("ClearScreen"), 2)

    def test_direct_fat_authority_migration_and_transactions(self) -> None:
        self.assertIn("gDirectRootPath", SOURCE)
        self.assertIn("gDirectMarkerPath", SOURCE)
        self.assertIn("gLegacyRetiredPath", SOURCE)
        self.assertIn("gTransactionBackupPath", SOURCE)
        self.assertIn("storage_scan_direct", SOURCE)
        self.assertIn("storage_replace_file", SOURCE)
        self.assertIn("storage_write_transaction", SOURCE)
        self.assertIn("storage_transaction_valid", SOURCE)
        transaction_reader = source_block("static int storage_read_transaction(", "static int storage_transaction_pending(")
        self.assertIn("storage_transaction_valid(transaction)", transaction_reader)
        self.assertLess(SOURCE.index("storage_write_transaction(&transaction)"), SOURCE.index("storage_rename_path(transaction.target"))
        self.assertIn("storage_import_legacy", SOURCE)
        self.assertNotIn("storage_sync", SOURCE)
        self.assertNotIn("storage_probe_slots", SOURCE)
        importer = source_block("static int storage_import_legacy(void)", "static int storage_os_missing(void)")
        self.assertLess(importer.index("storage_write_marker()"), importer.index("storage_write_retirement_marker()"))
        self.assertLess(importer.index("storage_write_retirement_marker()"), importer.index("storage_delete_path(gSlot0Path"))
        self.assertIn("ascii_case_equal", SOURCE)
        cache = SOURCE.split("/* A bounded, rebuildable metadata cache.", 1)[1].split("} FS_NODE;", 1)[0]
        self.assertNotIn("data[FS_DATA_BYTES]", cache)
        self.assertNotIn("storage_rollback", SOURCE)
        writer = source_block("static int storage_write_exact(", "static int storage_read_transaction(")
        self.assertIn("bytes == length", writer)
        self.assertIn("file->Flush(file)", writer)
        transaction_writer = source_block("static int storage_write_transaction(", "static void storage_recover_transaction(")
        self.assertLess(transaction_writer.index("gTransactionBackupPath"), transaction_writer.index("gTransactionPath"))
        replace = source_block("static int storage_replace_file(", "static int storage_delete_node(")
        self.assertLess(replace.index("storage_write_transaction(&transaction)"), replace.index("storage_rename_path(transaction.target"))
        self.assertIn("storage_rename_path(transaction.previous, transaction.target)", replace)
        recovery = source_block("static void storage_recover_transaction", "static int storage_replace_file")
        self.assertIn("if (!authoritative) failures++", recovery)
        self.assertIn("storage_read_transaction(gTransactionBackupPath", recovery)
        self.assertIn("!storage_path_exists(gTransactionNewPath)", recovery)
        self.assertIn("storage_recover_transaction();", SOURCE)
        scanner = source_block("static int storage_scan_directory(", "static int storage_scan_direct(")
        self.assertIn("!(entries[index].attribute & EFI_FILE_DIRECTORY)", scanner)
        boot = source_block("static int boot_screen(", "static void pre_os_help(")
        self.assertNotIn("fs_restore_system();", boot)
        self.assertIn("if (!mounted && !legacyFiles && factoryInstall)", boot)
        self.assertIn("fs_format();", boot)
        self.assertIn("RECOVERY REQUIRED - OPENING PRE-OS ENVIRONMENT", boot)
        restore = source_block("static int fs_restore_system(void)", "static void fs_format(void)")
        self.assertIn('fs_find_child((UINTN)runtime, "minifs2.info")', restore)
        self.assertIn('fs_find_child((UINTN)runtime, "snapshots.info")', restore)
        self.assertIn("TINYGPTFS/ROOT", README)
        self.assertIn("TINYFS.RET", README)

    def test_settings_is_full_screen_and_auto_saves(self) -> None:
        settings_ui = source_block(
            "static void settings_show(const char *notice)",
            '#include "update.inc"',
        )
        self.assertIn("Changes save automatically", settings_ui)
        self.assertIn("Return to shell", settings_ui)
        self.assertIn("Background color", settings_ui)
        self.assertIn("settings_choose_background(&gSettings.backgroundColor)", settings_ui)
        self.assertIn("background_color", SOURCE)
        self.assertIn("settings_text_attribute", settings_ui)
        self.assertIn("color == gSettings.textColor || color == gSettings.accentColor", settings_ui)
        self.assertIn("Unknown selection; choose 0 through 8.", settings_ui)
        self.assertIn("Invalid background; choose 1 through 8.", settings_ui)
        self.assertIn("gST->ConOut->ClearScreen", settings_ui)
        self.assertIn("settings_save_notice()", settings_ui)
        self.assertNotIn("Save and exit", settings_ui)
        self.assertIn("OS console background color", README)

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
        self.assertIn("nightly/TinyGPT-update.txt", UPDATE_SOURCE)
        self.assertIn("update_digest_equal(currentDigest, manifest.digest)", UPDATE_SOURCE)
        self.assertIn("TINYGPT_DISPLAY_VERSION", UPDATE_SOURCE)
        self.assertIn('TINYGPT_BUILD_CHANNEL "main"', SOURCE)

    def test_global_authentication_is_redundant_hashed_and_fail_closed(self) -> None:
        self.assertIn("gBootVolumeRoot", AUTH_SOURCE)
        self.assertIn("gAuthSlot0Path", AUTH_SOURCE)
        self.assertIn("gAuthSlot1Path", AUTH_SOURCE)
        self.assertIn("generation", AUTH_SOURCE)
        self.assertIn("AUTH_DB_CORRUPT", AUTH_SOURCE)
        self.assertIn("adminCount", AUTH_SOURCE)
        self.assertIn("database->accountCount == 0U || adminCount > 0U", AUTH_SOURCE)
        self.assertIn("AUTH_KDF_ITERATIONS 4096U", AUTH_SOURCE)
        self.assertIn("sha256_bytes", AUTH_SOURCE)
        self.assertIn("auth_constant_time_equal", AUTH_SOURCE)
        self.assertIn("gRngProtocolGuid", AUTH_SOURCE)
        self.assertNotIn("plaintext", AUTH_SOURCE.lower())

    def test_login_accounts_and_privileged_reauthentication_are_integrated(self) -> None:
        entry = SOURCE.split("EFI_STATUS EFIAPI EfiMain", 1)[1]
        dispatch = source_block("static void run_command(char *line)", "__attribute__((used))")
        self.assertIn("auth_database_load();", entry)
        self.assertIn("auth_login()", entry)
        self.assertIn('streq(command, "logout")', dispatch)
        self.assertIn("settings_accounts()", SOURCE)
        for operation in ("auth_add_account", "auth_change_password", "auth_delete_account"):
            self.assertIn(operation, ACCOUNT_SETTINGS_SOURCE)
        self.assertIn('auth_authorize_admin("update installation")', UPDATE_SOURCE)
        for action in ("partition add", "partition delete", "partition name", "repair", "reset"):
            self.assertIn(f'auth_pre_os_authorize_admin("{action}")', SOURCE)
        combined = SOURCE + UPDATE_SOURCE
        for token in ("Type UNLOCK", "Type UPDATE", "Type RESET", "DELETE PARTITION", "Type REBOOT"):
            self.assertNotIn(token, combined)
        self.assertNotIn("gProtectionUnlocked", SOURCE + EDITOR_SOURCE)
        self.assertIn("static int auth_read_username", AUTH_SOURCE)
        self.assertIn("else overflow = 1", AUTH_SOURCE)
        self.assertEqual(AUTH_SOURCE.count("auth_read_username(username)"), 3)

    def test_account_management_is_in_settings_not_shell_commands(self) -> None:
        dispatch = source_block("static void run_command(char *line)", "__attribute__((used))")
        settings_ui = source_block("static void command_settings(void)", 'static int auth_session_is_admin(void);')
        self.assertIn('streq(choice, "8")', settings_ui)
        self.assertIn("if (!settings_accounts())", settings_ui)
        self.assertIn("gScrollbackEnabled = previousScrollback", settings_ui)
        account_branch = settings_ui.split('streq(choice, "8")', 1)[1].split("} else {", 1)[0]
        self.assertIn("continue;", account_branch)
        self.assertNotIn("settings_save_notice", account_branch)
        for command in ("users", "useradd", "passwd", "userdel"):
            self.assertNotRegex(dispatch, rf'(?:streq|starts_with)\(command, "{command}(?: |")')
        for label in ("Change my password", "Add account", "Change another user's password", "Delete account", "Back to Settings"):
            self.assertIn(label, ACCOUNT_SETTINGS_SOURCE)
        self.assertIn("if (auth_session_is_admin())", ACCOUNT_SETTINGS_SOURCE)
        self.assertIn("auth_read_username(username)", ACCOUNT_SETTINGS_SOURCE)
        self.assertIn("gAuthDatabaseState != AUTH_DB_VALID", ACCOUNT_SETTINGS_SOURCE)
        self.assertIn("Settings > User accounts", README)

    def test_nightly_pipeline_keeps_main_and_beta_channels_separate(self) -> None:
        self.assertIn("branches:\n      - nightly", NIGHTLY_WORKFLOW)
        self.assertIn("gh release create nightly", NIGHTLY_WORKFLOW)
        self.assertIn("TinyGPT-nightly.img", NIGHTLY_WORKFLOW)
        self.assertIn("TinyGPT-nightly-BOOTAA64.EFI", NIGHTLY_WORKFLOW)
        self.assertNotIn("TinyGPT-v${VERSION}-nightly", NIGHTLY_WORKFLOW)
        self.assertIn("TINYGPT_BUILD_CHANNEL: nightly", NIGHTLY_WORKFLOW)
        self.assertIn('--title "TinyGPT nightly"', NIGHTLY_WORKFLOW)
        self.assertIn("--prerelease", NIGHTLY_WORKFLOW)
        self.assertIn("cancel-in-progress: false", NIGHTLY_WORKFLOW)
        self.assertNotIn("gh release delete-asset", NIGHTLY_WORKFLOW)
        self.assertNotIn("pages: write", NIGHTLY_WORKFLOW)
        self.assertIn("event_type=nightly-published", NIGHTLY_WORKFLOW)
        self.assertIn("git merge-base --is-ancestor", RELEASE_WORKFLOW)
        self.assertIn('--target "$GITHUB_SHA"', RELEASE_WORKFLOW)
        self.assertIn("event_type=release-published", RELEASE_WORKFLOW)
        self.assertIn("repository_dispatch:", PAGES_WORKFLOW)
        self.assertNotIn("workflow_run:", PAGES_WORKFLOW)
        self.assertIn("ref: main", PAGES_WORKFLOW)
        self.assertIn("TinyGPT-nightly-update.txt", PAGES_WORKFLOW)
        self.assertIn("--main release-main --nightly release-nightly", PAGES_WORKFLOW)

    def test_img_is_the_only_maintained_boot_distribution(self) -> None:
        self.assertIn(
            "tools/make_image.py build/BOOTAA64.EFI build/TinyGPT.img",
            BUILD_SCRIPT,
        )
        self.assertNotIn("make_utm_bundle.py", BUILD_SCRIPT)
        self.assertIn(
            'cp build/TinyGPT.img "dist/TinyGPT-${tag}.img"',
            RELEASE_WORKFLOW,
        )
        self.assertNotIn("UTM.utm.zip", RELEASE_WORKFLOW)
        self.assertFalse((ROOT / "tools" / "make_utm_bundle.py").exists())

    def test_readme_keeps_user_disclaimer(self) -> None:
        disclaimer = next(line for line in README.splitlines() if line)
        self.assertTrue(disclaimer.startswith("# (DISCLAIMER:"))
        self.assertIn("fully managed by ChatGPT codex", disclaimer)
        self.assertIn("8minecraft.19@gmail.com", disclaimer)


if __name__ == "__main__":
    unittest.main()
