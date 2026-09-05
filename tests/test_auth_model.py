from __future__ import annotations

import hashlib
import unittest
from dataclasses import dataclass

ABSENT, VALID, CORRUPT = range(3)
STANDARD, ADMIN = range(2)
ITERATIONS = 4096


def password_hash(password: str, salt: bytes) -> bytes:
    raw = password.encode("ascii")
    digest = hashlib.sha256(salt + raw).digest()
    for _ in range(1, ITERATIONS):
        digest = hashlib.sha256(digest + salt + raw).digest()
    return digest


@dataclass(frozen=True)
class Slot:
    generation: int
    accounts: tuple[tuple[str, int, bytes, bytes], ...]
    valid_checksum: bool = True


def slot_valid(slot: Slot | None) -> bool:
    return bool(
        slot
        and slot.valid_checksum
        and (not slot.accounts or any(role == ADMIN for _, role, _, _ in slot.accounts))
    )


def select_slots(first: Slot | None, second: Slot | None) -> tuple[int, Slot | None]:
    valid = [slot for slot in (first, second) if slot_valid(slot)]
    if valid:
        return VALID, max(valid, key=lambda slot: slot.generation)
    return (CORRUPT if first is not None or second is not None else ABSENT), None


def setup_allowed(state: int, selected: Slot | None) -> bool:
    return state == ABSENT or (state == VALID and selected is not None and not selected.accounts)


def can_delete(accounts: list[tuple[str, int]], target: str, active: str) -> bool:
    found = next((account for account in accounts if account[0].lower() == target.lower()), None)
    if found is None or found[0].lower() == active.lower():
        return False
    return found[1] != ADMIN or sum(role == ADMIN for _, role in accounts) > 1


class AuthenticationModelTests(unittest.TestCase):
    def test_slot_selection_prefers_newest_valid_generation(self) -> None:
        older = Slot(4, (("root", ADMIN, b"s", b"h"),))
        newer = Slot(5, (("root", ADMIN, b"s", b"h"),))
        self.assertEqual(select_slots(older, newer), (VALID, newer))
        self.assertEqual(select_slots(newer, Slot(9, (), False)), (VALID, newer))

    def test_absent_and_corrupt_are_distinct_and_only_empty_can_setup(self) -> None:
        state, selected = select_slots(None, None)
        self.assertEqual(state, ABSENT)
        self.assertTrue(setup_allowed(state, selected))
        state, selected = select_slots(Slot(1, (), False), None)
        self.assertEqual(state, CORRUPT)
        self.assertFalse(setup_allowed(state, selected))
        empty = Slot(2, ())
        self.assertTrue(setup_allowed(*select_slots(empty, None)))

    def test_nonempty_database_without_an_administrator_is_corrupt(self) -> None:
        all_standard = Slot(3, (("user", STANDARD, b"s", b"h"),))
        state, selected = select_slots(all_standard, None)
        self.assertEqual(state, CORRUPT)
        self.assertIsNone(selected)
        self.assertFalse(setup_allowed(state, selected))

    def test_password_hash_is_salted_iterated_and_role_is_separate(self) -> None:
        first = password_hash("correct horse", bytes(range(16)))
        second = password_hash("correct horse", bytes(range(1, 17)))
        self.assertNotEqual(first, second)
        self.assertEqual(first, password_hash("correct horse", bytes(range(16))))
        self.assertNotEqual(first, password_hash("wrong horse", bytes(range(16))))
        self.assertNotEqual(STANDARD, ADMIN)

    def test_last_admin_and_active_account_cannot_be_deleted(self) -> None:
        accounts = [("Admin", ADMIN), ("user", STANDARD)]
        self.assertFalse(can_delete(accounts, "admin", "user"))
        self.assertFalse(can_delete(accounts, "USER", "user"))
        accounts.append(("backup", ADMIN))
        self.assertTrue(can_delete(accounts, "Admin", "user"))

    def test_unknown_username_never_enables_setup_with_valid_accounts(self) -> None:
        database = Slot(1, (("admin", ADMIN, b"s", b"h"),))
        state, selected = select_slots(database, None)
        self.assertFalse(setup_allowed(state, selected))


if __name__ == "__main__":
    unittest.main()
