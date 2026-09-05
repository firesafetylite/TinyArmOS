/* Execute the production auth code against a small fake UEFI filesystem/console.
 * No VM disks, host credentials, or real firmware are accessed by this test. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef unsigned long long UINT64;
typedef unsigned long long UINTN;
typedef UINT16 CHAR16;
typedef UINT64 EFI_STATUS;
typedef struct { UINT32 Data1; UINT16 Data2, Data3; UINT8 Data4[8]; } EFI_GUID;
typedef struct { UINT16 ScanCode; CHAR16 UnicodeChar; } EFI_INPUT_KEY;
#define EFIAPI
#define EFI_SUCCESS 0ULL
#define EFI_NOT_FOUND 14ULL
#define EFI_NOT_READY 6ULL
#define EFI_DEVICE_ERROR 7ULL
#define EFI_ACCESS_DENIED 15ULL
#define EFI_FILE_MODE_READ 1ULL
#define EFI_FILE_MODE_WRITE 2ULL
#define EFI_FILE_MODE_CREATE (1ULL << 63)

typedef struct EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;
struct EFI_FILE_PROTOCOL {
    EFI_STATUS (*Open)(EFI_FILE_PROTOCOL *, EFI_FILE_PROTOCOL **, CHAR16 *, UINT64, UINT64);
    EFI_STATUS (*Read)(EFI_FILE_PROTOCOL *, UINTN *, void *);
    EFI_STATUS (*Write)(EFI_FILE_PROTOCOL *, UINTN *, void *);
    EFI_STATUS (*SetPosition)(EFI_FILE_PROTOCOL *, UINT64);
    EFI_STATUS (*Flush)(EFI_FILE_PROTOCOL *);
    EFI_STATUS (*Close)(EFI_FILE_PROTOCOL *);
    EFI_STATUS (*Delete)(EFI_FILE_PROTOCOL *);
    int slot;
    UINTN position;
};
typedef struct FakeInput FakeInput;
struct FakeInput { EFI_STATUS (*ReadKeyStroke)(FakeInput *, EFI_INPUT_KEY *); };
typedef struct {
    EFI_STATUS (*LocateProtocol)(EFI_GUID *, void *, void **);
    EFI_STATUS (*Stall)(UINTN);
} FakeBootServices;
typedef struct FakeOutput FakeOutput;
struct FakeOutput { EFI_STATUS (*ClearScreen)(FakeOutput *); };
typedef struct { FakeBootServices *BootServices; FakeInput *ConIn; FakeOutput *ConOut; } FakeSystemTable;
static FakeSystemTable *gST;
static EFI_FILE_PROTOCOL *gBootVolumeRoot;
static void memory_zero(void *ptr, UINTN size) { memset(ptr, 0, (size_t)size); }
static void memory_copy(void *dst, const void *src, UINTN size) { memcpy(dst, src, (size_t)size); }
static void secure_zero(void *ptr, UINTN size) {
    volatile UINT8 *out = ptr;
    while (size--) *out++ = 0;
}
static UINTN string_length(const char *value) { return (UINTN)strlen(value); }
static void string_copy(char *dst, const char *src, UINTN size) {
    snprintf(dst, (size_t)size, "%s", src);
}
static int streq(const char *a, const char *b) { return strcmp(a, b) == 0; }
static char output[32768];
static void print(const char *text) {
    assert(strlen(output) + strlen(text) < sizeof(output));
    strcat(output, text);
}
static void print_char(char ch) { char text[] = { ch, 0 }; print(text); }
static void delay_ms(UINT64 ms) { (void)ms; }
static UINT64 timer_count(void) { static UINT64 ticks; return ++ticks; }
static void read_line(char *line, UINTN capacity) {
    UINTN used = 0;
    EFI_INPUT_KEY key;
    for (;;) {
        assert(gST->ConIn->ReadKeyStroke(gST->ConIn, &key) == EFI_SUCCESS);
        if (key.UnicodeChar == '\n' || key.UnicodeChar == '\r') break;
        if (used + 1U < capacity) line[used++] = (char)key.UnicodeChar;
    }
    line[used] = 0;
}
static void settings_use_default_color(void) {}
static void settings_use_accent_color(void) {}
#include "../src/sha256.inc"
#include "../src/auth.inc"
#include "../src/account_settings.inc"

typedef struct {
    UINT8 bytes[sizeof(AUTH_DATABASE) + 1];
    UINTN length;
    int exists;
    EFI_STATUS openError;
    EFI_STATUS readError;
    int shortWrite;
    int flushFailure;
} FakeSlot;
static FakeSlot slots[2];
static EFI_FILE_PROTOCOL root, handles[2];
static const char *input;
static int rngAvailable;
static int writeCount;

static EFI_STATUS fake_close(EFI_FILE_PROTOCOL *file) { (void)file; return EFI_SUCCESS; }
static EFI_STATUS fake_delete(EFI_FILE_PROTOCOL *file) {
    slots[file->slot].exists = 0;
    slots[file->slot].length = 0;
    return EFI_SUCCESS;
}
static EFI_STATUS fake_position(EFI_FILE_PROTOCOL *file, UINT64 position) {
    file->position = position;
    return EFI_SUCCESS;
}
static EFI_STATUS fake_flush(EFI_FILE_PROTOCOL *file) {
    return slots[file->slot].flushFailure ? EFI_DEVICE_ERROR : EFI_SUCCESS;
}
static EFI_STATUS fake_read(EFI_FILE_PROTOCOL *file, UINTN *bytes, void *data) {
    FakeSlot *slot = &slots[file->slot];
    if (slot->readError) return slot->readError;
    UINTN remaining = slot->length > file->position ? slot->length - file->position : 0;
    if (*bytes > remaining) *bytes = remaining;
    memcpy(data, slot->bytes + file->position, (size_t)*bytes);
    file->position += *bytes;
    return EFI_SUCCESS;
}
static EFI_STATUS fake_write(EFI_FILE_PROTOCOL *file, UINTN *bytes, void *data) {
    FakeSlot *slot = &slots[file->slot];
    if (slot->shortWrite && *bytes) (*bytes)--;
    assert(file->position + *bytes <= sizeof(slot->bytes));
    memcpy(slot->bytes + file->position, data, (size_t)*bytes);
    file->position += *bytes;
    if (slot->length < file->position) slot->length = file->position;
    writeCount++;
    return EFI_SUCCESS;
}
static EFI_STATUS fake_open(EFI_FILE_PROTOCOL *directory, EFI_FILE_PROTOCOL **file,
                            CHAR16 *path, UINT64 mode, UINT64 attributes) {
    (void)directory; (void)attributes;
    int index = path[9] == '0' ? 0 : 1;
    assert(path[9] == '0' || path[9] == '1');
    if (slots[index].openError) return slots[index].openError;
    if (!slots[index].exists && !(mode & EFI_FILE_MODE_CREATE)) return EFI_NOT_FOUND;
    slots[index].exists = 1;
    handles[index].position = 0;
    *file = &handles[index];
    return EFI_SUCCESS;
}
static EFI_STATUS fake_rng(EFI_RNG_PROTOCOL *rng, EFI_GUID *algorithm, UINTN bytes, UINT8 *out) {
    (void)rng; (void)algorithm;
    static UINT8 value;
    memset(out, ++value, (size_t)bytes);
    return EFI_SUCCESS;
}
static EFI_STATUS fake_locate(EFI_GUID *guid, void *registration, void **protocol) {
    (void)guid; (void)registration;
    static EFI_RNG_PROTOCOL rng = { NULL, fake_rng };
    if (!rngAvailable) return EFI_NOT_FOUND;
    *protocol = &rng;
    return EFI_SUCCESS;
}
static EFI_STATUS fake_stall(UINTN microseconds) { (void)microseconds; return EFI_SUCCESS; }
static EFI_STATUS fake_key(FakeInput *console, EFI_INPUT_KEY *key) {
    (void)console;
    assert(input && *input); /* Missing input indicates an unexpected extra prompt. */
    key->ScanCode = 0;
    key->UnicodeChar = (UINT8)*input++;
    return EFI_SUCCESS;
}
static EFI_STATUS fake_clear(FakeOutput *console) { (void)console; return EFI_SUCCESS; }
static void reset_fixture(void) {
    static FakeBootServices boot = { fake_locate, fake_stall };
    static FakeInput console = { fake_key };
    static FakeOutput display = { fake_clear };
    static FakeSystemTable table = { &boot, &console, &display };
    memset(slots, 0, sizeof(slots));
    memset(&gAuthDatabase, 0, sizeof(gAuthDatabase));
    memset(output, 0, sizeof(output));
    gAuthDatabaseState = AUTH_DB_ABSENT;
    gCurrentAccount = -1;
    gAuthFallbackCounter = 0;
    gAuthStorageFailed = 0;
    input = NULL;
    rngAvailable = 0;
    writeCount = 0;
    gST = &table;
    root.Open = fake_open;
    gBootVolumeRoot = &root;
    for (int index = 0; index < 2; index++) {
        handles[index] = (EFI_FILE_PROTOCOL){ fake_open, fake_read, fake_write,
            fake_position, fake_flush, fake_close, fake_delete, index, 0 };
    }
}
static void setup(void) {
    assert(auth_database_load() == AUTH_DB_ABSENT);
    input = "Admin\ncorrect horse\ncorrect horse\n";
    assert(auth_login());
    assert(!*input);
    assert(auth_session_is_admin());
    assert(gAuthDatabase.accountCount == 1);
    assert(slots[0].exists && slots[1].exists);
    assert(!strstr(output, "correct horse"));
}
static void test_setup_login_roles(void) {
    reset_fixture();
    setup(); /* Includes RNG-unavailable salt fallback. */
    assert(auth_database_load() == AUTH_DB_VALID);
    input = "ADMIN\ncorrect horse\n";
    assert(auth_login());
    input = "correct horse\nstandard pass\nstandard pass\n";
    auth_add_account("guest", AUTH_ROLE_STANDARD);
    assert(gAuthDatabase.accountCount == 2);
    int guest = auth_find_account("guest");
    assert(guest >= 0);
    assert(!auth_constant_time_equal(gAuthDatabase.accounts[0].salt,
                                    gAuthDatabase.accounts[guest].salt, AUTH_SALT_BYTES));
    assert(auth_database_load() == AUTH_DB_VALID);
    input = "guest\nstandard pass\n";
    assert(auth_login());
    assert(!auth_session_is_admin());
    assert(!auth_authorize_admin("root wipe"));
    assert(!*input); /* Standard user is denied without a privilege escalation prompt. */
    input = "standard pass\nchanged pass\nchanged pass\n";
    auth_change_password(NULL);
    assert(auth_verify_password(&gAuthDatabase.accounts[guest], "changed pass"));
    assert(!auth_verify_password(&gAuthDatabase.accounts[guest], "standard pass"));
    input = "Admin\nwrong password\n";
    assert(!auth_login());
    assert(!auth_session_is_admin());
    input = "unknown\ncorrect horse\n";
    assert(!auth_login());
    assert(gAuthDatabase.accountCount == 2);
    input = "guest\nchanged pass\n";
    assert(!auth_pre_os_authorize_admin("reset"));
    input = "Admin\ncorrect horse\n";
    assert(auth_pre_os_authorize_admin("reset"));
    assert(gCurrentAccount == -1);
}
static void test_rng_and_invariants(void) {
    reset_fixture(); rngAvailable = 1; setup();
    UINT8 oldSalt[AUTH_SALT_BYTES];
    memcpy(oldSalt, gAuthDatabase.accounts[0].salt, sizeof(oldSalt));
    assert(auth_set_password(&gAuthDatabase.accounts[0], "new password"));
    assert(memcmp(oldSalt, gAuthDatabase.accounts[0].salt, sizeof(oldSalt)));
    assert(auth_verify_password(&gAuthDatabase.accounts[0], "new password"));
    AUTH_DATABASE noAdmin = gAuthDatabase;
    UINT8 digest[AUTH_HASH_BYTES];
    noAdmin.accounts[0].role = AUTH_ROLE_STANDARD;
    auth_database_checksum(&noAdmin, digest);
    memcpy(noAdmin.checksum, digest, sizeof(digest));
    assert(!auth_database_valid(&noAdmin));
    auth_delete_account("Admin");
    assert(gAuthDatabase.accountCount == 1); /* Active/last administrator remains. */
}
static void test_slot_errors_and_durability(void) {
    reset_fixture();
    slots[0].openError = EFI_ACCESS_DENIED;
    assert(auth_database_load() == AUTH_DB_CORRUPT);
    assert(!auth_setup_first_admin(1));
    assert(!writeCount);
    reset_fixture();
    gBootVolumeRoot = NULL;
    assert(auth_database_load() == AUTH_DB_CORRUPT);
    reset_fixture(); setup();
    slots[0].readError = slots[1].readError = EFI_DEVICE_ERROR;
    assert(auth_database_load() == AUTH_DB_CORRUPT);
    reset_fixture(); setup();
    slots[0].bytes[0] ^= 1;
    assert(auth_database_load() == AUTH_DB_VALID); /* Intact backup is usable. */
    slots[1].bytes[0] ^= 1;
    assert(auth_database_load() == AUTH_DB_CORRUPT);
    assert(!auth_login());
    reset_fixture(); setup();
    slots[0].length++;
    slots[1].length++;
    assert(auth_database_load() == AUTH_DB_CORRUPT); /* Trailing junk is not a DB. */
    reset_fixture(); setup();
    slots[0].length++;
    assert(auth_database_load() == AUTH_DB_VALID);
    assert(auth_database_save()); /* Replaces, rather than retaining trailing junk. */
    assert(slots[0].length == sizeof(AUTH_DATABASE));
    reset_fixture(); setup();
    slots[0].shortWrite = slots[1].shortWrite = 1;
    assert(!auth_database_save());
    assert(gAuthDatabaseState == AUTH_DB_CORRUPT);
    reset_fixture(); setup();
    slots[0].flushFailure = slots[1].flushFailure = 1;
    assert(!auth_database_save()); /* Reading cached bytes does not excuse flush failure. */
    assert(gAuthDatabaseState == AUTH_DB_CORRUPT);
    assert(gCurrentAccount == -1);
    assert(auth_database_load() == AUTH_DB_CORRUPT); /* Pre-OS cannot clear the write-fault latch. */
}
static void test_bounded_input(void) {
    char password[AUTH_PASSWORD_BYTES], username[AUTH_USERNAME_BYTES];
    reset_fixture();
    input = "ab\bcd\n";
    auth_read_password("", password);
    assert(strcmp(password, "acd") == 0);
    assert(!strstr(output, "acd"));
    input = "12345678901234567890123456789012345678901234567890123456789012345\n";
    auth_read_password("", password);
    assert(password[0] == 0);
    input = "123456789012345678901234\n";
    assert(!auth_read_username(username));
}
static void test_account_settings_admin(void) {
    reset_fixture(); setup();
    input = "2\nguest\n1\ncorrect horse\nstandard pass\nstandard pass\n\n0\n";
    assert(settings_accounts() && !*input);
    assert(gAuthDatabase.accountCount == 2);
    int guest = auth_find_account("guest");
    assert(guest >= 0 && gAuthDatabase.accounts[guest].role == AUTH_ROLE_STANDARD);
    assert(strstr(output, "Account added."));
    assert(!strstr(output, "standard pass") && !strstr(output, "correct horse"));
    input = "3\nguest\ncorrect horse\nchanged pass\nchanged pass\n\n0\n";
    assert(settings_accounts() && !*input);
    assert(auth_verify_password(&gAuthDatabase.accounts[guest], "changed pass"));
    input = "2\nbackup\n2\ncorrect horse\nbackup pass\nbackup pass\n\n0\n";
    assert(settings_accounts() && !*input);
    assert(gAuthDatabase.accounts[auth_find_account("backup")].role == AUTH_ROLE_ADMIN);
    UINT64 generation = gAuthDatabase.generation;
    input = "4\nAdmin\n\n0\n";
    assert(settings_accounts() && !*input);
    assert(gAuthDatabase.generation == generation);
    assert(strstr(output, "Cannot delete the active account."));
    input = "4\nguest\ncorrect horse\n\n0\n";
    assert(settings_accounts() && !*input);
    assert(auth_find_account("guest") == -1);
    assert(auth_database_load() == AUTH_DB_VALID);
    assert(auth_find_account("guest") == -1);
    assert(gAuthDatabase.accountCount == 2);
}
static void test_account_settings_standard(void) {
    reset_fixture(); setup();
    input = "correct horse\nstandard pass\nstandard pass\n";
    auth_add_account("guest", AUTH_ROLE_STANDARD);
    input = "guest\nstandard pass\n";
    assert(auth_login());
    memset(output, 0, sizeof(output));
    UINT64 generation = gAuthDatabase.generation;
    input = "2\n3\n4\n0\n";
    assert(settings_accounts() && !*input);
    assert(gAuthDatabase.generation == generation);
    assert(!strstr(output, "  2  Add account"));
    assert(!strstr(output, "  3  Change another"));
    assert(!strstr(output, "  4  Delete account"));
    auth_add_account("unwanted", AUTH_ROLE_ADMIN);
    auth_change_password("Admin");
    assert(gAuthDatabase.generation == generation && !*input);
    input = "1\nstandard pass\nchanged pass\nchanged pass\n\n0\n";
    assert(settings_accounts() && !*input);
    assert(auth_verify_password(&gAuthDatabase.accounts[gCurrentAccount], "changed pass"));
    assert(!strstr(output, "standard pass") && !strstr(output, "changed pass"));
}
static void test_account_settings_cancel_errors_and_storage_failure(void) {
    reset_fixture(); setup();
    UINT64 generation = gAuthDatabase.generation;
    input = "2\n\n\n0\n"; /* Blank username cancels. */
    assert(settings_accounts() && !*input);
    input = "2\nspare\n0\n\n0\n"; /* Cancel role selection. */
    assert(settings_accounts() && !*input);
    input = "2\nspare\n3\n\n0\n"; /* Invalid role cannot default to administrator. */
    assert(settings_accounts() && !*input);
    input = "2\n123456789012345678901234\n\n0\n";
    assert(settings_accounts() && !*input);
    input = "1\nwrong pass\n\n0\n";
    assert(settings_accounts() && !*input);
    input = "1\ncorrect horse\nnew password\nnot matching\n\n0\n";
    assert(settings_accounts() && !*input);
    assert(gAuthDatabase.generation == generation);
    assert(auth_verify_password(&gAuthDatabase.accounts[0], "correct horse"));
    slots[0].flushFailure = slots[1].flushFailure = 1;
    input = "1\ncorrect horse\nnew password\nnew password\n";
    assert(!settings_accounts() && !*input); /* Revocation exits without another menu prompt. */
    assert(gCurrentAccount == -1 && gAuthDatabaseState == AUTH_DB_CORRUPT);
    assert(strstr(output, "Password was not saved."));
}
int main(int argc, char **argv) {
    if (argc == 2) {
        UINT8 digest[32], salt[16];
        for (UINTN index = 0; index < sizeof(salt); index++) salt[index] = (UINT8)index;
        auth_password_hash(argv[1], salt, digest);
        for (UINTN index = 0; index < sizeof(digest); index++) printf("%02x", digest[index]);
        puts("");
        return 0;
    }
    test_setup_login_roles();
    test_rng_and_invariants();
    test_slot_errors_and_durability();
    test_bounded_input();
    test_account_settings_admin();
    test_account_settings_standard();
    test_account_settings_cancel_errors_and_storage_failure();
    puts("production authentication tests passed");
    return 0;
}
