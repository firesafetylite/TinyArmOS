/* Full-screen account management within Settings. Persistence stays in auth.inc. */
static void settings_accounts_header(const char *title) {
    gST->ConOut->ClearScreen(gST->ConOut);
    settings_use_accent_color();
    print("=== Settings > User accounts");
    if (title) { print(" > "); print(title); }
    print(" ===\n");
    settings_use_default_color();
}

static void settings_accounts_show(const char *notice) {
    settings_accounts_header((const char *)0);
    print("Global accounts (saved on protected recovery storage):\n\n");
    auth_list_accounts();
    print("\n  1  Change my password\n");
    if (auth_session_is_admin()) {
        print("  2  Add account\n");
        print("  3  Change another user's password\n");
        print("  4  Delete account\n");
    }
    print("  0  Back to Settings\n");
    if (notice) { print("\n"); print(notice); print("\n"); }
}

static int settings_accounts_username(char username[AUTH_USERNAME_BYTES]) {
    print("Username (blank to cancel): ");
    if (!auth_read_username(username)) return 0;
    if (!username[0]) { print("Canceled.\n"); return 0; }
    return 1;
}

/* Return false if a storage failure revoked the session, so Settings exits too. */
static int settings_accounts(void) {
    const char *notice = (const char *)0;
    char choice[32];
    char username[AUTH_USERNAME_BYTES];
    for (;;) {
        int admin;
        if (gCurrentAccount < 0 || gAuthDatabaseState != AUTH_DB_VALID) {
            print("Account storage is unavailable; leaving Settings. Reboot to recover.\n");
            return 0;
        }
        admin = auth_session_is_admin();
        settings_accounts_show(notice);
        print("\nSelect: ");
        read_line(choice, sizeof(choice));
        if (streq(choice, "0")) return 1;
        notice = (const char *)0;
        if (streq(choice, "1")) {
            settings_accounts_header("Change my password");
            auth_change_password((const char *)0);
        } else if (admin && streq(choice, "2")) {
            settings_accounts_header("Add account");
            if (settings_accounts_username(username)) {
                print("\n  1  Standard\n  2  Administrator\n  0  Cancel\n\nRole: ");
                read_line(choice, sizeof(choice));
                if (streq(choice, "1") || streq(choice, "2"))
                    auth_add_account(username, streq(choice, "2") ? AUTH_ROLE_ADMIN : AUTH_ROLE_STANDARD);
                else print(streq(choice, "0") ? "Canceled.\n" : "Invalid role; account was not added.\n");
            }
        } else if (admin && (streq(choice, "3") || streq(choice, "4"))) {
            int deleting = streq(choice, "4");
            settings_accounts_header(deleting ? "Delete account" : "Change another user's password");
            if (deleting) print("Deletes only the account, not shared files. The active account cannot be deleted.\n");
            if (settings_accounts_username(username)) {
                print(deleting ? "Delete account: " : "Change password for: ");
                print(username); print("\n");
                if (deleting) auth_delete_account(username);
                else auth_change_password(username);
            }
        } else {
            notice = admin ? "Unknown selection; choose 0 through 4." : "Choose 1 to change your password, or 0 to go back.";
            continue;
        }
        memory_zero(username, sizeof(username));
        /* Keep the operation's result visible; never report an automatic save
           to .tinygptrc for credentials, or accept more actions after failure. */
        if (gCurrentAccount < 0 || gAuthDatabaseState != AUTH_DB_VALID) continue;
        print("\nPress Enter to return to User accounts: ");
        read_line(choice, sizeof(choice));
    }
}
