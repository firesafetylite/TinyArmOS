from __future__ import annotations

import unittest
from dataclasses import dataclass
from pathlib import Path

SOURCE = (Path(__file__).resolve().parents[1] / "src" / "uefi.c").read_text(
    encoding="utf-8"
)


@dataclass(frozen=True)
class Transaction:
    operation: str
    target: str
    previous: str = ""
    temporary: str = ""


class FakeDirectStorage:
    """Executable crash-state model for the direct FAT transaction contract."""

    def __init__(self) -> None:
        self.files: dict[str, bytes] = {}
        self.format_valid = False
        self.legacy_retired = False
        self.legacy_payload: dict[str, bytes] | None = None
        self.primary: Transaction | None | str = None
        self.backup: Transaction | None | str = None

    def mount_authority(self) -> str:
        if self.format_valid:
            return "direct"
        if self.legacy_retired:
            return "recovery"
        return "legacy" if self.legacy_payload is not None else "missing"

    def recover(self) -> bool:
        committed = self.primary is not None
        if not committed:
            self.files.pop("TXN.NEW", None)
            self.backup = None
            return True
        transaction = self.primary if isinstance(self.primary, Transaction) else None
        if transaction is None and isinstance(self.backup, Transaction):
            transaction = self.backup
        if transaction is None:
            if "TXN.NEW" not in self.files and "TXN.PREV" not in self.files:
                self.primary = None
                self.backup = None
                return True
            return False
        if transaction.operation == "replace":
            if transaction.target not in self.files:
                source = "TXN.NEW" if "TXN.NEW" in self.files else "TXN.PREV"
                if source not in self.files:
                    return False
                self.files[transaction.target] = self.files.pop(source)
            self.files.pop("TXN.NEW", None)
            self.files.pop("TXN.PREV", None)
        elif transaction.operation == "delete":
            self.files.pop("TXN.PREV", None)
        elif transaction.operation == "rename":
            if transaction.target not in self.files:
                if transaction.previous not in self.files:
                    return False
                self.files[transaction.target] = self.files.pop(transaction.previous)
        else:
            return False
        self.primary = None
        self.backup = None
        return True

    def replace(self, target: str, payload: bytes, fail_before_commit: bool = False) -> bool:
        self.files["TXN.NEW"] = payload
        transaction = Transaction("replace", target, "TXN.PREV", "TXN.NEW")
        self.backup = transaction
        if fail_before_commit:
            return False
        self.primary = transaction
        if target in self.files:
            self.files["TXN.PREV"] = self.files.pop(target)
        return self.recover()


class DirectFilesystemModelTests(unittest.TestCase):
    def test_valid_direct_marker_wins_over_stale_legacy_slots(self) -> None:
        storage = FakeDirectStorage()
        storage.format_valid = True
        storage.legacy_retired = True
        storage.legacy_payload = {"/home/old.txt": b"stale"}
        storage.files["/home/new.txt"] = b"direct"
        self.assertEqual(storage.mount_authority(), "direct")
        self.assertIn("gLegacyRetiredPath", SOURCE)

    def test_corrupt_direct_marker_cannot_reactivate_retired_legacy(self) -> None:
        storage = FakeDirectStorage()
        storage.format_valid = False
        storage.legacy_retired = True
        storage.legacy_payload = {"/home/old.txt": b"stale"}
        self.assertEqual(storage.mount_authority(), "recovery")
        self.assertIn("if (storage_retirement_valid()) return 0;", SOURCE)

    def test_torn_primary_manifest_recovers_from_redundant_manifest(self) -> None:
        storage = FakeDirectStorage()
        storage.files["/home/note.txt"] = b"old"
        transaction = Transaction("replace", "/home/note.txt", "TXN.PREV", "TXN.NEW")
        storage.files["TXN.NEW"] = b"new"
        storage.files["TXN.PREV"] = storage.files.pop("/home/note.txt")
        storage.primary = "corrupt"
        storage.backup = transaction
        self.assertTrue(storage.recover())
        self.assertEqual(storage.files["/home/note.txt"], b"new")
        self.assertIsNone(storage.primary)
        self.assertIsNone(storage.backup)

    def test_corrupt_manifest_without_moved_payload_is_discarded(self) -> None:
        storage = FakeDirectStorage()
        storage.primary = "corrupt"
        storage.backup = "corrupt"
        self.assertTrue(storage.recover())
        self.assertIsNone(storage.primary)
        self.assertIsNone(storage.backup)
        self.assertIn("!storage_path_exists(gTransactionNewPath)", SOURCE)

    def test_interrupted_delete_and_rename_are_completed(self) -> None:
        delete = FakeDirectStorage()
        delete.files["TXN.PREV"] = b"removed payload"
        transaction = Transaction("delete", "/home/remove.txt", "TXN.PREV")
        delete.primary = delete.backup = transaction
        self.assertTrue(delete.recover())
        self.assertNotIn("TXN.PREV", delete.files)

        rename = FakeDirectStorage()
        rename.files["/home/old.txt"] = b"payload"
        transaction = Transaction("rename", "/home/new.txt", "/home/old.txt")
        rename.primary = rename.backup = transaction
        self.assertTrue(rename.recover())
        self.assertEqual(rename.files["/home/new.txt"], b"payload")
        self.assertNotIn("/home/old.txt", rename.files)

    def test_settings_round_trip_and_failed_editor_save_do_not_claim_persistence(self) -> None:
        storage = FakeDirectStorage()
        settings = b"text_color=7\nscrollback=1\n"
        self.assertTrue(storage.replace("/home/.tinygptrc", settings))
        self.assertEqual(storage.files["/home/.tinygptrc"], settings)

        storage.files["/home/note.txt"] = b"old"
        self.assertFalse(storage.replace("/home/note.txt", b"editor buffer", fail_before_commit=True))
        self.assertEqual(storage.files["/home/note.txt"], b"old")
        self.assertIn("persistence not claimed", (Path(__file__).resolve().parents[1] / "src" / "editor.inc").read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
