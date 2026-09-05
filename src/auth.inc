/* Global recovery-volume authentication database and bounded account operations. */
#define AUTH_MAX_ACCOUNTS 8U
#define AUTH_USERNAME_BYTES 24U
#define AUTH_PASSWORD_BYTES 65U
#define AUTH_SALT_BYTES 16U
#define AUTH_HASH_BYTES 32U
#define AUTH_KDF_ITERATIONS 4096U
#define AUTH_DB_MAGIC 0x3148545541474e54ULL
#define AUTH_DB_VERSION 1U
#define AUTH_ROLE_STANDARD 0U
#define AUTH_ROLE_ADMIN 1U

typedef enum {
    AUTH_DB_ABSENT = 0,
    AUTH_DB_VALID = 1,
    AUTH_DB_CORRUPT = 2
} AUTH_DB_STATE;

typedef struct {
    UINT8 used;
    UINT8 role;
    UINT8 reserved[6];
    char username[AUTH_USERNAME_BYTES];
    UINT8 salt[AUTH_SALT_BYTES];
    UINT8 passwordHash[AUTH_HASH_BYTES];
} AUTH_ACCOUNT;

typedef struct {
    UINT64 magic;
    UINT32 version;
    UINT32 accountCount;
    UINT64 generation;
    AUTH_ACCOUNT accounts[AUTH_MAX_ACCOUNTS];
    UINT8 checksum[AUTH_HASH_BYTES];
} AUTH_DATABASE;

typedef struct EFI_RNG_PROTOCOL EFI_RNG_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_RNG_GET_INFO)(EFI_RNG_PROTOCOL *, UINTN *, EFI_GUID *);
typedef EFI_STATUS (EFIAPI *EFI_RNG_GET_RNG)(EFI_RNG_PROTOCOL *, EFI_GUID *, UINTN, UINT8 *);
struct EFI_RNG_PROTOCOL {
    EFI_RNG_GET_INFO GetInfo;
    EFI_RNG_GET_RNG GetRNG;
};

static const CHAR16 gAuthSlot0Path[] = {'\\','T','I','N','Y','A','U','T','H','0','.','D','A','T',0};
static const CHAR16 gAuthSlot1Path[] = {'\\','T','I','N','Y','A','U','T','H','1','.','D','A','T',0};
static EFI_GUID gRngProtocolGuid = {0x3152bca5,0xeade,0x433d,{0x86,0x2e,0xc0,0x1c,0xdc,0x29,0x1f,0x44}};
static AUTH_DATABASE gAuthDatabase;
static AUTH_DB_STATE gAuthDatabaseState;
static int gCurrentAccount = -1;
static UINT64 gAuthFallbackCounter;
static int gAuthStorageFailed;

static char auth_ascii_lower(char value) {
    return value >= 'A' && value <= 'Z' ? (char)(value + ('a' - 'A')) : value;
}

static int auth_username_equal(const char *left, const char *right) {
    while (*left && *right && auth_ascii_lower(*left) == auth_ascii_lower(*right)) {
        left++;
        right++;
    }
    return auth_ascii_lower(*left) == auth_ascii_lower(*right);
}

static int auth_username_valid(const char *username) {
    UINTN length = 0;
    while (username[length]) {
        char ch = username[length];
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') || ch == '_' || ch == '-')) return 0;
        length++;
        if (length >= AUTH_USERNAME_BYTES) return 0;
    }
    return length >= 1U;
}

static int auth_password_valid(const char *password) {
    UINTN length = string_length(password);
    return length >= 8U && length < AUTH_PASSWORD_BYTES;
}

static void auth_database_checksum(AUTH_DATABASE *database, UINT8 digest[AUTH_HASH_BYTES]) {
    UINT8 saved[AUTH_HASH_BYTES];
    memory_copy(saved, database->checksum, sizeof(saved));
    memory_zero(database->checksum, sizeof(database->checksum));
    sha256_bytes((const UINT8 *)database, sizeof(*database), digest);
    memory_copy(database->checksum, saved, sizeof(saved));
    memory_zero(saved, sizeof(saved));
}

static int auth_constant_time_equal(const UINT8 *left, const UINT8 *right, UINTN bytes) {
    volatile UINT8 difference = 0;
    UINTN index;
    for (index = 0; index < bytes; index++) difference |= (UINT8)(left[index] ^ right[index]);
    return difference == 0;
}

static int auth_database_valid(AUTH_DATABASE *database) {
    UINT8 digest[AUTH_HASH_BYTES];
    UINTN index;
    UINTN count = 0;
    UINTN adminCount = 0;
    if (database->magic != AUTH_DB_MAGIC || database->version != AUTH_DB_VERSION ||
        database->accountCount > AUTH_MAX_ACCOUNTS) return 0;
    auth_database_checksum(database, digest);
    if (!auth_constant_time_equal(digest, database->checksum, sizeof(digest))) {
        memory_zero(digest, sizeof(digest));
        return 0;
    }
    memory_zero(digest, sizeof(digest));
    for (index = 0; index < AUTH_MAX_ACCOUNTS; index++) {
        UINTN other;
        AUTH_ACCOUNT *account = &database->accounts[index];
        if (!account->used) continue;
        count++;
        if (account->role > AUTH_ROLE_ADMIN || !auth_username_valid(account->username)) return 0;
        if (account->role == AUTH_ROLE_ADMIN) adminCount++;
        for (other = 0; other < index; other++)
            if (database->accounts[other].used &&
                auth_username_equal(account->username, database->accounts[other].username)) return 0;
    }
    return count == database->accountCount &&
        (database->accountCount == 0U || adminCount > 0U);
}

static int auth_slot_read(const CHAR16 *path, AUTH_DATABASE *database, int *exists) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    UINTN bytes = sizeof(*database);
    EFI_STATUS status;
    /* Only EFI_NOT_FOUND proves absence. I/O/access errors must not enable setup. */
    *exists = 1;
    if (!gBootVolumeRoot) return 0;
    status = gBootVolumeRoot->Open(gBootVolumeRoot, &file, (CHAR16 *)path, EFI_FILE_MODE_READ, 0);
    if (status == EFI_NOT_FOUND) { *exists = 0; return 0; }
    if (status != EFI_SUCCESS || !file) return 0;
    memory_zero(database, sizeof(*database));
    status = file->Read(file, &bytes, database);
    if (status == EFI_SUCCESS && bytes == sizeof(*database)) {
        UINT8 extra;
        UINTN trailing = 1U;
        status = file->Read(file, &trailing, &extra);
        if (status == EFI_SUCCESS && trailing) status = EFI_DEVICE_ERROR;
    }
    file->Close(file);
    return status == EFI_SUCCESS && bytes == sizeof(*database) && auth_database_valid(database);
}

static AUTH_DB_STATE auth_database_load(void) {
    AUTH_DATABASE first;
    AUTH_DATABASE second;
    int firstExists = 0, secondExists = 0;
    int firstValid, secondValid;
    memory_zero(&gAuthDatabase, sizeof(gAuthDatabase));
    gCurrentAccount = -1;
    if (gAuthStorageFailed) {
        gAuthDatabaseState = AUTH_DB_CORRUPT;
        return gAuthDatabaseState;
    }
    firstValid = auth_slot_read(gAuthSlot0Path, &first, &firstExists);
    secondValid = auth_slot_read(gAuthSlot1Path, &second, &secondExists);
    if (firstValid || secondValid) {
        AUTH_DATABASE *selected = firstValid && (!secondValid || first.generation >= second.generation) ?
            &first : &second;
        memory_copy(&gAuthDatabase, selected, sizeof(gAuthDatabase));
        gAuthDatabaseState = AUTH_DB_VALID;
    } else {
        gAuthDatabaseState = (firstExists || secondExists) ? AUTH_DB_CORRUPT : AUTH_DB_ABSENT;
    }
    memory_zero(&first, sizeof(first));
    memory_zero(&second, sizeof(second));
    return gAuthDatabaseState;
}

static int auth_slot_write_verify(const CHAR16 *path, const AUTH_DATABASE *database) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    AUTH_DATABASE verified;
    UINTN bytes = sizeof(*database);
    EFI_STATUS status;
    int exists = 0;
    /* Only the selected old/invalid slot is replaced; the other copy stays intact. */
    status = gBootVolumeRoot->Open(gBootVolumeRoot, &file, (CHAR16 *)path,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (status == EFI_SUCCESS && file) {
        if (file->Delete(file) != EFI_SUCCESS) return 0;
    } else if (status != EFI_NOT_FOUND) return 0;
    file = (EFI_FILE_PROTOCOL *)0;
    status = gBootVolumeRoot->Open(gBootVolumeRoot, &file, (CHAR16 *)path,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (status != EFI_SUCCESS || !file) return 0;
    status = file->SetPosition(file, 0);
    if (status == EFI_SUCCESS) status = file->Write(file, &bytes, (void *)database);
    if (status == EFI_SUCCESS && bytes == sizeof(*database)) status = file->Flush(file);
    file->Close(file);
    if (status != EFI_SUCCESS || bytes != sizeof(*database)) return 0;
    if (!auth_slot_read(path, &verified, &exists) ||
        !auth_constant_time_equal((const UINT8 *)&verified, (const UINT8 *)database, sizeof(*database))) {
        memory_zero(&verified, sizeof(verified));
        return 0;
    }
    memory_zero(&verified, sizeof(verified));
    return 1;
}

static int auth_database_save(void) {
    AUTH_DATABASE first, second;
    int firstExists = 0, secondExists = 0;
    int firstValid = auth_slot_read(gAuthSlot0Path, &first, &firstExists);
    int secondValid = auth_slot_read(gAuthSlot1Path, &second, &secondExists);
    const CHAR16 *target;
    const CHAR16 *mirror = (const CHAR16 *)0;
    UINT8 digest[AUTH_HASH_BYTES];
    if (!gBootVolumeRoot || gAuthDatabaseState == AUTH_DB_CORRUPT ||
        (gAuthDatabaseState == AUTH_DB_ABSENT && (firstExists || secondExists)) ||
        (gAuthDatabaseState == AUTH_DB_VALID && !firstValid && !secondValid) ||
        gAuthDatabase.generation == ~0ULL) return 0;
    if (!firstValid) target = gAuthSlot0Path;
    else if (!secondValid) target = gAuthSlot1Path;
    else target = first.generation <= second.generation ? gAuthSlot0Path : gAuthSlot1Path;
    if (!firstValid && !secondValid)
        mirror = target == gAuthSlot0Path ? gAuthSlot1Path : gAuthSlot0Path;
    gAuthDatabase.magic = AUTH_DB_MAGIC;
    gAuthDatabase.version = AUTH_DB_VERSION;
    gAuthDatabase.generation++;
    memory_zero(gAuthDatabase.checksum, sizeof(gAuthDatabase.checksum));
    auth_database_checksum(&gAuthDatabase, digest);
    memory_copy(gAuthDatabase.checksum, digest, sizeof(digest));
    memory_zero(digest, sizeof(digest));
    memory_zero(&first, sizeof(first));
    memory_zero(&second, sizeof(second));
    if (!auth_slot_write_verify(target, &gAuthDatabase)) {
        /* A cached read cannot prove durability after a failed write/flush.
           Revoke the session until reboot resolves any ambiguous disk state. */
        gAuthDatabaseState = AUTH_DB_CORRUPT;
        gAuthStorageFailed = 1;
        gCurrentAccount = -1;
        print("Account storage write failed; reboot before further authentication.\n");
        return 0;
    }
    /* The first durable generation is mirrored after its verified commit. A mirror
       failure leaves one valid fail-closed copy and is repaired by the next save. */
    if (mirror) (void)auth_slot_write_verify(mirror, &gAuthDatabase);
    gAuthDatabaseState = AUTH_DB_VALID;
    return 1;
}

static int auth_find_account(const char *username) {
    UINTN index;
    for (index = 0; index < AUTH_MAX_ACCOUNTS; index++)
        if (gAuthDatabase.accounts[index].used &&
            auth_username_equal(gAuthDatabase.accounts[index].username, username)) return (int)index;
    return -1;
}

static void auth_password_hash(const char *password, const UINT8 salt[AUTH_SALT_BYTES],
                               UINT8 digest[AUTH_HASH_BYTES]) {
    UINT8 work[AUTH_HASH_BYTES + AUTH_SALT_BYTES + AUTH_PASSWORD_BYTES];
    UINTN passwordBytes = string_length(password);
    UINTN iteration;
    memory_copy(work, salt, AUTH_SALT_BYTES);
    memory_copy(work + AUTH_SALT_BYTES, password, passwordBytes);
    sha256_bytes(work, AUTH_SALT_BYTES + passwordBytes, digest);
    for (iteration = 1U; iteration < AUTH_KDF_ITERATIONS; iteration++) {
        memory_copy(work, digest, AUTH_HASH_BYTES);
        memory_copy(work + AUTH_HASH_BYTES, salt, AUTH_SALT_BYTES);
        memory_copy(work + AUTH_HASH_BYTES + AUTH_SALT_BYTES, password, passwordBytes);
        sha256_bytes(work, AUTH_HASH_BYTES + AUTH_SALT_BYTES + passwordBytes, digest);
    }
    secure_zero(work, sizeof(work));
}

static int auth_verify_password(const AUTH_ACCOUNT *account, const char *password) {
    UINT8 digest[AUTH_HASH_BYTES];
    int valid;
    auth_password_hash(password, account->salt, digest);
    valid = auth_constant_time_equal(digest, account->passwordHash, sizeof(digest));
    secure_zero(digest, sizeof(digest));
    return valid;
}

static int auth_salt_duplicate(const UINT8 salt[AUTH_SALT_BYTES]) {
    UINTN index;
    for (index = 0; index < AUTH_MAX_ACCOUNTS; index++)
        if (gAuthDatabase.accounts[index].used &&
            auth_constant_time_equal(salt, gAuthDatabase.accounts[index].salt, AUTH_SALT_BYTES)) return 1;
    return 0;
}

static int auth_generate_salt(const char *username, UINT8 salt[AUTH_SALT_BYTES]) {
    EFI_RNG_PROTOCOL *rng = (EFI_RNG_PROTOCOL *)0;
    UINTN attempt;
    if (gST->BootServices->LocateProtocol(&gRngProtocolGuid, (void *)0, (void **)&rng) == EFI_SUCCESS &&
        rng && rng->GetRNG) {
        for (attempt = 0; attempt < 4U; attempt++) {
            UINT8 nonzero = 0;
            UINTN index;
            if (rng->GetRNG(rng, (EFI_GUID *)0, AUTH_SALT_BYTES, salt) != EFI_SUCCESS) break;
            for (index = 0; index < AUTH_SALT_BYTES; index++) nonzero |= salt[index];
            if (nonzero && !auth_salt_duplicate(salt)) return 1;
        }
    }
    /* Bounded fallback: timer, database generation, username, and a per-boot counter
       are SHA-256 mixed. This guarantees practical salt uniqueness, not firmware entropy. */
    for (attempt = 0; attempt < 16U; attempt++) {
        UINT8 seed[96];
        UINT8 digest[AUTH_HASH_BYTES];
        UINTN used = 0;
        UINT64 timer = timer_count();
        UINT64 counter = ++gAuthFallbackCounter;
        memory_zero(seed, sizeof(seed));
        memory_copy(seed + used, &timer, sizeof(timer)); used += sizeof(timer);
        memory_copy(seed + used, &gAuthDatabase.generation, sizeof(gAuthDatabase.generation)); used += sizeof(gAuthDatabase.generation);
        memory_copy(seed + used, &counter, sizeof(counter)); used += sizeof(counter);
        memory_copy(seed + used, username, string_length(username)); used += string_length(username);
        sha256_bytes(seed, used, digest);
        memory_copy(salt, digest, AUTH_SALT_BYTES);
        memory_zero(seed, sizeof(seed));
        memory_zero(digest, sizeof(digest));
        if (!auth_salt_duplicate(salt)) return 1;
    }
    memory_zero(salt, AUTH_SALT_BYTES);
    return 0;
}

static void auth_read_password(const char *prompt, char password[AUTH_PASSWORD_BYTES]) {
    UINTN used = 0;
    int overflow = 0;
    EFI_INPUT_KEY key;
    print(prompt);
    memory_zero(password, AUTH_PASSWORD_BYTES);
    for (;;) {
        EFI_STATUS status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
        if (status == EFI_NOT_READY) { gST->BootServices->Stall(1000U); continue; }
        if (status != EFI_SUCCESS || !key.UnicodeChar) continue;
        if (key.UnicodeChar == '\r' || key.UnicodeChar == '\n') {
            print("\n");
            password[used] = 0;
            if (overflow) {
                secure_zero(password, AUTH_PASSWORD_BYTES);
                print("Password input exceeded 64 characters; entry rejected.\n");
            }
            secure_zero(&key, sizeof(key));
            return;
        }
        if (key.UnicodeChar == 8 || key.UnicodeChar == 127) {
            if (used) { used--; password[used] = 0; print("\b \b"); }
        } else if (key.UnicodeChar >= 32 && key.UnicodeChar <= 126) {
            if (used + 1U < AUTH_PASSWORD_BYTES) {
                password[used++] = (char)key.UnicodeChar;
                print_char('*');
            } else overflow = 1;
        }
        secure_zero(&key, sizeof(key));
    }
}

static int auth_read_username(char username[AUTH_USERNAME_BYTES]) {
    UINTN used = 0;
    int overflow = 0;
    EFI_INPUT_KEY key;
    memory_zero(username, AUTH_USERNAME_BYTES);
    for (;;) {
        EFI_STATUS status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
        if (status == EFI_NOT_READY) { gST->BootServices->Stall(1000U); continue; }
        if (status != EFI_SUCCESS || !key.UnicodeChar) continue;
        if (key.UnicodeChar == '\r' || key.UnicodeChar == '\n') {
            print("\n");
            username[used] = 0;
            return !overflow;
        }
        if (key.UnicodeChar == 8 || key.UnicodeChar == 127) {
            if (used) { used--; username[used] = 0; print("\b \b"); }
        } else if (key.UnicodeChar >= 32 && key.UnicodeChar <= 126) {
            if (used + 1U < AUTH_USERNAME_BYTES) {
                username[used++] = (char)key.UnicodeChar;
                print_char((char)key.UnicodeChar);
            } else overflow = 1;
        }
    }
}

static int auth_set_password(AUTH_ACCOUNT *account, const char *password) {
    UINT8 salt[AUTH_SALT_BYTES];
    /* Generate separately: testing an in-place salt matches the account itself. */
    if (!auth_generate_salt(account->username, salt)) return 0;
    auth_password_hash(password, salt, account->passwordHash);
    memory_copy(account->salt, salt, sizeof(salt));
    secure_zero(salt, sizeof(salt));
    return 1;
}

static int auth_prompt_new_password(AUTH_ACCOUNT *account) {
    char first[AUTH_PASSWORD_BYTES], second[AUTH_PASSWORD_BYTES];
    int success = 0;
    auth_read_password("New password (8-64 characters): ", first);
    auth_read_password("Repeat password: ", second);
    if (!auth_password_valid(first)) print("Password must contain 8-64 printable characters.\n");
    else if (!streq(first, second)) print("Passwords do not match.\n");
    else success = auth_set_password(account, first);
    secure_zero(first, sizeof(first));
    secure_zero(second, sizeof(second));
    return success;
}

static int auth_setup_first_admin(int establishSession) {
    char username[AUTH_USERNAME_BYTES];
    AUTH_DATABASE backup;
    AUTH_ACCOUNT *account;
    if (gAuthDatabaseState == AUTH_DB_CORRUPT) {
        print("Authentication database is corrupt; setup is locked. Use trusted offline recovery.\n");
        return 0;
    }
    if (gAuthDatabaseState == AUTH_DB_VALID && gAuthDatabase.accountCount) return 0;
    print("\n=== First Administrator Setup ===\n");
    print("Username (1-23 letters, digits, _ or -): ");
    if (!auth_read_username(username) || !auth_username_valid(username) || auth_find_account(username) >= 0) {
        print("Invalid or duplicate username.\n");
        memory_zero(username, sizeof(username));
        return 0;
    }
    memory_copy(&backup, &gAuthDatabase, sizeof(backup));
    if (gAuthDatabaseState == AUTH_DB_ABSENT) memory_zero(&gAuthDatabase, sizeof(gAuthDatabase));
    account = &gAuthDatabase.accounts[0];
    memory_zero(account, sizeof(*account));
    account->used = 1;
    account->role = AUTH_ROLE_ADMIN;
    string_copy(account->username, username, sizeof(account->username));
    gAuthDatabase.accountCount = 1U;
    if (!auth_prompt_new_password(account) || !auth_database_save()) {
        memory_copy(&gAuthDatabase, &backup, sizeof(backup));
        print("Administrator setup was not saved.\n");
        memory_zero(&backup, sizeof(backup));
        memory_zero(username, sizeof(username));
        return 0;
    }
    if (establishSession) gCurrentAccount = 0;
    print("Administrator account created.\n");
    memory_zero(&backup, sizeof(backup));
    memory_zero(username, sizeof(username));
    return 1;
}

static int auth_session_is_admin(void) {
    return gAuthDatabaseState == AUTH_DB_VALID && gCurrentAccount >= 0 &&
        (UINTN)gCurrentAccount < AUTH_MAX_ACCOUNTS &&
        gAuthDatabase.accounts[gCurrentAccount].used &&
        gAuthDatabase.accounts[gCurrentAccount].role == AUTH_ROLE_ADMIN;
}

static int auth_login(void) {
    char username[AUTH_USERNAME_BYTES];
    char password[AUTH_PASSWORD_BYTES];
    int account;
    gCurrentAccount = -1;
    if (gAuthDatabaseState == AUTH_DB_ABSENT ||
        (gAuthDatabaseState == AUTH_DB_VALID && !gAuthDatabase.accountCount))
        return auth_setup_first_admin(1);
    if (gAuthDatabaseState != AUTH_DB_VALID) {
        print("Authentication database is corrupt; login denied.\n");
        delay_ms(1000);
        return 0;
    }
    print("Username: ");
    account = auth_read_username(username) ? auth_find_account(username) : -1;
    auth_read_password("Password: ", password);
    if (account >= 0 && auth_verify_password(&gAuthDatabase.accounts[account], password)) {
        gCurrentAccount = account;
        print("Welcome, "); print(gAuthDatabase.accounts[account].username); print(".\n");
    } else print("Login failed.\n");
    memory_zero(username, sizeof(username));
    secure_zero(password, sizeof(password));
    return gCurrentAccount >= 0;
}

static int auth_authorize_admin(const char *action) {
    char password[AUTH_PASSWORD_BYTES];
    int allowed;
    if (!auth_session_is_admin()) {
        print(action); print(": administrator access required\n");
        return 0;
    }
    print("Administrator re-authentication for "); print(action); print(".\n");
    auth_read_password("Password: ", password);
    allowed = auth_verify_password(&gAuthDatabase.accounts[gCurrentAccount], password);
    secure_zero(password, sizeof(password));
    if (!allowed) {
        delay_ms(1000U);
        print("Authentication failed; operation cancelled.\n");
    }
    return allowed;
}

static int auth_pre_os_authorize_admin(const char *action) {
    char username[AUTH_USERNAME_BYTES], password[AUTH_PASSWORD_BYTES];
    int account;
    if (auth_database_load() == AUTH_DB_ABSENT ||
        (gAuthDatabaseState == AUTH_DB_VALID && !gAuthDatabase.accountCount)) {
        if (!auth_setup_first_admin(0)) return 0;
    }
    if (gAuthDatabaseState != AUTH_DB_VALID) {
        print("Authentication database is corrupt; privileged recovery operations are denied.\n");
        return 0;
    }
    print("Administrator authentication for "); print(action); print(".\nUsername: ");
    account = auth_read_username(username) ? auth_find_account(username) : -1;
    auth_read_password("Password: ", password);
    if (account < 0 || gAuthDatabase.accounts[account].role != AUTH_ROLE_ADMIN ||
        !auth_verify_password(&gAuthDatabase.accounts[account], password)) account = -1;
    memory_zero(username, sizeof(username));
    secure_zero(password, sizeof(password));
    if (account < 0) {
        delay_ms(1000U);
        print("Administrator authentication failed; operation cancelled.\n");
    }
    return account >= 0;
}

static void auth_list_accounts(void) {
    UINTN index;
    for (index = 0; index < AUTH_MAX_ACCOUNTS; index++) if (gAuthDatabase.accounts[index].used) {
        print(gAuthDatabase.accounts[index].username);
        print(gAuthDatabase.accounts[index].role == AUTH_ROLE_ADMIN ? "  Administrator" : "  Standard");
        if ((int)index == gCurrentAccount) print("  (active)");
        print("\n");
    }
}

static void auth_add_account(const char *username, UINT8 role) {
    AUTH_DATABASE backup;
    UINTN index;
    if (!username || !auth_username_valid(username) || auth_find_account(username) >= 0) {
        print("Username must be unique and contain 1-23 letters, digits, _ or -.\n"); return;
    }
    if (role > AUTH_ROLE_ADMIN) { print("Invalid account role.\n"); return; }
    if (!auth_authorize_admin("adding an account")) return;
    for (index = 0; index < AUTH_MAX_ACCOUNTS && gAuthDatabase.accounts[index].used; index++);
    if (index == AUTH_MAX_ACCOUNTS) { print("Account database is full (8 accounts).\n"); return; }
    memory_copy(&backup, &gAuthDatabase, sizeof(backup));
    memory_zero(&gAuthDatabase.accounts[index], sizeof(AUTH_ACCOUNT));
    gAuthDatabase.accounts[index].used = 1;
    gAuthDatabase.accounts[index].role = role;
    string_copy(gAuthDatabase.accounts[index].username, username, AUTH_USERNAME_BYTES);
    gAuthDatabase.accountCount++;
    if (!auth_prompt_new_password(&gAuthDatabase.accounts[index]) || !auth_database_save()) {
        memory_copy(&gAuthDatabase, &backup, sizeof(backup)); print("Account was not saved.\n");
    } else print("Account added.\n");
    memory_zero(&backup, sizeof(backup));
}

static void auth_change_password(const char *username) {
    int target = username && *username ? auth_find_account(username) : gCurrentAccount;
    AUTH_DATABASE backup;
    char password[AUTH_PASSWORD_BYTES];
    if (target < 0) { print("Account not found.\n"); return; }
    if (target == gCurrentAccount) {
        auth_read_password("Current password: ", password);
        if (!auth_verify_password(&gAuthDatabase.accounts[target], password)) {
            secure_zero(password, sizeof(password));
            delay_ms(1000U); print("Current password is incorrect.\n"); return;
        }
        secure_zero(password, sizeof(password));
    } else if (!auth_authorize_admin("changing another user's password")) return;
    memory_copy(&backup, &gAuthDatabase, sizeof(backup));
    if (!auth_prompt_new_password(&gAuthDatabase.accounts[target]) || !auth_database_save()) {
        memory_copy(&gAuthDatabase, &backup, sizeof(backup)); print("Password was not saved.\n");
    } else print("Password changed.\n");
    memory_zero(&backup, sizeof(backup));
}

static void auth_delete_account(const char *username) {
    int target = auth_find_account(username);
    AUTH_DATABASE backup;
    UINTN index, admins = 0;
    if (target < 0) { print("Account not found.\n"); return; }
    if (target == gCurrentAccount) { print("Cannot delete the active account.\n"); return; }
    for (index = 0; index < AUTH_MAX_ACCOUNTS; index++)
        if (gAuthDatabase.accounts[index].used && gAuthDatabase.accounts[index].role == AUTH_ROLE_ADMIN) admins++;
    if (gAuthDatabase.accounts[target].role == AUTH_ROLE_ADMIN && admins <= 1U) {
        print("Cannot delete the last administrator.\n"); return;
    }
    if (!auth_authorize_admin("deleting an account")) return;
    memory_copy(&backup, &gAuthDatabase, sizeof(backup));
    memory_zero(&gAuthDatabase.accounts[target], sizeof(AUTH_ACCOUNT));
    gAuthDatabase.accountCount--;
    if (!auth_database_save()) { memory_copy(&gAuthDatabase, &backup, sizeof(backup)); print("Account deletion was not saved.\n"); }
    else print("Account deleted.\n");
    memory_zero(&backup, sizeof(backup));
}
