#define EDITOR_SCAN_UP        1U
#define EDITOR_SCAN_DOWN      2U
#define EDITOR_SCAN_RIGHT     3U
#define EDITOR_SCAN_LEFT      4U
#define EDITOR_SCAN_HOME      5U
#define EDITOR_SCAN_END       6U
#define EDITOR_SCAN_DELETE    8U
#define EDITOR_SCAN_PAGE_UP   9U
#define EDITOR_SCAN_PAGE_DOWN 10U
#define EDITOR_SCAN_F2        12U
#define EDITOR_SCAN_ESCAPE    23U

static char gEditorBuffer[FS_DATA_BYTES];
static FS_NODE gEditorSaveBackup;

static UINTN editor_line_start(const char *buffer, UINTN cursor) {
    while (cursor && buffer[cursor - 1U] != '\n') cursor--;
    return cursor;
}

static UINTN editor_line_end(const char *buffer, UINTN length, UINTN cursor) {
    while (cursor < length && buffer[cursor] != '\n') cursor++;
    return cursor;
}

static UINTN editor_line_number(const char *buffer, UINTN cursor) {
    UINTN index;
    UINTN line = 0;
    for (index = 0; index < cursor; index++) if (buffer[index] == '\n') line++;
    return line;
}

static UINTN editor_line_offset(const char *buffer, UINTN length, UINTN wantedLine) {
    UINTN offset = 0;
    UINTN line = 0;
    while (line < wantedLine) {
        while (offset < length && buffer[offset] != '\n') offset++;
        if (offset >= length) return length + 1U;
        offset++;
        line++;
    }
    return offset;
}

static UINTN editor_move_vertical(
    const char *buffer, UINTN length, UINTN cursor, int direction
) {
    UINTN start = editor_line_start(buffer, cursor);
    UINTN column = cursor - start;
    UINTN targetStart;
    UINTN targetEnd;
    if (direction < 0) {
        if (!start) return cursor;
        targetEnd = start - 1U;
        targetStart = editor_line_start(buffer, targetEnd);
    } else {
        UINTN end = editor_line_end(buffer, length, cursor);
        if (end >= length) return cursor;
        targetStart = end + 1U;
        targetEnd = editor_line_end(buffer, length, targetStart);
    }
    if (column > targetEnd - targetStart) column = targetEnd - targetStart;
    return targetStart + column;
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
    UINTN input;
    UINTN output = 0;
    if (gNodes[node].size >= FS_DATA_BYTES) return 0;
    for (input = 0; input < gNodes[node].size; input++) {
        UINT8 ch = (UINT8)gNodes[node].data[input];
        if (ch == '\r') {
            if (input + 1U < gNodes[node].size && gNodes[node].data[input + 1U] == '\n')
                input++;
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
    *columnsOut = columns;
    *rowsOut = rows;
}

static void editor_write_row(UINTN row, const char *text, UINTN columns) {
    CHAR16 output[SCROLLBACK_COLUMNS];
    UINTN used = 0;
    while (text[used] && used < columns) {
        output[used] = (CHAR16)(UINT8)text[used];
        used++;
    }
    output[used] = 0;
    gST->ConOut->SetCursorPosition(gST->ConOut, 0, row);
    out16(output);
}

static void editor_draw(
    const char *path, UINTN length, UINTN cursor, UINTN *topLine,
    UINTN *leftColumn, int readOnly, int modified, const char *notice
) {
    char pathLine[SCROLLBACK_COLUMNS];
    char visible[SCROLLBACK_COLUMNS];
    UINTN columns;
    UINTN rows;
    UINTN contentRows;
    UINTN cursorLine = editor_line_number(gEditorBuffer, cursor);
    UINTN cursorStart = editor_line_start(gEditorBuffer, cursor);
    UINTN cursorColumn = cursor - cursorStart;
    UINTN row;
    editor_dimensions(&columns, &rows);
    contentRows = rows - 4U;
    if (cursorLine < *topLine) *topLine = cursorLine;
    else if (cursorLine >= *topLine + contentRows)
        *topLine = cursorLine - contentRows + 1U;
    if (cursorColumn < *leftColumn) *leftColumn = cursorColumn;
    else if (cursorColumn >= *leftColumn + columns)
        *leftColumn = cursorColumn - columns + 1U;

    gST->ConOut->ClearScreen(gST->ConOut);
    settings_use_accent_color();
    editor_write_row(0, "=== TinyArmOS Text Editor ===", columns);
    settings_use_default_color();
    pathLine[0] = 0;
    string_append(pathLine, path, sizeof(pathLine));
    if (readOnly) string_append(pathLine, "  [READ ONLY]", sizeof(pathLine));
    if (modified) string_append(pathLine, "  *", sizeof(pathLine));
    editor_write_row(1, pathLine, columns);

    for (row = 0; row < contentRows; row++) {
        UINTN offset = editor_line_offset(gEditorBuffer, length, *topLine + row);
        UINTN skipped = 0;
        UINTN used = 0;
        if (offset <= length) {
            while (offset < length && gEditorBuffer[offset] != '\n' && skipped < *leftColumn) {
                offset++;
                skipped++;
            }
            while (offset < length && gEditorBuffer[offset] != '\n' && used < columns) {
                char ch = gEditorBuffer[offset++];
                visible[used++] = ch == '\t' ? ' ' : ch;
            }
        }
        visible[used] = 0;
        editor_write_row(2U + row, visible, columns);
    }
    editor_write_row(rows - 2U, notice ? notice : "ASCII text; maximum file size is 8191 bytes.", columns);
    settings_use_accent_color();
    editor_write_row(rows - 1U, "F2/Ctrl+S Save | Esc Exit | Arrows Move | Home/End | PgUp/PgDn", columns);
    settings_use_default_color();
    gST->ConOut->SetCursorPosition(
        gST->ConOut, cursorColumn - *leftColumn, 2U + cursorLine - *topLine
    );
    gST->ConOut->EnableCursor(gST->ConOut, 1);
}

static int editor_save(int *node, UINTN parent, const char *name, UINTN length) {
    UINT64 previousGeneration = gGeneration;
    int created = *node < 0;
    if (!created) memory_copy(&gEditorSaveBackup, &gNodes[*node], sizeof(FS_NODE));
    else *node = fs_alloc(FS_FILE, parent, name, 0);
    if (*node < 0 || gNodes[*node].type != FS_FILE) return -1;
    memory_zero(gNodes[*node].data, FS_DATA_BYTES);
    memory_copy(gNodes[*node].data, gEditorBuffer, length);
    gNodes[*node].size = length;
    fs_update((UINTN)*node);
    if (fs_commit()) return 1;
    if (created) {
        memory_zero(&gNodes[*node], sizeof(FS_NODE));
        *node = -1;
    } else memory_copy(&gNodes[*node], &gEditorSaveBackup, sizeof(FS_NODE));
    gGeneration = previousGeneration;
    return 0;
}

static void command_editor(const char *command) {
    char requestedPath[FS_PATH_BYTES];
    char displayPath[FS_PATH_BYTES];
    char newName[FS_NAME_BYTES];
    const char *argument = command + 4;
    const char *notice = (const char *)0;
    UINTN parent = FS_ROOT;
    UINTN length = 0;
    UINTN cursor = 0;
    UINTN topLine = 0;
    UINTN leftColumn = 0;
    UINTN contentRows;
    UINTN screenColumns;
    UINT8 previousScrollback;
    int node;
    int readOnly;
    int modified = 0;
    int discardArmed = 0;
    EFI_INPUT_KEY key;

    while (*argument == ' ') argument++;
    if (!*argument) {
        print("File path (existing or new; blank cancels): ");
        read_line(requestedPath, sizeof(requestedPath));
        argument = requestedPath;
        while (*argument == ' ') argument++;
        if (!*argument) {
            print("editor cancelled\n");
            return;
        }
    } else string_copy(requestedPath, argument, sizeof(requestedPath));

    node = fs_resolve(argument);
    if (node >= 0) {
        if (gNodes[node].type != FS_FILE) {
            print("edit: path is a directory\n");
            return;
        }
        parent = gNodes[node].parent;
        string_copy(newName, gNodes[node].name, sizeof(newName));
        fs_path((UINTN)node, displayPath, sizeof(displayPath));
        if (!editor_load_text((UINTN)node, &length)) {
            print("edit: file is not supported ASCII text\n");
            return;
        }
    } else {
        if (!fs_resolve_parent(argument, &parent, newName)) {
            print("edit: invalid path or parent directory\n");
            return;
        }
        string_copy(displayPath, argument, sizeof(displayPath));
        gEditorBuffer[0] = 0;
    }
    readOnly = fs_is_protected(node >= 0 ? (UINTN)node : parent) && !gProtectionUnlocked;
    previousScrollback = gScrollbackEnabled;
    gScrollbackEnabled = 0;

    for (;;) {
        editor_draw(displayPath, length, cursor, &topLine, &leftColumn,
                    readOnly, modified, notice);
        notice = (const char *)0;
        if (gST->ConIn->ReadKeyStroke(gST->ConIn, &key) != EFI_SUCCESS) {
            __asm__ volatile("yield");
            continue;
        }
        editor_dimensions(&screenColumns, &contentRows);
        contentRows -= 4U;
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
                notice = "Read-only: exit, run 'protect unlock', then reopen to edit.";
                continue;
            }
            saved = editor_save(&node, parent, newName, length);
            if (saved > 0) {
                modified = 0;
                discardArmed = 0;
                fs_path((UINTN)node, displayPath, sizeof(displayPath));
                notice = "Saved persistent MiniFS snapshot.";
            } else if (!saved) {
                notice = "Snapshot save failed; original file restored; editor text kept.";
            } else notice = "Save failed: filesystem is full or the path now exists.";
            continue;
        }
        if (key.ScanCode == EDITOR_SCAN_LEFT) {
            if (cursor) cursor--;
        } else if (key.ScanCode == EDITOR_SCAN_RIGHT) {
            if (cursor < length) cursor++;
        } else if (key.ScanCode == EDITOR_SCAN_UP) {
            cursor = editor_move_vertical(gEditorBuffer, length, cursor, -1);
        } else if (key.ScanCode == EDITOR_SCAN_DOWN) {
            cursor = editor_move_vertical(gEditorBuffer, length, cursor, 1);
        } else if (key.ScanCode == EDITOR_SCAN_HOME) {
            cursor = editor_line_start(gEditorBuffer, cursor);
        } else if (key.ScanCode == EDITOR_SCAN_END) {
            cursor = editor_line_end(gEditorBuffer, length, cursor);
        } else if (key.ScanCode == EDITOR_SCAN_PAGE_UP) {
            UINTN count;
            for (count = 0; count < contentRows; count++)
                cursor = editor_move_vertical(gEditorBuffer, length, cursor, -1);
        } else if (key.ScanCode == EDITOR_SCAN_PAGE_DOWN) {
            UINTN count;
            for (count = 0; count < contentRows; count++)
                cursor = editor_move_vertical(gEditorBuffer, length, cursor, 1);
        } else if (readOnly && (key.ScanCode == EDITOR_SCAN_DELETE || key.UnicodeChar)) {
            notice = "Read-only: exit, run 'protect unlock', then reopen to edit.";
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
    gST->ConOut->EnableCursor(gST->ConOut, 1);
    gScrollbackEnabled = previousScrollback;
    gST->ConOut->ClearScreen(gST->ConOut);
    settings_use_default_color();
    print("Exited Text Editor.\n");
}
