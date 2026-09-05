#define EDITOR_SCAN_UP        1U
#define EDITOR_SCAN_DOWN      2U
#define EDITOR_SCAN_RIGHT     3U
#define EDITOR_SCAN_LEFT      4U
#define EDITOR_SCAN_DELETE    8U
#define EDITOR_SCAN_F2        12U
#define EDITOR_SCAN_ESCAPE    23U
#define EDITOR_SCREEN_ROWS    64U

/* The editor buffer is transient; the authoritative bytes stay in the FAT file. */
static char gEditorBuffer[FS_DATA_BYTES];
static char gEditorScreen[EDITOR_SCREEN_ROWS][SCROLLBACK_COLUMNS];
static UINT8 gEditorScreenValid[EDITOR_SCREEN_ROWS];

static void editor_visual_advance(
    char ch, UINTN width, UINTN *row, UINTN *column, int *wrapped
) {
    if (ch == '\n') {
        if (*wrapped) *wrapped = 0;
        else {
            (*row)++;
            *column = 0;
        }
        return;
    }
    *wrapped = 0;
    (*column)++;
    if (*column >= width) {
        (*row)++;
        *column = 0;
        *wrapped = 1;
    }
}

static void editor_visual_position(
    const char *buffer, UINTN cursor, UINTN width, UINTN *rowOut, UINTN *columnOut
) {
    UINTN offset;
    UINTN row = 0;
    UINTN column = 0;
    int wrapped = 0;
    for (offset = 0; offset < cursor; offset++)
        editor_visual_advance(buffer[offset], width, &row, &column, &wrapped);
    *rowOut = row;
    *columnOut = column;
}

static int editor_visual_offset(
    const char *buffer, UINTN length, UINTN width, UINTN targetRow,
    UINTN targetColumn, UINTN *offsetOut
) {
    UINTN offset;
    UINTN row = 0;
    UINTN column = 0;
    UINTN best = 0;
    int wrapped = 0;
    int found = 0;
    for (offset = 0; offset <= length; offset++) {
        if (row == targetRow) {
            if (column <= targetColumn) {
                best = offset;
                found = 1;
            } else break;
        } else if (row > targetRow) break;
        if (offset == length) break;
        editor_visual_advance(buffer[offset], width, &row, &column, &wrapped);
    }
    if (!found) return 0;
    *offsetOut = best;
    return 1;
}

static UINTN editor_move_vertical(
    const char *buffer, UINTN length, UINTN cursor, UINTN width, int direction
) {
    UINTN row;
    UINTN column;
    UINTN target;
    editor_visual_position(buffer, cursor, width, &row, &column);
    if (direction < 0) {
        if (!row) return cursor;
        row--;
    } else row++;
    if (!editor_visual_offset(buffer, length, width, row, column, &target)) return cursor;
    return target;
}

static void editor_visual_row_text(
    const char *buffer, UINTN length, UINTN width, UINTN targetRow,
    char output[SCROLLBACK_COLUMNS]
) {
    UINTN offset;
    UINTN row = 0;
    UINTN column = 0;
    UINTN used = 0;
    int wrapped = 0;
    for (offset = 0; offset < length; offset++) {
        char ch = buffer[offset];
        if (row == targetRow && ch != '\n' && column < width) {
            output[column] = ch == '\t' ? ' ' : ch;
            if (column + 1U > used) used = column + 1U;
        }
        editor_visual_advance(ch, width, &row, &column, &wrapped);
        if (row > targetRow) break;
    }
    output[used] = 0;
}

static int editor_insert(char *buffer, UINTN *length, UINTN *cursor, char ch) {
    UINTN index;
    if (*length + 1U >= FS_DATA_BYTES) return 0;
    for (index = *length + 1U; index > *cursor; index--)
        buffer[index] = buffer[index - 1U];
    buffer[*cursor] = ch;
    (*cursor)++;
    (*length)++;
    return 1;
}

static void editor_delete(char *buffer, UINTN *length, UINTN offset) {
    UINTN index;
    if (offset >= *length) return;
    for (index = offset; index < *length; index++) buffer[index] = buffer[index + 1U];
    (*length)--;
}

static int editor_load_text(UINTN node, UINTN *lengthOut) {
    char diskText[FS_DATA_BYTES];
    UINTN diskLength = 0;
    UINTN input;
    UINTN output = 0;
    if (!storage_read_node(node, diskText, sizeof(diskText), &diskLength)) return 0;
    for (input = 0; input < diskLength; input++) {
        UINT8 ch = (UINT8)diskText[input];
        if (ch == '\r') {
            if (input + 1U < diskLength && diskText[input + 1U] == '\n') input++;
            ch = '\n';
        }
        if (!(ch == '\n' || ch == '\t' || (ch >= 32U && ch <= 126U))) return 0;
        if (output + 1U >= FS_DATA_BYTES) return 0;
        gEditorBuffer[output++] = (char)ch;
    }
    gEditorBuffer[output] = 0;
    *lengthOut = output;
    return 1;
}

static void editor_dimensions(UINTN *columnsOut, UINTN *rowsOut) {
    UINTN columns = 80U;
    UINTN rows = 25U;
    UINTN mode = 0;
    if (gST->ConOut->Mode && gST->ConOut->Mode->Mode >= 0)
        mode = (UINTN)gST->ConOut->Mode->Mode;
    if (gST->ConOut->QueryMode(gST->ConOut, mode, &columns, &rows) != EFI_SUCCESS) {
        columns = 80U;
        rows = 25U;
    }
    if (columns < 20U) columns = 20U;
    if (columns >= SCROLLBACK_COLUMNS) columns = SCROLLBACK_COLUMNS - 1U;
    if (rows < 8U) rows = 8U;
    if (rows > EDITOR_SCREEN_ROWS) rows = EDITOR_SCREEN_ROWS;
    if (columns > 1U) columns--;
    *columnsOut = columns;
    *rowsOut = rows;
}

static void editor_screen_reset(void) {
    memory_zero(gEditorScreenValid, sizeof(gEditorScreenValid));
    gST->ConOut->ClearScreen(gST->ConOut);
}

static void editor_wait_key(EFI_INPUT_KEY *key) {
    while (gST->ConIn->ReadKeyStroke(gST->ConIn, key) != EFI_SUCCESS)
        __asm__ volatile("yield");
}

static void editor_write_row(UINTN row, const char *text, UINTN columns) {
    CHAR16 output[SCROLLBACK_COLUMNS];
    UINTN used = 0;
    UINTN index;
    UINTN length = string_length(text);
    int unchanged = row < EDITOR_SCREEN_ROWS && gEditorScreenValid[row];
    while (used < columns) {
        char ch = used < length ? text[used] : ' ';
        output[used] = (CHAR16)(UINT8)ch;
        if (unchanged && gEditorScreen[row][used] != ch) unchanged = 0;
        used++;
    }
    output[used] = 0;
    if (unchanged) return;
    if (row < EDITOR_SCREEN_ROWS) {
        for (index = 0; index < columns; index++)
            gEditorScreen[row][index] = (char)(UINT8)output[index];
        gEditorScreen[row][columns] = 0;
        gEditorScreenValid[row] = 1;
    }
    gST->ConOut->SetCursorPosition(gST->ConOut, 0, row);
    out16(output);
}

static void editor_draw(
    const char *path, UINTN length, UINTN cursor, UINTN *topRow,
    int readOnly, int modified, const char *notice
) {
    char pathLine[SCROLLBACK_COLUMNS];
    char visible[SCROLLBACK_COLUMNS];
    UINTN columns;
    UINTN rows;
    UINTN contentRows;
    UINTN cursorRow;
    UINTN cursorColumn;
    UINTN row;
    editor_dimensions(&columns, &rows);
    contentRows = rows - 4U;
    editor_visual_position(gEditorBuffer, cursor, columns, &cursorRow, &cursorColumn);
    if (cursorRow < *topRow) *topRow = cursorRow;
    else if (cursorRow >= *topRow + contentRows)
        *topRow = cursorRow - contentRows + 1U;

    settings_use_accent_color();
    editor_write_row(0, "=== TinyGPT Text Editor ===", columns);
    settings_use_default_color();
    pathLine[0] = 0;
    string_append(pathLine, path, sizeof(pathLine));
    if (readOnly) string_append(pathLine, "  [READ ONLY]", sizeof(pathLine));
    if (modified) string_append(pathLine, "  *", sizeof(pathLine));
    editor_write_row(1, pathLine, columns);

    for (row = 0; row < contentRows; row++) {
        editor_visual_row_text(
            gEditorBuffer, length, columns, *topRow + row, visible
        );
        editor_write_row(2U + row, visible, columns);
    }
    editor_write_row(rows - 2U, notice ? notice : "Soft wrap is on; Up/Down moves and scrolls wrapped rows.", columns);
    settings_use_accent_color();
    editor_write_row(rows - 1U, "F2/Ctrl+S Save | Esc Exit | Arrow Keys Move/Scroll | Backspace/Delete", columns);
    settings_use_default_color();
    gST->ConOut->SetCursorPosition(
        gST->ConOut, cursorColumn, 2U + cursorRow - *topRow
    );
    gST->ConOut->EnableCursor(gST->ConOut, 1);
}

static int editor_save(int *node, UINTN parent, const char *name, UINTN length) {
    int created = *node < 0;
    if (created) *node = fs_alloc(FS_FILE, parent, name, 0);
    if (*node < 0 || gNodes[*node].type != FS_FILE) return -1;
    if (fs_write_file((UINTN)*node, gEditorBuffer, length)) return 1;
    if (created) {
        (void)storage_delete_node((UINTN)*node);
        memory_zero(&gNodes[*node], sizeof(FS_NODE));
        *node = -1;
    }
    return 0;
}

static UINTN editor_picker_collect(UINTN directory, UINTN nodes[FS_MAX_NODES]) {
    UINTN type;
    UINTN node;
    UINTN count = 0;
    for (type = FS_DIRECTORY; type >= FS_FILE; type--) {
        for (node = 1U; node < FS_MAX_NODES; node++) {
            if (gNodes[node].used && gNodes[node].parent == directory &&
                gNodes[node].type == type) nodes[count++] = node;
        }
    }
    return count;
}

static void editor_picker_draw(
    UINTN directory, const UINTN nodes[FS_MAX_NODES], UINTN nodeCount,
    UINTN selected, UINTN *topItem, const char *notice
) {
    char location[FS_PATH_BYTES];
    char rowText[SCROLLBACK_COLUMNS];
    UINTN columns;
    UINTN rows;
    UINTN listRows;
    UINTN specials = directory == FS_ROOT ? 1U : 2U;
    UINTN itemCount = nodeCount + specials;
    UINTN row;
    editor_dimensions(&columns, &rows);
    listRows = rows - 5U;
    if (selected < *topItem) *topItem = selected;
    else if (selected >= *topItem + listRows) *topItem = selected - listRows + 1U;
    fs_path(directory, location, sizeof(location));
    gST->ConOut->EnableCursor(gST->ConOut, 0);
    settings_use_accent_color();
    editor_write_row(0, "=== TinyGPT Text Editor: Open File ===", columns);
    settings_use_default_color();
    rowText[0] = 0;
    string_append(rowText, "Location: ", sizeof(rowText));
    string_append(rowText, location, sizeof(rowText));
    editor_write_row(1, rowText, columns);
    editor_write_row(2, "Select an ASCII text file or create a new one.", columns);
    for (row = 0; row < listRows; row++) {
        UINTN item = *topItem + row;
        rowText[0] = 0;
        if (item < itemCount) {
            string_append(rowText, item == selected ? "> " : "  ", sizeof(rowText));
            if (!item) string_append(rowText, "[ New text file ]", sizeof(rowText));
            else if (directory != FS_ROOT && item == 1U)
                string_append(rowText, "<DIR> ..", sizeof(rowText));
            else {
                UINTN node = nodes[item - specials];
                string_append(rowText, gNodes[node].type == FS_DIRECTORY ? "<DIR> " : "       ", sizeof(rowText));
                string_append(rowText, gNodes[node].name, sizeof(rowText));
                if (fs_is_protected(node)) string_append(rowText, "  [protected]", sizeof(rowText));
            }
        }
        if (item == selected) settings_use_accent_color();
        else settings_use_default_color();
        editor_write_row(3U + row, rowText, columns);
    }
    settings_use_default_color();
    editor_write_row(rows - 2U, notice ? notice : "Enter opens the selected item.", columns);
    settings_use_accent_color();
    editor_write_row(rows - 1U, "Up/Down Select | Enter Open | N New | Left/Backspace/B Parent | Esc Exit", columns);
    settings_use_default_color();
}

static void editor_new_file_draw(
    UINTN directory, const char *name, const char *notice
) {
    char location[FS_PATH_BYTES];
    char input[SCROLLBACK_COLUMNS];
    UINTN columns;
    UINTN rows;
    UINTN cursorColumn;
    editor_dimensions(&columns, &rows);
    fs_path(directory, location, sizeof(location));
    settings_use_accent_color();
    editor_write_row(0, "=== TinyGPT Text Editor: New File ===", columns);
    settings_use_default_color();
    editor_write_row(1, "Enter a FAT-backed file name. The file is created when first saved.", columns);
    input[0] = 0;
    string_append(input, "Location: ", sizeof(input));
    string_append(input, location, sizeof(input));
    editor_write_row(2, input, columns);
    input[0] = 0;
    string_append(input, "Name: ", sizeof(input));
    string_append(input, name, sizeof(input));
    editor_write_row(4, input, columns);
    editor_write_row(rows - 2U, notice ? notice : "Allowed: letters, digits, dot, underscore, dash, plus.", columns);
    settings_use_accent_color();
    editor_write_row(rows - 1U, "Enter Create | Backspace Delete | Left/Esc Back to File Picker", columns);
    settings_use_default_color();
    cursorColumn = string_length(input);
    if (cursorColumn >= columns) cursorColumn = columns - 1U;
    gST->ConOut->SetCursorPosition(gST->ConOut, cursorColumn, 4U);
    gST->ConOut->EnableCursor(gST->ConOut, 1);
}

static int editor_new_file_modal(UINTN directory, char *path, UINTN capacity) {
    char directoryPath[FS_PATH_BYTES];
    char name[FS_NAME_BYTES];
    const char *notice = (const char *)0;
    UINTN used = 0;
    EFI_INPUT_KEY key;
    name[0] = 0;
    editor_screen_reset();
    for (;;) {
        editor_new_file_draw(directory, name, notice);
        notice = (const char *)0;
        editor_wait_key(&key);
        if (key.ScanCode == EDITOR_SCAN_ESCAPE || key.ScanCode == EDITOR_SCAN_LEFT ||
            key.UnicodeChar == 27U) return 0;
        if (key.ScanCode == EDITOR_SCAN_DELETE || key.UnicodeChar == 8U ||
            key.UnicodeChar == 127U) {
            if (used) name[--used] = 0;
        } else if (key.UnicodeChar == '\r' || key.UnicodeChar == '\n') {
            UINTN needed;
            if (!fs_valid_name(name)) {
                notice = "Invalid file name.";
                continue;
            }
            if (fs_find_child(directory, name) >= 0) {
                notice = "That name already exists; select it from the file picker.";
                continue;
            }
            fs_path(directory, directoryPath, sizeof(directoryPath));
            needed = string_length(directoryPath) + string_length(name) +
                     (directory == FS_ROOT ? 1U : 2U);
            if (needed > capacity) {
                notice = "The resulting path is too long.";
                continue;
            }
            string_copy(path, directoryPath, capacity);
            if (directory != FS_ROOT) string_append(path, "/", capacity);
            string_append(path, name, capacity);
            return 1;
        } else if (key.UnicodeChar >= 32U && key.UnicodeChar <= 126U &&
                   used + 1U < sizeof(name)) {
            name[used++] = (char)key.UnicodeChar;
            name[used] = 0;
        }
    }
}

static int editor_file_picker(char *path, UINTN capacity) {
    UINTN nodes[FS_MAX_NODES];
    UINTN directory = gCwd;
    UINTN selected = 0;
    UINTN topItem = 0;
    const char *notice = (const char *)0;
    EFI_INPUT_KEY key;
    if (directory >= FS_MAX_NODES || !gNodes[directory].used ||
        gNodes[directory].type != FS_DIRECTORY) directory = FS_ROOT;
    editor_screen_reset();
    for (;;) {
        UINTN nodeCount = editor_picker_collect(directory, nodes);
        UINTN specials = directory == FS_ROOT ? 1U : 2U;
        UINTN itemCount = nodeCount + specials;
        if (selected >= itemCount) selected = itemCount - 1U;
        editor_picker_draw(directory, nodes, nodeCount, selected, &topItem, notice);
        notice = (const char *)0;
        editor_wait_key(&key);
        if (key.ScanCode == EDITOR_SCAN_ESCAPE || key.UnicodeChar == 27U) return 0;
        if (key.ScanCode == EDITOR_SCAN_UP) {
            selected = selected ? selected - 1U : itemCount - 1U;
        } else if (key.ScanCode == EDITOR_SCAN_DOWN) {
            selected = selected + 1U < itemCount ? selected + 1U : 0U;
        } else if (key.ScanCode == EDITOR_SCAN_LEFT ||
                   key.ScanCode == EDITOR_SCAN_DELETE || key.UnicodeChar == 8U ||
                   key.UnicodeChar == 127U || key.UnicodeChar == 'b' ||
                   key.UnicodeChar == 'B') {
            if (directory != FS_ROOT) {
                directory = gNodes[directory].parent;
                selected = 0;
                topItem = 0;
            } else notice = "Already at the filesystem root.";
        } else if (key.UnicodeChar == 'n' || key.UnicodeChar == 'N' ||
                   ((key.UnicodeChar == '\r' || key.UnicodeChar == '\n') && !selected)) {
            if (fs_is_protected(directory) && !auth_session_is_admin()) {
                notice = "Protected directory: authenticate as an administrator before creating a file here.";
            } else if (editor_new_file_modal(directory, path, capacity)) return 1;
            else {
                editor_screen_reset();
                notice = "New file canceled.";
            }
        } else if (key.UnicodeChar == '\r' || key.UnicodeChar == '\n') {
            if (directory != FS_ROOT && selected == 1U) {
                directory = gNodes[directory].parent;
                selected = 0;
                topItem = 0;
            } else {
                UINTN node = nodes[selected - specials];
                if (gNodes[node].type == FS_DIRECTORY) {
                    directory = node;
                    selected = 0;
                    topItem = 0;
                } else {
                    fs_path(node, path, capacity);
                    return 1;
                }
            }
        }
    }
}

static void editor_return_to_shell(UINT8 previousScrollback, const char *message) {
    gST->ConOut->EnableCursor(gST->ConOut, 1);
    gScrollbackEnabled = previousScrollback;
    gST->ConOut->ClearScreen(gST->ConOut);
    settings_use_default_color();
    print(message);
}

static void command_textedit(const char *command) {
    char requestedPath[FS_PATH_BYTES];
    char displayPath[FS_PATH_BYTES];
    char newName[FS_NAME_BYTES];
    const char *argument = command + 8;
    const char *notice = (const char *)0;
    UINTN parent = FS_ROOT;
    UINTN length = 0;
    UINTN cursor = 0;
    UINTN topRow = 0;
    UINTN screenColumns;
    UINTN screenRows;
    UINT8 previousScrollback = gScrollbackEnabled;
    int node;
    int readOnly;
    int modified = 0;
    int discardArmed = 0;
    EFI_INPUT_KEY key;

    while (*argument == ' ') argument++;
    gScrollbackEnabled = 0;
    if (!*argument) {
        if (!editor_file_picker(requestedPath, sizeof(requestedPath))) {
            editor_return_to_shell(previousScrollback, "Text Editor canceled.\n");
            return;
        }
    } else string_copy(requestedPath, argument, sizeof(requestedPath));

    node = fs_resolve(requestedPath);
    if (node >= 0) {
        if (gNodes[node].type != FS_FILE) {
            editor_return_to_shell(previousScrollback, "textedit: path is a directory\n");
            return;
        }
        parent = gNodes[node].parent;
        string_copy(newName, gNodes[node].name, sizeof(newName));
        fs_path((UINTN)node, displayPath, sizeof(displayPath));
        if (!editor_load_text((UINTN)node, &length)) {
            editor_return_to_shell(previousScrollback, "textedit: file is not supported ASCII text\n");
            return;
        }
    } else {
        if (!fs_resolve_parent(requestedPath, &parent, newName)) {
            editor_return_to_shell(previousScrollback, "textedit: invalid path or parent directory\n");
            return;
        }
        string_copy(displayPath, requestedPath, sizeof(displayPath));
        gEditorBuffer[0] = 0;
    }
    readOnly = fs_is_protected(node >= 0 ? (UINTN)node : parent) && !auth_session_is_admin();
    editor_screen_reset();

    for (;;) {
        editor_draw(displayPath, length, cursor, &topRow,
                    readOnly, modified, notice);
        notice = (const char *)0;
        editor_wait_key(&key);
        editor_dimensions(&screenColumns, &screenRows);
        if (key.ScanCode == EDITOR_SCAN_ESCAPE || key.UnicodeChar == 27U) {
            if (modified && !discardArmed) {
                notice = "Unsaved changes: press F2 to save or Esc again to discard.";
                discardArmed = 1;
                continue;
            }
            break;
        }
        if (key.ScanCode == EDITOR_SCAN_F2 || key.UnicodeChar == 19U) {
            int saved;
            if (readOnly) {
                notice = "Read-only: exit, authenticate as an administrator, then reopen to edit.";
                continue;
            }
            if (fs_is_protected(node >= 0 ? (UINTN)node : parent) &&
                !auth_authorize_admin("protected editor save")) {
                notice = "Administrator authentication failed; file not saved.";
                continue;
            }
            saved = editor_save(&node, parent, newName, length);
            if (saved > 0) {
                modified = 0;
                discardArmed = 0;
                fs_path((UINTN)node, displayPath, sizeof(displayPath));
                notice = "Saved authoritative FAT file.";
            } else if (!saved) {
                notice = "Disk save incomplete; persistence not claimed; editor text kept for retry.";
            } else notice = "Save failed: filesystem is full or the path now exists.";
            continue;
        }
        if (key.ScanCode == EDITOR_SCAN_LEFT) {
            if (cursor) cursor--;
        } else if (key.ScanCode == EDITOR_SCAN_RIGHT) {
            if (cursor < length) cursor++;
        } else if (key.ScanCode == EDITOR_SCAN_UP) {
            cursor = editor_move_vertical(
                gEditorBuffer, length, cursor, screenColumns, -1
            );
        } else if (key.ScanCode == EDITOR_SCAN_DOWN) {
            cursor = editor_move_vertical(
                gEditorBuffer, length, cursor, screenColumns, 1
            );
        } else if (readOnly && (key.ScanCode == EDITOR_SCAN_DELETE || key.UnicodeChar)) {
            notice = "Read-only: exit, authenticate as an administrator, then reopen to edit.";
        } else if (key.ScanCode == EDITOR_SCAN_DELETE) {
            if (cursor < length) {
                editor_delete(gEditorBuffer, &length, cursor);
                modified = 1;
                discardArmed = 0;
            }
        } else if (key.UnicodeChar == 8U || key.UnicodeChar == 127U) {
            if (cursor) {
                cursor--;
                editor_delete(gEditorBuffer, &length, cursor);
                modified = 1;
                discardArmed = 0;
            }
        } else if (key.UnicodeChar == '\r' || key.UnicodeChar == '\n') {
            if (editor_insert(gEditorBuffer, &length, &cursor, '\n')) {
                modified = 1;
                discardArmed = 0;
            } else notice = "File limit reached; delete text before inserting more.";
        } else if (key.UnicodeChar >= 32U && key.UnicodeChar <= 126U) {
            if (editor_insert(gEditorBuffer, &length, &cursor, (char)key.UnicodeChar)) {
                modified = 1;
                discardArmed = 0;
            } else notice = "File limit reached; delete text before inserting more.";
        }
    }
    editor_return_to_shell(previousScrollback, "Exited Text Editor.\n");
}
