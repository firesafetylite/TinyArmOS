from __future__ import annotations

import unittest
import zlib
from dataclasses import dataclass

ALIGNMENT = 2048
FIRST_USABLE = 2048
LAST_USABLE = 260000


@dataclass(frozen=True)
class Entry:
    start: int
    end: int
    kind: str = "tinygpt-fat"
    gpt_label: str = "DATA"
    fat_label: str = "DATA"


def validate_extents(entries: list[Entry | None]) -> None:
    used = [entry for entry in entries if entry is not None]
    for entry in used:
        if not (FIRST_USABLE <= entry.start <= entry.end <= LAST_USABLE):
            raise ValueError("extent outside usable GPT bounds")
    for index, left in enumerate(used):
        for right in used[index + 1 :]:
            if not (left.end < right.start or right.end < left.start):
                raise ValueError("overlapping GPT entries")


def first_empty_slot(entries: list[Entry | None]) -> int:
    for index in range(1, min(len(entries), 16)):
        if entries[index] is None:
            return index + 1
    raise ValueError("no available GPT slot")


def first_fit(entries: list[Entry | None], sectors: int) -> int:
    validate_extents(entries)
    cursor = ((FIRST_USABLE + ALIGNMENT - 1) // ALIGNMENT) * ALIGNMENT
    for entry in sorted((entry for entry in entries if entry), key=lambda item: item.start):
        if cursor + sectors - 1 < entry.start:
            return cursor
        cursor = ((entry.end + 1 + ALIGNMENT - 1) // ALIGNMENT) * ALIGNMENT
    if cursor + sectors - 1 <= LAST_USABLE:
        return cursor
    raise ValueError("no fitting extent")


def delete(entries: list[Entry | None], registry: list[str], partition: int) -> Entry:
    if partition == 1:
        raise ValueError("recovery is protected")
    index = partition - 1
    if index >= len(entries) or entries[index] is None or not registry[index]:
        raise ValueError("absent or unregistered")
    entry = entries[index]
    assert entry is not None
    validate_extents(entries)
    if (
        entry.kind != "tinygpt-fat"
        or entry.gpt_label != registry[index]
        or entry.fat_label != registry[index]
    ):
        raise ValueError("foreign or mismatched entry")
    entries[index] = None
    registry[index] = ""
    return entry


def mirrored_commit(table: bytes, fail_phase: str | None = None) -> tuple[bytes, bytes]:
    old = b"old-table"
    backup = old
    primary = old
    if fail_phase == "backup-write":
        return primary, backup
    backup = table
    if fail_phase == "backup-verify":
        return primary, old
    if fail_phase == "primary-write":
        return primary, backup
    primary = table
    if fail_phase == "primary-verify":
        return primary, backup
    return primary, backup


class PartitionModelTests(unittest.TestCase):
    def base_entries(self) -> list[Entry | None]:
        return [
            Entry(2048, 67583, gpt_label="TINYRECOV", fat_label="TINYRECOV"),
            Entry(67584, 131038, gpt_label="TINYGPT", fat_label="TINYGPT"),
            Entry(131072, 147455, gpt_label="THREE", fat_label="THREE"),
            Entry(147456, 163839, gpt_label="FOUR", fat_label="FOUR"),
        ]

    def test_delete_middle_then_reuses_same_aligned_extent(self) -> None:
        entries = self.base_entries()
        registry = ["TINYRECOV", "TINYGPT", "THREE", "FOUR"]
        payload = bytearray(b"recoverable former FAT payload")
        removed = delete(entries, registry, 3)
        self.assertEqual(first_fit(entries, removed.end - removed.start + 1), removed.start)
        self.assertEqual(first_empty_slot(entries), 3)
        self.assertEqual(payload, b"recoverable former FAT payload")

    def test_delete_partition_two_reuses_its_slot_and_extent(self) -> None:
        entries = self.base_entries()
        registry = ["TINYRECOV", "TINYGPT", "THREE", "FOUR"]
        removed = delete(entries, registry, 2)
        self.assertEqual(first_empty_slot(entries), 2)
        self.assertEqual(first_fit(entries, removed.end - removed.start + 1), removed.start)

    def test_first_fit_handles_unsorted_adjacent_and_gap_sizes(self) -> None:
        entries = [Entry(135168, 151551), *self.base_entries()[:2]]
        self.assertEqual(first_fit(entries, 1024), 131072)
        self.assertEqual(first_fit(entries, 2048), 131072)
        self.assertEqual(first_fit(entries, 4096), 131072)
        self.assertEqual(first_fit(entries, 4097), 151552)
        adjacent = [Entry(2048, 4095), Entry(4096, 6143)]
        self.assertEqual(first_fit(adjacent, 2048), 6144)
        spanning_alignment = [Entry(2048, 6143), Entry(7000, 8191)]
        self.assertEqual(first_fit(spanning_alignment, 2048), 8192)

    def test_no_gap_and_malformed_overlap_are_rejected(self) -> None:
        with self.assertRaises(ValueError):
            first_fit([Entry(FIRST_USABLE, LAST_USABLE)], 2048)
        with self.assertRaises(ValueError):
            first_fit([Entry(2048, 8191), Entry(4096, 12287)], 2048)

    def test_delete_rejects_recovery_foreign_stale_and_label_mismatch(self) -> None:
        registry = ["TINYRECOV", "TINYGPT", "THREE"]
        for partition, entries, names in (
            (1, self.base_entries(), registry.copy()),
            (3, [*self.base_entries()[:2], Entry(133120, 149503, kind="foreign")], registry.copy()),
            (3, [*self.base_entries()[:2], None], registry.copy()),
            (3, [*self.base_entries()[:2], Entry(133120, 149503, fat_label="OTHER")], registry.copy()),
        ):
            with self.subTest(partition=partition, entries=entries):
                with self.assertRaises(ValueError):
                    delete(entries, names, partition)

    def test_full_allowed_slots_leave_no_number_even_when_space_exists(self) -> None:
        entries: list[Entry | None] = [Entry(2048, 4095)] * 16
        with self.assertRaises(ValueError):
            first_empty_slot(entries)

    def test_mirrored_commit_success_has_equal_crcs_and_failures_are_classifiable(self) -> None:
        table = b"\0" * 128 + b"surviving-entry"
        primary, backup = mirrored_commit(table)
        self.assertEqual(primary, backup)
        self.assertEqual(zlib.crc32(primary), zlib.crc32(backup))
        self.assertEqual(mirrored_commit(table, "backup-write"), (b"old-table", b"old-table"))
        self.assertEqual(mirrored_commit(table, "backup-verify"), (b"old-table", b"old-table"))
        primary, backup = mirrored_commit(table, "primary-write")
        self.assertNotEqual(primary, backup)
        self.assertEqual(backup, table)
        with self.assertRaises(ValueError):
            if primary != backup:
                raise ValueError("divergent GPT mirrors")

    def test_registry_failure_leaves_committed_delete_distinguishable(self) -> None:
        entries = self.base_entries()
        persisted_registry = ["TINYRECOV", "TINYGPT", "THREE", "FOUR"]
        memory_registry = persisted_registry.copy()
        delete(entries, memory_registry, 3)
        registry_flush_succeeded = False
        self.assertIsNone(entries[2])
        self.assertEqual(memory_registry[2], "")
        self.assertEqual(persisted_registry[2], "THREE")
        self.assertFalse(registry_flush_succeeded)


if __name__ == "__main__":
    unittest.main()
