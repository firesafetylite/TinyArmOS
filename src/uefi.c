// TinyArmOS: a freestanding ARM64 UEFI shell with a persistent mini filesystem.
typedef unsigned char      UINT8;
typedef unsigned short     UINT16;
typedef unsigned int       UINT32;
typedef unsigned long long UINT64;
typedef unsigned long long UINTN;
typedef int                INT32;
typedef UINT16             CHAR16;
typedef void              *EFI_HANDLE;
typedef UINT64             EFI_STATUS;

#define EFI_SUCCESS   0
#define EFI_NOT_READY (0x8000000000000000ULL | 6ULL)
#define EFIAPI

struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
struct EFI_RUNTIME_SERVICES;
struct EFI_FILE_PROTOCOL;

typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8 Data4[8];
} EFI_GUID;

typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

typedef EFI_STATUS (EFIAPI *EFI_INPUT_RESET)(struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL *, UINT8);
typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY)(struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL *, EFI_INPUT_KEY *);

typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_INPUT_RESET Reset;
    EFI_INPUT_READ_KEY ReadKeyStroke;
    void *WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_TEXT_RESET)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, UINT8);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, CHAR16 *);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_TEST_STRING)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, CHAR16 *);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_QUERY_MODE)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, UINTN, UINTN *, UINTN *);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_MODE)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, UINTN);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, UINTN);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_CURSOR_POSITION)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, UINTN, UINTN);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_ENABLE_CURSOR)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, UINT8);

typedef struct {
    INT32 MaxMode;
    INT32 Mode;
    INT32 Attribute;
    INT32 CursorColumn;
    INT32 CursorRow;
    UINT8 CursorVisible;
} EFI_SIMPLE_TEXT_OUTPUT_MODE;

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_TEXT_RESET Reset;
    EFI_TEXT_STRING OutputString;
    EFI_TEXT_TEST_STRING TestString;
    EFI_TEXT_QUERY_MODE QueryMode;
    EFI_TEXT_SET_MODE SetMode;
    EFI_TEXT_SET_ATTRIBUTE SetAttribute;
    EFI_TEXT_CLEAR_SCREEN ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
    EFI_TEXT_ENABLE_CURSOR EnableCursor;
    EFI_SIMPLE_TEXT_OUTPUT_MODE *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef enum {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

typedef void (EFIAPI *EFI_RESET_SYSTEM)(EFI_RESET_TYPE, EFI_STATUS, UINTN, void *);

typedef struct EFI_RUNTIME_SERVICES {
    EFI_TABLE_HEADER Hdr;
    void *GetTime;
    void *SetTime;
    void *GetWakeupTime;
    void *SetWakeupTime;
    void *SetVirtualAddressMap;
    void *ConvertPointer;
    void *GetVariable;
    void *GetNextVariableName;
    void *SetVariable;
    void *GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM ResetSystem;
} EFI_RUNTIME_SERVICES;

typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(UINT32, UINTN, void **);
typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(void *);
typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(EFI_HANDLE, EFI_GUID *, void **);
typedef EFI_STATUS (EFIAPI *EFI_SET_WATCHDOG_TIMER)(UINTN, UINT64, UINTN, CHAR16 *);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(EFI_GUID *, void *, void **);
typedef struct {
    EFI_TABLE_HEADER Hdr;
    void *BeforeAllocatePool[5];
    EFI_ALLOCATE_POOL AllocatePool;
    EFI_FREE_POOL FreePool;
    void *BeforeHandleProtocol[9];
    EFI_HANDLE_PROTOCOL HandleProtocol;
    void *BeforeSetWatchdogTimer[12];
    EFI_SET_WATCHDOG_TIMER SetWatchdogTimer;
    void *BeforeLocateProtocol[7];
    EFI_LOCATE_PROTOCOL LocateProtocol;
} EFI_BOOT_SERVICES_PREFIX;

typedef struct {
    UINT32 Revision;
    EFI_HANDLE ParentHandle;
    void *SystemTable;
    EFI_HANDLE DeviceHandle;
    void *FilePath;
    void *Reserved;
    UINT32 LoadOptionsSize;
    void *LoadOptions;
    void *ImageBase;
    UINT64 ImageSize;
    UINT32 ImageCodeType;
    UINT32 ImageDataType;
    void *Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_FILE_OPEN)(struct EFI_FILE_PROTOCOL *, struct EFI_FILE_PROTOCOL **, CHAR16 *, UINT64, UINT64);
typedef EFI_STATUS (EFIAPI *EFI_FILE_CLOSE)(struct EFI_FILE_PROTOCOL *);
typedef EFI_STATUS (EFIAPI *EFI_FILE_DELETE)(struct EFI_FILE_PROTOCOL *);
typedef EFI_STATUS (EFIAPI *EFI_FILE_READ)(struct EFI_FILE_PROTOCOL *, UINTN *, void *);
typedef EFI_STATUS (EFIAPI *EFI_FILE_WRITE)(struct EFI_FILE_PROTOCOL *, UINTN *, void *);
typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_POSITION)(struct EFI_FILE_PROTOCOL *, UINT64 *);
typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_POSITION)(struct EFI_FILE_PROTOCOL *, UINT64);
typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_INFO)(struct EFI_FILE_PROTOCOL *, EFI_GUID *, UINTN *, void *);
typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_INFO)(struct EFI_FILE_PROTOCOL *, EFI_GUID *, UINTN, void *);
typedef EFI_STATUS (EFIAPI *EFI_FILE_FLUSH)(struct EFI_FILE_PROTOCOL *);

typedef struct EFI_FILE_PROTOCOL {
    UINT64 Revision;
    EFI_FILE_OPEN Open;
    EFI_FILE_CLOSE Close;
    EFI_FILE_DELETE Delete;
    EFI_FILE_READ Read;
    EFI_FILE_WRITE Write;
    EFI_FILE_GET_POSITION GetPosition;
    EFI_FILE_SET_POSITION SetPosition;
    EFI_FILE_GET_INFO GetInfo;
    EFI_FILE_SET_INFO SetInfo;
    EFI_FILE_FLUSH Flush;
} EFI_FILE_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_OPEN_VOLUME)(void *, EFI_FILE_PROTOCOL **);
typedef struct {
    UINT64 Revision;
    EFI_OPEN_VOLUME OpenVolume;
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef struct {
    UINT32 RedMask;
    UINT32 GreenMask;
    UINT32 BlueMask;
    UINT32 ReservedMask;
} EFI_PIXEL_BITMASK;

typedef struct {
    UINT32 Version;
    UINT32 HorizontalResolution;
    UINT32 VerticalResolution;
    UINT32 PixelFormat;
    EFI_PIXEL_BITMASK PixelInformation;
    UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    UINT32 MaxMode;
    UINT32 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN SizeOfInfo;
    UINT64 FrameBufferBase;
    UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct {
    UINT8 Blue;
    UINT8 Green;
    UINT8 Red;
    UINT8 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

struct EFI_GRAPHICS_OUTPUT_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_QUERY_MODE)(struct EFI_GRAPHICS_OUTPUT_PROTOCOL *, UINT32, UINTN *, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **);
typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_SET_MODE)(struct EFI_GRAPHICS_OUTPUT_PROTOCOL *, UINT32);
typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_BLT)(struct EFI_GRAPHICS_OUTPUT_PROTOCOL *, EFI_GRAPHICS_OUTPUT_BLT_PIXEL *, UINT32, UINTN, UINTN, UINTN, UINTN, UINTN, UINTN, UINTN);
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_GRAPHICS_QUERY_MODE QueryMode;
    EFI_GRAPHICS_SET_MODE SetMode;
    EFI_GRAPHICS_BLT Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    EFI_RUNTIME_SERVICES *RuntimeServices;
    EFI_BOOT_SERVICES_PREFIX *BootServices;
    UINTN NumberOfTableEntries;
    void *ConfigurationTable;
} EFI_SYSTEM_TABLE;

#define EFI_FILE_MODE_READ   0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE  0x0000000000000002ULL
#define EFI_FILE_MODE_CREATE 0x8000000000000000ULL

#define FS_MAX_NODES  96
#define FS_NAME_BYTES 32
#define FS_DATA_BYTES 8192
#define FS_PATH_BYTES 192
#define SCROLLBACK_LINES 256
#define SCROLLBACK_COLUMNS 160
#define FS_ROOT        0
#define FS_FILE        1
#define FS_DIRECTORY   2
#define FS_PROTECTED   1
#define FS_IMAGE_MAGIC 0x3253464d52415954ULL
#define FS_IMAGE_VERSION 3

static EFI_SYSTEM_TABLE *gST;
static EFI_FILE_PROTOCOL *gVolumeRoot;
static UINT64 gStartTicks;
static UINT64 gTimerHz;
static UINT64 gCommands;
static UINT64 gGeneration;
static UINT64 gSlotGeneration[2];
static UINT8 gSlotValid[2];
static UINT8 gStorageReady;
static UINT8 gProtectionUnlocked;
static UINTN gCwd;
static UINTN gPreviousCwd;
static char gScrollback[SCROLLBACK_LINES][SCROLLBACK_COLUMNS];
static UINTN gScrollbackCount;
static UINTN gScrollbackLength;
static UINTN gScrollbackOffset;
static UINTN gConsoleColumns;
static UINTN gConsoleRows;
static UINT8 gScrollbackWrapped[SCROLLBACK_LINES];
static UINT8 gScrollbackEnabled;

typedef struct {
    UINT8 used;
    UINT8 type;
    UINT8 flags;
    UINT8 reserved;
    UINTN parent;
    UINTN size;
    UINT32 checksum;
    char name[FS_NAME_BYTES];
    char data[FS_DATA_BYTES];
} FS_NODE;

typedef struct {
    UINT64 magic;
    UINT32 version;
    UINT32 nodeCount;
    UINT64 generation;
    UINT64 payloadBytes;
    UINT32 payloadChecksum;
    UINT32 reserved;
} FS_IMAGE_HEADER;

static FS_NODE gNodes[FS_MAX_NODES];
static FS_NODE gLoadBuffer[FS_MAX_NODES];

static UINT64 timer_count(void) {
    UINT64 value;
    __asm__ volatile("isb; mrs %0, cntpct_el0" : "=r"(value));
    return value;
}

static UINT64 timer_frequency(void) {
    UINT64 value;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(value));
    return value;
}

static UINT64 current_el(void) {
    UINT64 value;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(value));
    return value >> 2;
}

static void delay_ms(UINT64 milliseconds) {
    UINT64 ticks = gTimerHz ? (gTimerHz / 1000) * milliseconds : 0;
    UINT64 deadline = timer_count() + ticks;
    while (timer_count() < deadline) __asm__ volatile("yield");
}

static void memory_zero(void *destination, UINTN bytes) {
    UINT8 *out = (UINT8 *)destination;
    while (bytes--) *out++ = 0;
}

static void memory_copy(void *destination, const void *source, UINTN bytes) {
    UINT8 *out = (UINT8 *)destination;
    const UINT8 *in = (const UINT8 *)source;
    while (bytes--) *out++ = *in++;
}

static UINT32 hash_bytes(const void *data, UINTN bytes) {
    const UINT8 *cursor = (const UINT8 *)data;
    UINT32 hash = 2166136261U;
    while (bytes--) {
        hash ^= *cursor++;
        hash *= 16777619U;
    }
    return hash;
}

static void out16(CHAR16 *text) {
    gST->ConOut->OutputString(gST->ConOut, text);
}

static void console_write_raw(const char *text) {
    CHAR16 buffer[128];
    UINTN used = 0;
    while (*text) {
        char ch = *text++;
        if (ch == '\n') {
            if (used == 126) {
                buffer[used] = 0;
                out16(buffer);
                used = 0;
            }
            buffer[used++] = '\r';
        }
        buffer[used++] = (CHAR16)(UINT8)ch;
        if (used >= 126) {
            buffer[used] = 0;
            out16(buffer);
            used = 0;
        }
    }
    if (used) {
        buffer[used] = 0;
        out16(buffer);
    }
}

static void scrollback_reset(void) {
    memory_zero(gScrollback, sizeof(gScrollback));
    memory_zero(gScrollbackWrapped, sizeof(gScrollbackWrapped));
    gScrollbackCount = 1;
    gScrollbackLength = 0;
    gScrollbackOffset = 0;
}

static void scrollback_new_line(UINT8 wrapped) {
    UINTN index;
    if (gScrollbackCount < SCROLLBACK_LINES) {
        gScrollbackWrapped[gScrollbackCount] = wrapped;
        gScrollbackCount++;
    } else {
        for (index = 1; index < SCROLLBACK_LINES; index++) {
            memory_copy(gScrollback[index - 1], gScrollback[index], SCROLLBACK_COLUMNS);
            gScrollbackWrapped[index - 1] = gScrollbackWrapped[index];
        }
        memory_zero(gScrollback[SCROLLBACK_LINES - 1], SCROLLBACK_COLUMNS);
        gScrollbackWrapped[SCROLLBACK_LINES - 1] = wrapped;
    }
    gScrollbackLength = 0;
}

static void scrollback_capture_char(char ch) {
    char *line;
    if (!gScrollbackEnabled || !gScrollbackCount) return;
    line = gScrollback[gScrollbackCount - 1];
    if (ch == '\r') return;
    if (ch == '\n') {
        scrollback_new_line(0);
        return;
    }
    if (ch == '\b') {
        if (gScrollbackLength) {
            line[--gScrollbackLength] = 0;
        } else if (gScrollbackCount > 1 && gScrollbackWrapped[gScrollbackCount - 1]) {
            UINTN length = 0;
            gScrollbackCount--;
            line = gScrollback[gScrollbackCount - 1];
            while (line[length] && length + 1 < SCROLLBACK_COLUMNS) length++;
            gScrollbackLength = length;
        }
        return;
    }
    if ((UINT8)ch < 32) return;
    if (gScrollbackLength >= gConsoleColumns || gScrollbackLength + 1 >= SCROLLBACK_COLUMNS) {
        scrollback_new_line(1);
        line = gScrollback[gScrollbackCount - 1];
    }
    line[gScrollbackLength++] = ch;
    line[gScrollbackLength] = 0;
}

static void scrollback_capture(const char *text) {
    while (*text) scrollback_capture_char(*text++);
}

static void scrollback_render(void) {
    UINTN visible = gConsoleRows > 2 ? gConsoleRows - (gScrollbackOffset ? 1 : 0) : 20;
    UINTN end;
    UINTN start;
    UINTN index;
    if (!gScrollbackEnabled || !gScrollbackCount) return;
    if (gScrollbackOffset >= gScrollbackCount) gScrollbackOffset = gScrollbackCount - 1;
    end = gScrollbackCount - gScrollbackOffset;
    start = end > visible ? end - visible : 0;
    gST->ConOut->ClearScreen(gST->ConOut);
    for (index = start; index < end; index++) {
        console_write_raw(gScrollback[index]);
        if (index + 1 < end || gScrollbackOffset) console_write_raw("\n");
    }
    if (gScrollbackOffset) console_write_raw("-- SCROLLBACK: Up/Down line, PageUp/PageDown page, End/Esc live --");
}

static void scrollback_move(int direction, UINTN lines) {
    UINTN page = gConsoleRows > 4 ? gConsoleRows - 2 : 10;
    UINTN maxOffset = gScrollbackCount > page ? gScrollbackCount - page : 0;
    if (direction < 0) {
        if (gScrollbackOffset + lines > maxOffset) gScrollbackOffset = maxOffset;
        else gScrollbackOffset += lines;
    } else {
        if (gScrollbackOffset > lines) gScrollbackOffset -= lines;
        else gScrollbackOffset = 0;
    }
    scrollback_render();
}

static void scrollback_page(int direction) {
    scrollback_move(direction, gConsoleRows > 4 ? gConsoleRows - 2 : 10);
}

static void scrollback_enable(void) {
    UINTN columns = 80;
    UINTN rows = 25;
    UINTN mode = 0;
    if (gST->ConOut->Mode && gST->ConOut->Mode->Mode >= 0) mode = (UINTN)gST->ConOut->Mode->Mode;
    if (gST->ConOut->QueryMode(gST->ConOut, mode, &columns, &rows) != EFI_SUCCESS) {
        columns = 80;
        rows = 25;
    }
    if (columns < 20) columns = 20;
    if (columns >= SCROLLBACK_COLUMNS) columns = SCROLLBACK_COLUMNS - 1;
    if (rows < 5) rows = 25;
    gConsoleColumns = columns;
    gConsoleRows = rows;
    gScrollbackEnabled = 1;
    scrollback_reset();
}

static void print(const char *text) {
    if (gScrollbackEnabled) scrollback_capture(text);
    console_write_raw(text);
}

static void print_char(char ch) {
    CHAR16 text[2];
    if (gScrollbackEnabled) scrollback_capture_char(ch);
    text[0] = (CHAR16)(UINT8)ch;
    text[1] = 0;
    out16(text);
}

static void print_u64(UINT64 value) {
    char digits[32];
    UINTN count = 0;
    if (!value) {
        print("0");
        return;
    }
    while (value) {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    }
    while (count) print_char(digits[--count]);
}

static void print_hex(UINT64 value) {
    const char *hex = "0123456789abcdef";
    int shift;
    int started = 0;
    print("0x");
    for (shift = 60; shift >= 0; shift -= 4) {
        UINT8 digit = (UINT8)((value >> shift) & 15);
        if (digit || started || !shift) {
            print_char(hex[digit]);
            started = 1;
        }
    }
}

static UINTN string_length(const char *text) {
    UINTN length = 0;
    while (text[length]) length++;
    return length;
}

static int streq(const char *left, const char *right) {
    while (*left && *right && *left == *right) {
        left++;
        right++;
    }
    return *left == *right;
}

static int starts_with(const char *text, const char *prefix) {
    while (*prefix) if (*text++ != *prefix++) return 0;
    return 1;
}

static void string_copy(char *destination, const char *source, UINTN capacity) {
    UINTN index = 0;
    if (!capacity) return;
    while (source[index] && index + 1 < capacity) {
        destination[index] = source[index];
        index++;
    }
    destination[index] = 0;
}

static void string_append(char *destination, const char *source, UINTN capacity) {
    UINTN used = string_length(destination);
    if (used < capacity) string_copy(destination + used, source, capacity - used);
}

static char *skip_spaces(char *text) {
    while (*text == ' ') text++;
    return text;
}

static char *next_argument(char *text, char **remainder) {
    char *start = skip_spaces(text);
    char *end = start;
    if (!*start) {
        *remainder = start;
        return (char *)0;
    }
    while (*end && *end != ' ') end++;
    if (*end) {
        *end = 0;
        *remainder = skip_spaces(end + 1);
    } else {
        *remainder = end;
    }
    return start;
}

static const CHAR16 gSlot0Path[] = {'\\','T','I','N','Y','F','S','0','.','B','I','N',0};
static const CHAR16 gSlot1Path[] = {'\\','T','I','N','Y','F','S','1','.','B','I','N',0};

static int storage_init(EFI_HANDLE imageHandle) {
    static EFI_GUID loadedImageGuid = {0x5b1b31a1, 0x9562, 0x11d2, {0x8e,0x3f,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
    static EFI_GUID simpleFsGuid = {0x964e5b22, 0x6459, 0x11d2, {0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
    EFI_LOADED_IMAGE_PROTOCOL *loadedImage = (EFI_LOADED_IMAGE_PROTOCOL *)0;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *filesystem = (EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *)0;
    EFI_STATUS status;
    status = gST->BootServices->HandleProtocol(imageHandle, &loadedImageGuid, (void **)&loadedImage);
    if (status != EFI_SUCCESS || !loadedImage) return 0;
    status = gST->BootServices->HandleProtocol(loadedImage->DeviceHandle, &simpleFsGuid, (void **)&filesystem);
    if (status != EFI_SUCCESS || !filesystem) return 0;
    status = filesystem->OpenVolume(filesystem, &gVolumeRoot);
    return status == EFI_SUCCESS && gVolumeRoot;
}

static int storage_read_slot(UINTN slot, FS_NODE *output, UINT64 *generation) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    FS_IMAGE_HEADER header;
    UINTN bytes;
    EFI_STATUS status;
    CHAR16 *path = (CHAR16 *)(slot ? gSlot1Path : gSlot0Path);
    if (!gStorageReady) return 0;
    status = gVolumeRoot->Open(gVolumeRoot, &file, path, EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS || !file) return 0;
    bytes = sizeof(header);
    status = file->Read(file, &bytes, &header);
    if (status != EFI_SUCCESS || bytes != sizeof(header) || header.magic != FS_IMAGE_MAGIC ||
        header.version != FS_IMAGE_VERSION || header.nodeCount != FS_MAX_NODES ||
        header.payloadBytes != sizeof(gNodes)) {
        file->Close(file);
        return 0;
    }
    bytes = sizeof(gNodes);
    status = file->Read(file, &bytes, output);
    file->Close(file);
    if (status != EFI_SUCCESS || bytes != sizeof(gNodes)) return 0;
    if (hash_bytes(output, sizeof(gNodes)) != header.payloadChecksum) return 0;
    *generation = header.generation;
    return 1;
}

static void storage_probe_slots(void) {
    UINTN slot;
    for (slot = 0; slot < 2; slot++) {
        UINT64 generation = 0;
        gSlotValid[slot] = (UINT8)storage_read_slot(slot, gLoadBuffer, &generation);
        gSlotGeneration[slot] = gSlotValid[slot] ? generation : 0;
    }
}

static int storage_mount_latest(void) {
    UINTN slot;
    UINTN best = 2;
    storage_probe_slots();
    for (slot = 0; slot < 2; slot++) {
        if (gSlotValid[slot] && (best == 2 || gSlotGeneration[slot] > gSlotGeneration[best])) best = slot;
    }
    if (best == 2 || !storage_read_slot(best, gLoadBuffer, &gGeneration)) return 0;
    memory_copy(gNodes, gLoadBuffer, sizeof(gNodes));
    return 1;
}

static int storage_sync(void) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    FS_IMAGE_HEADER header;
    UINTN bytes;
    UINTN slot;
    EFI_STATUS status;
    CHAR16 *path;
    if (!gStorageReady) return 0;
    gGeneration++;
    slot = (UINTN)(gGeneration & 1ULL);
    path = (CHAR16 *)(slot ? gSlot1Path : gSlot0Path);
    header.magic = FS_IMAGE_MAGIC;
    header.version = FS_IMAGE_VERSION;
    header.nodeCount = FS_MAX_NODES;
    header.generation = gGeneration;
    header.payloadBytes = sizeof(gNodes);
    header.payloadChecksum = hash_bytes(gNodes, sizeof(gNodes));
    header.reserved = 0;
    status = gVolumeRoot->Open(gVolumeRoot, &file, path,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (status != EFI_SUCCESS || !file) return 0;
    file->SetPosition(file, 0);
    bytes = sizeof(header);
    status = file->Write(file, &bytes, &header);
    if (status == EFI_SUCCESS && bytes == sizeof(header)) {
        bytes = sizeof(gNodes);
        status = file->Write(file, &bytes, gNodes);
    }
    if (status == EFI_SUCCESS) file->Flush(file);
    file->Close(file);
    if (status != EFI_SUCCESS || bytes != sizeof(gNodes)) return 0;
    storage_probe_slots();
    return 1;
}

static int storage_rollback(void) {
    UINTN older;
    UINT64 loadedGeneration;
    storage_probe_slots();
    if (!gSlotValid[0] || !gSlotValid[1]) return 0;
    older = gSlotGeneration[0] < gSlotGeneration[1] ? 0 : 1;
    if (!storage_read_slot(older, gLoadBuffer, &loadedGeneration)) return 0;
    memory_copy(gNodes, gLoadBuffer, sizeof(gNodes));
    gGeneration = gSlotGeneration[0] > gSlotGeneration[1] ? gSlotGeneration[0] : gSlotGeneration[1];
    gCwd = FS_ROOT;
    return 1;
}

static UINT32 fs_node_checksum(const FS_NODE *node) {
    UINT32 hash = 2166136261U;
    UINTN index;
    const UINT8 metadata[] = {node->used, node->type, node->flags};
    for (index = 0; index < sizeof(metadata); index++) {
        hash ^= metadata[index];
        hash *= 16777619U;
    }
    for (index = 0; index < sizeof(node->parent); index++) {
        hash ^= (UINT8)(node->parent >> (index * 8));
        hash *= 16777619U;
    }
    for (index = 0; index < sizeof(node->size); index++) {
        hash ^= (UINT8)(node->size >> (index * 8));
        hash *= 16777619U;
    }
    for (index = 0; index < FS_NAME_BYTES && node->name[index]; index++) {
        hash ^= (UINT8)node->name[index];
        hash *= 16777619U;
    }
    if (node->type == FS_FILE) {
        UINTN size = node->size < FS_DATA_BYTES ? node->size : FS_DATA_BYTES;
        for (index = 0; index < size; index++) {
            hash ^= (UINT8)node->data[index];
            hash *= 16777619U;
        }
    }
    return hash;
}

static void fs_update(UINTN node) {
    gNodes[node].checksum = fs_node_checksum(&gNodes[node]);
}

static int fs_valid_name(const char *name) {
    UINTN length = 0;
    if (!name || !*name || streq(name, ".") || streq(name, "..")) return 0;
    while (*name) {
        char ch = *name++;
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' || ch == '-' || ch == '+')) return 0;
        if (++length >= FS_NAME_BYTES) return 0;
    }
    return 1;
}

static int fs_find_child(UINTN parent, const char *name) {
    UINTN index;
    for (index = 1; index < FS_MAX_NODES; index++) {
        if (gNodes[index].used && gNodes[index].parent == parent && streq(gNodes[index].name, name)) return (int)index;
    }
    return -1;
}

static int fs_alloc(UINT8 type, UINTN parent, const char *name, UINT8 flags) {
    UINTN index;
    if (parent >= FS_MAX_NODES || !gNodes[parent].used || gNodes[parent].type != FS_DIRECTORY ||
        !fs_valid_name(name) || fs_find_child(parent, name) >= 0) return -1;
    for (index = 1; index < FS_MAX_NODES; index++) {
        if (!gNodes[index].used) {
            memory_zero(&gNodes[index], sizeof(FS_NODE));
            gNodes[index].used = 1;
            gNodes[index].type = type;
            gNodes[index].flags = flags;
            gNodes[index].parent = parent;
            string_copy(gNodes[index].name, name, FS_NAME_BYTES);
            fs_update(index);
            return (int)index;
        }
    }
    return -1;
}

static void fs_set_file(UINTN node, const char *data) {
    UINTN length = string_length(data);
    if (length >= FS_DATA_BYTES) length = FS_DATA_BYTES - 1;
    memory_zero(gNodes[node].data, FS_DATA_BYTES);
    memory_copy(gNodes[node].data, data, length);
    gNodes[node].size = length;
    fs_update(node);
}

static int fs_ensure_dir(UINTN parent, const char *name, UINT8 flags) {
    int node = fs_find_child(parent, name);
    if (node >= 0 && gNodes[node].type != FS_DIRECTORY) {
        gNodes[node].used = 0;
        node = -1;
    }
    if (node < 0) node = fs_alloc(FS_DIRECTORY, parent, name, flags);
    if (node >= 0) {
        gNodes[node].flags |= flags;
        fs_update((UINTN)node);
    }
    return node;
}

static int fs_ensure_file(UINTN parent, const char *name, const char *data, UINT8 flags) {
    int node = fs_find_child(parent, name);
    if (node >= 0 && gNodes[node].type != FS_FILE) {
        gNodes[node].used = 0;
        node = -1;
    }
    if (node < 0) node = fs_alloc(FS_FILE, parent, name, flags);
    if (node >= 0) {
        gNodes[node].flags |= flags;
        fs_set_file((UINTN)node, data);
    }
    return node;
}

static void fs_restore_system(void) {
    int system = fs_ensure_dir(FS_ROOT, "system", FS_PROTECTED);
    int apps = fs_ensure_dir(FS_ROOT, "apps", FS_PROTECTED);
    int home = fs_ensure_dir(FS_ROOT, "home", 0);
    int recovery = fs_ensure_dir(FS_ROOT, "recovery", FS_PROTECTED);
    int boot = -1;
    int config = -1;
    int drivers = -1;
    int runtime = -1;
    int kernel = -1;
    int firmware = -1;
    int security = -1;
    int doomApp = -1;
    int shellApp = -1;
    int recoveryApp = -1;
    fs_ensure_dir(FS_ROOT, "tmp", 0);
    fs_ensure_dir(FS_ROOT, "lost+found", FS_PROTECTED);
    if (system >= 0) {
        boot = fs_ensure_dir((UINTN)system, "boot", FS_PROTECTED);
        kernel = fs_ensure_dir((UINTN)system, "kernel", FS_PROTECTED);
        firmware = fs_ensure_dir((UINTN)system, "firmware", FS_PROTECTED);
        config = fs_ensure_dir((UINTN)system, "config", FS_PROTECTED);
        drivers = fs_ensure_dir((UINTN)system, "drivers", FS_PROTECTED);
        runtime = fs_ensure_dir((UINTN)system, "runtime", FS_PROTECTED);
        security = fs_ensure_dir((UINTN)system, "security", FS_PROTECTED);
        fs_ensure_file((UINTN)system, "version.txt",
            "TinyArmOS 0.4\narchitecture=ARM64\nfirmware=UEFI\nkernel=freestanding\nfilesystem=MiniFS2\nunix=no", FS_PROTECTED);
        fs_ensure_file((UINTN)system, "manifest.txt",
            "Critical tree:\n/system/boot       loader and startup records\n/system/kernel     core, ABI, and memory records\n/system/firmware   UEFI interface records\n/system/config     boot and shell policy\n/system/drivers    hardware service records\n/system/runtime    MiniFS2 runtime records\n/system/security   integrity and protected paths\n/apps              installed native applications\n/recovery          repair and snapshot records", FS_PROTECTED);
    }
    if (boot >= 0) {
        fs_ensure_file((UINTN)boot, "BOOTAA64.EFI.info",
            "critical=yes\ntype=ARM64 UEFI application\nentry=EfiMain\nsource=/src/uefi.c\ndisk=EFI/BOOT/BOOTAA64.EFI", FS_PROTECTED);
        fs_ensure_file((UINTN)boot, "startup.nsh.info",
            "critical=yes\nfirmware fallback=FS0:\\EFI\\BOOT\\BOOTAA64.EFI", FS_PROTECTED);
        fs_ensure_file((UINTN)boot, "boot-chain.info",
            "UEFI firmware -> BOOTAA64.EFI -> verified startup -> Recovery Agent -> shell", FS_PROTECTED);
    }
    if (kernel >= 0) {
        fs_ensure_file((UINTN)kernel, "kernel.info",
            "critical=yes\nmodel=single-address-space\narchitecture=AArch64\nentry=EfiMain\nservices=shell,MiniFS2,recovery,apps", FS_PROTECTED);
        fs_ensure_file((UINTN)kernel, "abi.info",
            "freestanding C17\nunix_abi=no\nposix=no\nsyscalls=native TinyArmOS services", FS_PROTECTED);
        fs_ensure_file((UINTN)kernel, "memory.map",
            "core=static image\nfilesystem=static checked nodes\napplications=UEFI pool\nboot_services=active", FS_PROTECTED);
    }
    if (firmware >= 0) {
        fs_ensure_file((UINTN)firmware, "uefi.info",
            "critical=yes\ninterface=UEFI ARM64\nloader=BOOTAA64.EFI\nwatchdog=disabled while OS runs", FS_PROTECTED);
        fs_ensure_file((UINTN)firmware, "protocols.info",
            "SimpleTextInput\nSimpleTextOutput\nSimpleFileSystem\nLoadedImage\nGraphicsOutput\nRuntimeServices", FS_PROTECTED);
    }
    if (config >= 0) {
        fs_ensure_file((UINTN)config, "boot.cfg",
            "recovery=auto\nrecovery_window=2s\nsnapshots=2\nwatchdog=disabled", FS_PROTECTED);
        fs_ensure_file((UINTN)config, "shell.cfg",
            "home=/home\napps=/apps\ntemporary=/tmp\nprompt=tinyarm\nnavigation=go,open,up,back,home,root", FS_PROTECTED);
        fs_ensure_file((UINTN)config, "protection.cfg",
            "protected=/system,/apps,/recovery,/lost+found\ndefault=locked\nunlock=protect unlock\nscope=current-boot", FS_PROTECTED);
    }
    if (drivers >= 0) {
        fs_ensure_file((UINTN)drivers, "graphics.info", "UEFI Graphics Output Protocol\nconsumer=Freedoom\nmode=firmware-native", FS_PROTECTED);
        fs_ensure_file((UINTN)drivers, "input.info", "UEFI Simple Text Input\nkeyboard=polling\nrelease=simulated", FS_PROTECTED);
        fs_ensure_file((UINTN)drivers, "storage.info", "UEFI Simple File System\nvolume=EFI FAT32\nsnapshots=TINYFS0.BIN,TINYFS1.BIN", FS_PROTECTED);
        fs_ensure_file((UINTN)drivers, "timer.info", "ARM generic virtual counter\nclock=monotonic", FS_PROTECTED);
    }
    if (runtime >= 0) {
        fs_ensure_file((UINTN)runtime, "minifs2.info", "hierarchical=yes\nchecksums=FNV-1a\nauto_repair=yes\nmax_nodes=96\nfile_limit=8191", FS_PROTECTED);
        fs_ensure_file((UINTN)runtime, "mounts.info", "/            MiniFS2 read-write\n/system      protected\n/apps        protected\n/recovery    protected\nEFI FAT32    platform storage", FS_PROTECTED);
        fs_ensure_file((UINTN)runtime, "snapshots.info", "copies=2\nstrategy=alternating\nselection=newest-valid\nfiles=TINYFS0.BIN,TINYFS1.BIN", FS_PROTECTED);
    }
    if (security >= 0) {
        fs_ensure_file((UINTN)security, "integrity.policy",
            "critical nodes use FNV-1a checksums\nverify at boot\nauto-repair metadata\nrollback to newest valid snapshot", FS_PROTECTED);
        fs_ensure_file((UINTN)security, "protected.paths",
            "/system\n/apps\n/recovery\n/lost+found\nUnlock requires exact UNLOCK confirmation and expires at reboot.", FS_PROTECTED);
    }
    if (apps >= 0) {
        doomApp = fs_ensure_dir((UINTN)apps, "doom", FS_PROTECTED);
        shellApp = fs_ensure_dir((UINTN)apps, "shell", FS_PROTECTED);
        recoveryApp = fs_ensure_dir((UINTN)apps, "recovery", FS_PROTECTED);
        fs_ensure_file((UINTN)apps, "registry.txt",
            "doom      command: doom or run doom\nshell     built-in interactive shell\nrecovery  command: recovery", FS_PROTECTED);
    }
    if (doomApp >= 0) {
        fs_ensure_file((UINTN)doomApp, "app.info",
            "name=Freedoom\nengine=PureDOOM\nkind=native ARM64 UEFI\ncommand=doom\ngraphics=UEFI GOP\nsound=disabled", FS_PROTECTED);
        fs_ensure_file((UINTN)doomApp, "controls.txt",
            "WASD move/strafe\narrows turn/move\nF fire\nE use\nEnter select\nEsc menu\nQ or F12 return to shell", FS_PROTECTED);
        fs_ensure_file((UINTN)doomApp, "data.link",
            "Freedoom Phase 1 0.13.0\nplatform file=DOOMU.WAD\nlicense=BSD-3-Clause\nThis is a metadata link; the IWAD lives on the EFI FAT32 volume.", FS_PROTECTED);
        fs_ensure_file((UINTN)doomApp, "license.info",
            "PureDOOM engine=GPL-2.0\nFreedoom assets=BSD-3-Clause\nSee source distribution licenses.", FS_PROTECTED);
    }
    if (shellApp >= 0) fs_ensure_file((UINTN)shellApp, "app.info",
        "name=TinyArmOS Shell\nkind=built-in\nfilesystem=MiniFS2\ncommands=help", FS_PROTECTED);
    if (recoveryApp >= 0) fs_ensure_file((UINTN)recoveryApp, "app.info",
        "name=Recovery Agent\nkind=built-in\ncommand=recovery\nboot hotkey=R", FS_PROTECTED);
    if (home >= 0) {
        fs_ensure_dir((UINTN)home, "notes", 0);
        if (fs_find_child((UINTN)home, "readme.txt") < 0) {
            fs_ensure_file((UINTN)home, "readme.txt",
                "Easy navigation: home, root, up, back, go system, go apps, dir, open PATH. Try: sysfiles or apps", 0);
        }
    }
    if (recovery >= 0) {
        fs_ensure_file((UINTN)recovery, "help.txt",
            "Recovery Agent: scan, repair, rollback, restore, reset, continue", FS_PROTECTED);
        fs_ensure_file((UINTN)recovery, "policy.txt",
            "Verify every node checksum at boot. Auto-repair damaged metadata. Preserve two alternating generations. Protected nodes require an explicit boot-scoped unlock.", FS_PROTECTED);
        fs_ensure_file((UINTN)recovery, "snapshots.info",
            "primary=TINYFS0.BIN\nsecondary=TINYFS1.BIN\nselection=newest-valid\nrollback=previous-valid", FS_PROTECTED);
    }
}

static void fs_format(void) {
    memory_zero(gNodes, sizeof(gNodes));
    gNodes[FS_ROOT].used = 1;
    gNodes[FS_ROOT].type = FS_DIRECTORY;
    gNodes[FS_ROOT].flags = FS_PROTECTED;
    gNodes[FS_ROOT].parent = FS_ROOT;
    gNodes[FS_ROOT].name[0] = 0;
    fs_update(FS_ROOT);
    gCwd = FS_ROOT;
    fs_restore_system();
}

static int fs_resolve(const char *path) {
    UINTN current;
    const char *cursor;
    char component[FS_NAME_BYTES];
    if (!path || !*path) return (int)gCwd;
    if (path[0] == '~' && (!path[1] || path[1] == '/')) {
        int home = fs_find_child(FS_ROOT, "home");
        if (home < 0) return -1;
        current = (UINTN)home;
        cursor = path + 1;
    } else {
        current = path[0] == '/' ? FS_ROOT : gCwd;
        cursor = path;
    }
    while (*cursor) {
        UINTN length = 0;
        const char *remaining;
        while (*cursor == '/') cursor++;
        if (!*cursor) break;
        if (current >= FS_MAX_NODES || !gNodes[current].used || gNodes[current].type != FS_DIRECTORY) return -1;
        while (*cursor && *cursor != '/') {
            if (length + 1 >= sizeof(component)) return -1;
            component[length++] = *cursor++;
        }
        component[length] = 0;
        if (streq(component, ".")) continue;
        if (streq(component, "..")) {
            current = gNodes[current].parent;
        } else {
            int child = fs_find_child(current, component);
            if (child < 0) return -1;
            current = (UINTN)child;
        }
        if (current >= FS_MAX_NODES || !gNodes[current].used) return -1;
        remaining = cursor;
        while (*remaining == '/') remaining++;
        if (*remaining && gNodes[current].type != FS_DIRECTORY) return -1;
    }
    return (int)current;
}

static int fs_resolve_parent(const char *path, UINTN *parent, char *name) {
    UINTN length;
    UINTN slash;
    char parentPath[FS_PATH_BYTES];
    if (!path || !*path) return 0;
    length = string_length(path);
    if (!length || path[length - 1] == '/') return 0;
    slash = length;
    while (slash && path[slash - 1] != '/') slash--;
    string_copy(name, path + slash, FS_NAME_BYTES);
    if (!fs_valid_name(name)) return 0;
    if (!slash) {
        *parent = gCwd;
    } else if (slash == 1) {
        *parent = FS_ROOT;
    } else {
        UINTN copyLength = slash - 1;
        if (copyLength >= sizeof(parentPath)) return 0;
        memory_copy(parentPath, path, copyLength);
        parentPath[copyLength] = 0;
        {
            int resolved = fs_resolve(parentPath);
            if (resolved < 0) return 0;
            *parent = (UINTN)resolved;
        }
    }
    return gNodes[*parent].type == FS_DIRECTORY;
}

static void fs_path(UINTN node, char *buffer, UINTN capacity) {
    UINTN stack[FS_MAX_NODES];
    UINTN depth = 0;
    buffer[0] = 0;
    if (node == FS_ROOT) {
        string_copy(buffer, "/", capacity);
        return;
    }
    while (node != FS_ROOT && depth < FS_MAX_NODES) {
        if (node >= FS_MAX_NODES || !gNodes[node].used || gNodes[node].parent >= FS_MAX_NODES) {
            string_copy(buffer, "/?", capacity);
            return;
        }
        stack[depth++] = node;
        node = gNodes[node].parent;
    }
    if (node != FS_ROOT || depth >= FS_MAX_NODES) {
        string_copy(buffer, "/?", capacity);
        return;
    }
    while (depth) {
        string_append(buffer, "/", capacity);
        string_append(buffer, gNodes[stack[--depth]].name, capacity);
    }
}

static int fs_has_children(UINTN node) {
    UINTN index;
    for (index = 1; index < FS_MAX_NODES; index++) if (gNodes[index].used && gNodes[index].parent == node) return 1;
    return 0;
}

static void fs_remove_recursive(UINTN node) {
    UINTN index;
    for (index = 1; index < FS_MAX_NODES; index++) {
        if (gNodes[index].used && gNodes[index].parent == node) fs_remove_recursive(index);
    }
    if (node != FS_ROOT) gNodes[node].used = 0;
}

static int fs_is_ancestor(UINTN ancestor, UINTN node) {
    UINTN steps = 0;
    while (node != FS_ROOT && steps++ < FS_MAX_NODES) {
        if (node >= FS_MAX_NODES || !gNodes[node].used) return 0;
        if (node == ancestor) return 1;
        node = gNodes[node].parent;
    }
    return ancestor == FS_ROOT;
}

static int fs_is_protected(UINTN node) {
    UINTN steps = 0;
    while (node != FS_ROOT && node < FS_MAX_NODES && gNodes[node].used && steps++ < FS_MAX_NODES) {
        if (gNodes[node].flags & FS_PROTECTED) return 1;
        node = gNodes[node].parent;
    }
    return 0;
}

static void fs_list(UINTN directory) {
    UINTN index;
    UINTN found = 0;
    if (gNodes[directory].type == FS_FILE) {
        print(gNodes[directory].name);
        print("  ");
        print_u64(gNodes[directory].size);
        print(" bytes\n");
        return;
    }
    for (index = 1; index < FS_MAX_NODES; index++) {
        if (gNodes[index].used && gNodes[index].parent == directory) {
            print(gNodes[index].type == FS_DIRECTORY ? "  <DIR>  " : "         ");
            print(gNodes[index].name);
            if (gNodes[index].type == FS_FILE) {
                print("  ");
                print_u64(gNodes[index].size);
                print(" B");
            }
            if (fs_is_protected(index)) print("  [system]");
            print("\n");
            found++;
        }
    }
    if (!found) print("  <empty>\n");
}

static void fs_tree_node(UINTN node, UINTN depth) {
    UINTN index;
    UINTN spaces;
    for (index = 1; index < FS_MAX_NODES; index++) {
        if (gNodes[index].used && gNodes[index].parent == node) {
            for (spaces = 0; spaces < depth; spaces++) print("  ");
            print(gNodes[index].type == FS_DIRECTORY ? "+ " : "- ");
            print(gNodes[index].name);
            print("\n");
            if (gNodes[index].type == FS_DIRECTORY && depth < 12) fs_tree_node(index, depth + 1);
        }
    }
}

static const char *fs_easy_path(const char *name) {
    if (!name || !*name || streq(name, "home")) return "/home";
    if (streq(name, "root")) return "/";
    if (streq(name, "system")) return "/system";
    if (streq(name, "apps")) return "/apps";
    if (streq(name, "recovery")) return "/recovery";
    if (streq(name, "temp") || streq(name, "tmp")) return "/tmp";
    if (streq(name, "up")) return "..";
    return name;
}

static void fs_change_directory(const char *path, int previous, int showContents) {
    int node = previous ? (int)gPreviousCwd : fs_resolve(path);
    if (node < 0 || (UINTN)node >= FS_MAX_NODES || !gNodes[node].used) {
        print("go: path not found\n");
        return;
    }
    if (gNodes[node].type != FS_DIRECTORY) {
        print("go: not a directory\n");
        return;
    }
    {
        UINTN old = gCwd;
        gCwd = (UINTN)node;
        gPreviousCwd = old;
    }
    if (showContents) fs_list(gCwd);
}

static int fs_check(int repair, int verbose) {
    UINTN index;
    int errors = 0;
    if (!gNodes[FS_ROOT].used || gNodes[FS_ROOT].type != FS_DIRECTORY || gNodes[FS_ROOT].parent != FS_ROOT) {
        errors++;
        if (verbose) print("fsck: root node is invalid\n");
        if (repair) {
            fs_format();
            if (verbose) print("fsck: filesystem rebuilt from defaults\n");
            return errors;
        }
        return errors;
    }
    for (index = 0; index < FS_MAX_NODES; index++) {
        UINTN other;
        UINTN cursor;
        UINTN steps = 0;
        int invalid = 0;
        if (!gNodes[index].used) continue;
        if ((index && !fs_valid_name(gNodes[index].name)) ||
            (gNodes[index].type != FS_FILE && gNodes[index].type != FS_DIRECTORY) ||
            gNodes[index].parent >= FS_MAX_NODES || !gNodes[gNodes[index].parent].used ||
            gNodes[gNodes[index].parent].type != FS_DIRECTORY ||
            (gNodes[index].type == FS_FILE && gNodes[index].size >= FS_DATA_BYTES)) invalid = 1;
        cursor = index;
        while (cursor != FS_ROOT && steps++ < FS_MAX_NODES) {
            if (cursor >= FS_MAX_NODES || !gNodes[cursor].used) {
                invalid = 1;
                break;
            }
            cursor = gNodes[cursor].parent;
        }
        if (cursor != FS_ROOT || steps >= FS_MAX_NODES) invalid = 1;
        for (other = 1; other < index; other++) {
            if (gNodes[other].used && gNodes[other].parent == gNodes[index].parent &&
                streq(gNodes[other].name, gNodes[index].name)) invalid = 1;
        }
        if (invalid) {
            errors++;
            if (verbose) {
                print("fsck: invalid node ");
                print_u64(index);
                print("\n");
            }
            if (repair && index) gNodes[index].used = 0;
            continue;
        }
        if (gNodes[index].checksum != fs_node_checksum(&gNodes[index])) {
            errors++;
            if (verbose) {
                print("fsck: checksum mismatch: ");
                print(index ? gNodes[index].name : "/");
                print("\n");
            }
            if (repair) {
                if (gNodes[index].type == FS_FILE) gNodes[index].data[gNodes[index].size] = 0;
                fs_update(index);
            }
        }
    }
    if (repair) {
        fs_restore_system();
        if (gCwd >= FS_MAX_NODES || !gNodes[gCwd].used || gNodes[gCwd].type != FS_DIRECTORY) gCwd = FS_ROOT;
    }
    if (verbose) {
        if (errors) {
            print("fsck: ");
            print_u64((UINT64)errors);
            print(repair ? " issue(s) repaired\n" : " issue(s) found\n");
        } else print("fsck: clean\n");
    }
    return errors;
}

static void fs_commit(void) {
    if (gStorageReady && !storage_sync()) print("warning: persistent snapshot failed; RAM copy is still active\n");
}

static int poll_key(char *character) {
    EFI_INPUT_KEY key;
    EFI_STATUS status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
    if (status != EFI_SUCCESS || !key.UnicodeChar) return 0;
    *character = (char)key.UnicodeChar;
    return 1;
}

static void read_line(char *line, UINTN capacity) {
    UINTN used = 0;
    EFI_INPUT_KEY key;
    for (;;) {
        EFI_STATUS status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
        if (status == EFI_NOT_READY) {
            __asm__ volatile("yield");
            continue;
        }
        if (status != EFI_SUCCESS) continue;
        if (gScrollbackEnabled && key.ScanCode == 1) {
            scrollback_move(-1, 1);
            continue;
        }
        if (gScrollbackEnabled && key.ScanCode == 2) {
            scrollback_move(1, 1);
            continue;
        }
        if (gScrollbackEnabled && key.ScanCode == 9) {
            scrollback_page(-1);
            continue;
        }
        if (gScrollbackEnabled && key.ScanCode == 10) {
            scrollback_page(1);
            continue;
        }
        if (gScrollbackEnabled && key.ScanCode == 5) {
            UINTN page = gConsoleRows > 4 ? gConsoleRows - 2 : 10;
            gScrollbackOffset = gScrollbackCount > page ? gScrollbackCount - page : 0;
            scrollback_render();
            continue;
        }
        if (gScrollbackEnabled && gScrollbackOffset && (key.ScanCode == 6 || key.ScanCode == 23)) {
            gScrollbackOffset = 0;
            scrollback_render();
            continue;
        }
        if (gScrollbackEnabled && gScrollbackOffset) {
            gScrollbackOffset = 0;
            scrollback_render();
        }
        if (!key.UnicodeChar) continue;
        if (key.UnicodeChar == '\r' || key.UnicodeChar == '\n') {
            print("\n");
            line[used] = 0;
            return;
        }
        if (key.UnicodeChar == 8 || key.UnicodeChar == 127) {
            if (used) {
                used--;
                print("\b \b");
            }
            continue;
        }
        if (key.UnicodeChar >= 32 && key.UnicodeChar <= 126 && used + 1 < capacity) {
            line[used++] = (char)key.UnicodeChar;
            print_char((char)key.UnicodeChar);
        }
    }
}

#include "doom_port.inc"

static void boot_stage(UINTN step, const char *label, int okay) {
    print("  [");
    print_u64(step);
    print("/5] ");
    print(label);
    print(okay ? " ... OK\n" : " ... RECOVERY NEEDED\n");
    delay_ms(120);
}

static int boot_screen(EFI_HANDLE imageHandle) {
    int mounted;
    int errors;
    char key = 0;
    UINT64 deadline;
    gST->ConOut->ClearScreen(gST->ConOut);
    gST->ConOut->SetAttribute(gST->ConOut, 0x0b);
    print(
        "\n"
        "          _______ _             ___              ___  ____\n"
        "         /_  __(_)___  __  __ / _ | ______ _  / _ \\/ __/\n"
        "          / / / / _  \\/ / / // __ |/ __/  ' \\/ , _/\\ \\  \n"
        "         /_/ /_/_//_/\\_, //_/ |_/_/ /_/_/_/_/|_|/___/\n"
        "                    /___/\n\n"
    );
    gST->ConOut->SetAttribute(gST->ConOut, 0x07);
    print("  TinyArmOS 0.4 verified startup\n\n");
    boot_stage(1, "ARM64 firmware and timer", 1);
    gStorageReady = (UINT8)storage_init(imageHandle);
    boot_stage(2, gStorageReady ? "persistent UEFI storage" : "volatile fallback storage", 1);
    mounted = gStorageReady ? storage_mount_latest() : 0;
    if (!mounted) {
        fs_format();
        gGeneration = 0;
        if (gStorageReady) storage_sync();
    }
    boot_stage(3, mounted ? "dual-snapshot MiniFS2 mount" : "new MiniFS2 filesystem", 1);
    errors = fs_check(0, 0);
    boot_stage(4, "Recovery Agent integrity scan", errors == 0);
    if (errors) {
        fs_check(1, 0);
        fs_commit();
    }
    boot_stage(5, "interactive shell", 1);
    print("\n  Press R for Recovery Agent (2 seconds) ");
    deadline = timer_count() + gTimerHz * 2;
    while (timer_count() < deadline) {
        if (poll_key(&key) && (key == 'r' || key == 'R')) {
            print("RECOVERY\n");
            return 1;
        }
        __asm__ volatile("yield");
    }
    print("BOOT\n");
    delay_ms(150);
    return errors != 0;
}

static void recovery_help(void) {
    print(
        "Recovery commands:\n"
        "  scan       verify nodes and checksums\n"
        "  repair     repair metadata and restore system files\n"
        "  rollback   load the previous valid disk snapshot\n"
        "  restore    restore protected system files\n"
        "  unlock / lock protected-node writes\n"
        "  pwd / ls [PATH] / cd PATH\n"
        "  cat PATH / view PATH / stat PATH\n"
        "  tree [PATH] inspect the filesystem\n"
        "  reset      format MiniFS2 after confirmation\n"
        "  continue   return to the normal shell\n"
        "  reboot / shutdown\n"
    );
}

static void recovery_agent(void) {
    char line[128];
    gST->ConOut->ClearScreen(gST->ConOut);
    gST->ConOut->SetAttribute(gST->ConOut, 0x0e);
    print("=== TinyArmOS Recovery Agent ===\n");
    gST->ConOut->SetAttribute(gST->ConOut, 0x07);
    print("Two checksummed snapshots protect the persistent filesystem.\n");
    recovery_help();
    for (;;) {
        print("recovery> ");
        read_line(line, sizeof(line));
        if (streq(line, "help") || streq(line, "?")) {
            recovery_help();
        } else if (streq(line, "scan")) {
            fs_check(0, 1);
            storage_probe_slots();
            print("snapshot A: ");
            print(gSlotValid[0] ? "valid generation " : "missing/corrupt\n");
            if (gSlotValid[0]) { print_u64(gSlotGeneration[0]); print("\n"); }
            print("snapshot B: ");
            print(gSlotValid[1] ? "valid generation " : "missing/corrupt\n");
            if (gSlotValid[1]) { print_u64(gSlotGeneration[1]); print("\n"); }
        } else if (streq(line, "repair")) {
            fs_check(1, 1);
            fs_commit();
        } else if (streq(line, "rollback")) {
            if (!storage_rollback()) print("rollback: no older valid snapshot\n");
            else {
                fs_check(1, 1);
                fs_commit();
                print("rollback: previous snapshot restored\n");
            }
        } else if (streq(line, "restore")) {
            fs_restore_system();
            fs_commit();
            print("protected system files restored\n");
        } else if (streq(line, "unlock")) {
            char answer[16];
            print("Type UNLOCK to allow protected-node changes this boot: ");
            read_line(answer, sizeof(answer));
            if (streq(answer, "UNLOCK")) { gProtectionUnlocked = 1; print("protection unlocked until reboot or 'lock'\n"); }
            else print("unlock cancelled\n");
        } else if (streq(line, "lock")) {
            gProtectionUnlocked = 0;
            print("protected nodes locked\n");
        } else if (streq(line, "pwd")) {
            char path[FS_PATH_BYTES];
            fs_path(gCwd, path, sizeof(path));
            print(path); print("\n");
        } else if (streq(line, "ls") || starts_with(line, "ls ")) {
            char *path = streq(line, "ls") ? (char *)"" : skip_spaces(line + 3);
            int node = fs_resolve(path);
            if (node < 0) print("ls: path not found\n");
            else fs_list((UINTN)node);
        } else if (streq(line, "cd")) {
            int home = fs_resolve("/home");
            if (home >= 0) { gPreviousCwd = gCwd; gCwd = (UINTN)home; }
        } else if (starts_with(line, "cd ")) {
            char *path = skip_spaces(line + 3);
            int node = streq(path, "-") ? (int)gPreviousCwd : fs_resolve(path);
            if (node < 0) print("cd: path not found\n");
            else if (gNodes[node].type != FS_DIRECTORY) print("cd: not a directory\n");
            else { UINTN old = gCwd; gCwd = (UINTN)node; gPreviousCwd = old; }
        } else if (starts_with(line, "cat ") || starts_with(line, "view ")) {
            int node = fs_resolve(skip_spaces(line + 4 + (line[0] == 'v')));
            if (node < 0) print("view: file not found\n");
            else if (gNodes[node].type != FS_FILE) print("view: not a file\n");
            else { print(gNodes[node].data); print("\n"); }
        } else if (starts_with(line, "stat ")) {
            int node = fs_resolve(skip_spaces(line + 5));
            if (node < 0) print("stat: path not found\n");
            else {
                char path[FS_PATH_BYTES];
                fs_path((UINTN)node, path, sizeof(path));
                print("path: "); print(path);
                print("\ntype: "); print(gNodes[node].type == FS_DIRECTORY ? "directory" : "file");
                print("\nsize: "); print_u64(gNodes[node].size);
                print("\nchecksum: "); print_hex(gNodes[node].checksum); print("\n");
            }
        } else if (streq(line, "tree") || starts_with(line, "tree ")) {
            char *path = streq(line, "tree") ? (char *)"/" : skip_spaces(line + 5);
            int node = fs_resolve(path);
            if (node < 0) print("tree: path not found\n");
            else { char full[FS_PATH_BYTES]; fs_path((UINTN)node, full, sizeof(full)); print(full); print("\n"); fs_tree_node((UINTN)node, 1); }
        } else if (streq(line, "reset")) {
            char answer[16];
            print("Type RESET to erase all user files: ");
            read_line(answer, sizeof(answer));
            if (streq(answer, "RESET")) {
                fs_format();
                fs_commit();
                print("MiniFS2 reset complete\n");
            } else print("reset cancelled\n");
        } else if (streq(line, "continue") || streq(line, "exit")) {
            return;
        } else if (streq(line, "reboot")) {
            gST->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, (void *)0);
            for (;;) __asm__ volatile("wfe");
        } else if (streq(line, "shutdown")) {
            gST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, (void *)0);
            for (;;) __asm__ volatile("wfe");
        } else if (*line) {
            print("Unknown recovery command. Type help.\n");
        }
    }
}

static void command_help(void) {
    print(
        "Shell:\n"
        "  help, clear, echo TEXT, info, uptime, count\n"
        "  Up/Down scroll lines; PageUp/PageDown pages; End/Esc returns live\n"
        "Easy navigation:\n"
        "  home / root / up / back       jump between common places\n"
        "  go PLACE|PATH                  go home, system, apps, recovery, tmp\n"
        "  dir/list [PATH]                simple file listing\n"
        "  open PATH                      open a file or enter a directory\n"
        "  apps / run doom                browse or launch applications\n"
        "Filesystem (absolute or relative paths):\n"
        "  pwd                 current directory\n"
        "  ls [PATH]           list files\n"
        "  tree [PATH]         show directory tree\n"
        "  sysfiles            show critical /system files\n"
        "  cd [PATH|-]         change directory (no path = /home)\n"
        "  cat/view PATH       print a file\n"
        "  write PATH TEXT     create/replace a file\n"
        "  append PATH TEXT    append text\n"
        "  touch PATH          create an empty file\n"
        "  mkdir PATH          create a directory\n"
        "  rm PATH             remove a file\n"
        "  rm -rf PATH         recursively remove a tree\n"
        "  rmdir PATH          remove an empty directory\n"
        "  cp SOURCE DEST      copy a file\n"
        "  mv SOURCE DEST      move or rename a node\n"
        "  stat PATH, df, sync, fsck\n"
        "Games:\n"
        "  doom                 launch Freedoom (Q returns to shell)\n"
        "System:\n"
        "  protect [status|unlock|lock], recovery, reboot, shutdown\n"
    );
}

static void command_info(void) {
    print("TinyArmOS 0.4\n");
    print("Architecture : ARM64 / AArch64\n");
    print("Boot method  : UEFI (BOOTAA64.EFI)\n");
    print("Filesystem   : MiniFS2, 96 hierarchical nodes, 2 snapshots\n");
    print("Storage      : ");
    print(gStorageReady ? "persistent FAT-backed snapshots\n" : "volatile fallback\n");
    print("Generation   : ");
    print_u64(gGeneration);
    print("\nProtection   : ");
    print(gProtectionUnlocked ? "UNLOCKED until reboot" : "locked");
    print("\nCurrent EL   : EL");
    print_u64(current_el());
    print("\nTimer Hz     : ");
    print_u64(gTimerHz);
    print("\nFirmware     : ");
    if (gST->FirmwareVendor) out16(gST->FirmwareVendor);
    else print("unknown");
    print("\n");
}

static void run_command(char *line) {
    char *command = skip_spaces(line);
    gCommands++;
    if (!*command) return;
    if (streq(command, "help") || streq(command, "?")) {
        command_help();
    } else if (streq(command, "clear") || streq(command, "cls")) {
        gST->ConOut->ClearScreen(gST->ConOut);
    } else if (streq(command, "scroll")) {
        print("Scrollback stores ");
        print_u64(gScrollbackCount);
        print("/256 lines. Use Up/Down for lines, PageUp/PageDown for pages; End or Esc returns live.\n");
    } else if (streq(command, "scroll clear")) {
        scrollback_reset();
        print("scrollback cleared\n");
    } else if (starts_with(command, "echo ")) {
        print(skip_spaces(command + 5));
        print("\n");
    } else if (streq(command, "echo")) {
        print("\n");
    } else if (streq(command, "info") || streq(command, "version")) {
        command_info();
    } else if (streq(command, "uptime")) {
        UINT64 elapsed = timer_count() - gStartTicks;
        print_u64(gTimerHz ? elapsed / gTimerHz : 0);
        print(" seconds\n");
    } else if (streq(command, "history") || streq(command, "count")) {
        print_u64(gCommands);
        print(" commands entered this boot\n");
    } else if (streq(command, "pwd") || streq(command, "where")) {
        char path[FS_PATH_BYTES];
        fs_path(gCwd, path, sizeof(path));
        print(path);
        print("\n");
    } else if (streq(command, "ls") || starts_with(command, "ls ") ||
               streq(command, "dir") || starts_with(command, "dir ") ||
               streq(command, "list") || starts_with(command, "list ")) {
        char *path = (char *)"";
        int node;
        if (starts_with(command, "ls ")) path = skip_spaces(command + 3);
        else if (starts_with(command, "dir ")) path = skip_spaces(command + 4);
        else if (starts_with(command, "list ")) path = skip_spaces(command + 5);
        node = fs_resolve(path);
        if (node < 0) print("list: path not found\n");
        else fs_list((UINTN)node);
    } else if (streq(command, "tree") || starts_with(command, "tree ")) {
        char *path = streq(command, "tree") ? (char *)"" : skip_spaces(command + 5);
        int node = fs_resolve(path);
        if (node < 0) print("tree: path not found\n");
        else {
            char fullPath[FS_PATH_BYTES];
            fs_path((UINTN)node, fullPath, sizeof(fullPath));
            print(fullPath);
            print("\n");
            if (gNodes[node].type == FS_DIRECTORY) fs_tree_node((UINTN)node, 1);
        }
    } else if (streq(command, "sysfiles") || streq(command, "system")) {
        int systemNode = fs_resolve("/system");
        int appsNode = fs_resolve("/apps");
        print("Critical OS files [");
        print(gProtectionUnlocked ? "UNLOCKED" : "LOCKED");
        print("]:\n/system\n");
        if (systemNode >= 0) fs_tree_node((UINTN)systemNode, 1);
        print("/apps\n");
        if (appsNode >= 0) fs_tree_node((UINTN)appsNode, 1);
        print("Use 'open /system/manifest.txt' for descriptions.\n");
    } else if (streq(command, "apps")) {
        int node = fs_resolve("/apps");
        print("Installed applications:\n");
        if (node >= 0) fs_list((UINTN)node);
        print("Use 'go apps', 'open APP', or 'run doom'.\n");
    } else if (streq(command, "home") || streq(command, "root") || streq(command, "up") || streq(command, "back")) {
        fs_change_directory(fs_easy_path(command), streq(command, "back"), 0);
    } else if (streq(command, "go") || starts_with(command, "go ")) {
        char *target = streq(command, "go") ? (char *)"home" : skip_spaces(command + 3);
        fs_change_directory(fs_easy_path(target), streq(target, "back") || streq(target, "-"), 0);
    } else if (streq(command, "cd")) {
        fs_change_directory("/home", 0, 0);
    } else if (starts_with(command, "cd ")) {
        char *path = skip_spaces(command + 3);
        fs_change_directory(path, streq(path, "-"), 0);
    } else if (streq(command, "open") || starts_with(command, "open ")) {
        char *path = streq(command, "open") ? (char *)"" : skip_spaces(command + 5);
        int node = *path ? fs_resolve(fs_easy_path(path)) : (int)gCwd;
        if (node < 0) print("open: path not found\n");
        else if (gNodes[node].type == FS_DIRECTORY && !*path) fs_list((UINTN)node);
        else if (gNodes[node].type == FS_DIRECTORY) fs_change_directory(fs_easy_path(path), 0, 1);
        else { print(gNodes[node].data); print("\n"); }
    } else if (starts_with(command, "cat ") || starts_with(command, "view ")) {
        int node = fs_resolve(skip_spaces(command + 4 + (command[0] == 'v')));
        if (node < 0) print("cat: file not found\n");
        else if (gNodes[node].type != FS_FILE) print("cat: not a file\n");
        else {
            print(gNodes[node].data);
            print("\n");
        }
    } else if (starts_with(command, "write ") || starts_with(command, "append ")) {
        int append = starts_with(command, "append ");
        char *data;
        char *path = next_argument(command + (append ? 7 : 6), &data);
        int node = path ? fs_resolve(path) : -1;
        if (!path) print("write: expected PATH TEXT\n");
        else {
            if (node < 0) {
                UINTN parent;
                char name[FS_NAME_BYTES];
                if (!fs_resolve_parent(path, &parent, name)) {
                    print("write: invalid path\n");
                    return;
                }
                if (fs_is_protected(parent) && !gProtectionUnlocked) {
                    print("write: protected system path (use 'protect unlock')\n");
                    return;
                }
                node = fs_alloc(FS_FILE, parent, name, 0);
            }
            if (node < 0) print("write: filesystem full or path exists\n");
            else if (gNodes[node].type != FS_FILE) print("write: path is a directory\n");
            else if (fs_is_protected((UINTN)node) && !gProtectionUnlocked) print("write: protected system file (use 'protect unlock')\n");
            else {
                if (append) {
                    UINTN available = FS_DATA_BYTES - 1 - gNodes[node].size;
                    UINTN amount = string_length(data);
                    if (amount > available) amount = available;
                    memory_copy(gNodes[node].data + gNodes[node].size, data, amount);
                    gNodes[node].size += amount;
                    gNodes[node].data[gNodes[node].size] = 0;
                    fs_update((UINTN)node);
                } else fs_set_file((UINTN)node, data);
                fs_commit();
                print("saved ");
                print_u64(gNodes[node].size);
                print(" bytes\n");
            }
        }
    } else if (starts_with(command, "touch ") || starts_with(command, "mkdir ")) {
        int directory = starts_with(command, "mkdir ");
        char *path = skip_spaces(command + (directory ? 6 : 6));
        UINTN parent;
        char name[FS_NAME_BYTES];
        int existing = fs_resolve(path);
        if (existing >= 0) print(directory && gNodes[existing].type != FS_DIRECTORY ? "mkdir: file exists\n" : "already exists\n");
        else if (!fs_resolve_parent(path, &parent, name)) print("create: invalid path\n");
        else if (fs_is_protected(parent) && !gProtectionUnlocked) print("create: protected system path (use 'protect unlock')\n");
        else if (fs_alloc(directory ? FS_DIRECTORY : FS_FILE, parent, name, 0) < 0) print("create: filesystem full\n");
        else fs_commit();
    } else if (starts_with(command, "rm ") || starts_with(command, "rmdir ")) {
        int recursive = starts_with(command, "rm -rf ");
        int directory = recursive || starts_with(command, "rmdir ");
        char *path = skip_spaces(command + (recursive ? 7 : (directory ? 6 : 3)));
        int node = *path ? fs_resolve(path) : -1;
        if (node <= 0) print("remove: path not found or root\n");
        else if (fs_is_protected((UINTN)node) && !gProtectionUnlocked) print("remove: protected system node (use 'protect unlock')\n");
        else if (!recursive && directory && gNodes[node].type != FS_DIRECTORY) print("rmdir: not a directory\n");
        else if (!directory && gNodes[node].type != FS_FILE) print("rm: use rmdir or rm -rf for directories\n");
        else if (!recursive && gNodes[node].type == FS_DIRECTORY && fs_has_children((UINTN)node)) print("rmdir: directory not empty\n");
        else if ((UINTN)node == gCwd || fs_is_ancestor((UINTN)node, gCwd)) print("remove: directory is in use\n");
        else {
            if (fs_is_ancestor((UINTN)node, gPreviousCwd)) gPreviousCwd = FS_ROOT;
            if (recursive) fs_remove_recursive((UINTN)node);
            else gNodes[node].used = 0;
            fs_commit();
            print(recursive ? "removed recursively\n" : "removed\n");
        }
    } else if (starts_with(command, "cp ")) {
        char *destination;
        char *source = next_argument(command + 3, &destination);
        int sourceNode = source ? fs_resolve(source) : -1;
        UINTN parent = FS_ROOT;
        char name[FS_NAME_BYTES];
        int destinationNode = *destination ? fs_resolve(destination) : -1;
        if (sourceNode < 0 || gNodes[sourceNode].type != FS_FILE) print("cp: source file not found\n");
        else {
            if (destinationNode >= 0 && gNodes[destinationNode].type == FS_DIRECTORY) {
                parent = (UINTN)destinationNode;
                string_copy(name, gNodes[sourceNode].name, sizeof(name));
            } else if (!*destination || !fs_resolve_parent(destination, &parent, name)) {
                print("cp: invalid destination\n");
                return;
            }
            if (fs_is_protected(parent) && !gProtectionUnlocked) {
                print("cp: protected destination (use 'protect unlock')\n");
                return;
            }
            destinationNode = fs_alloc(FS_FILE, parent, name, 0);
            if (destinationNode < 0) print("cp: destination exists or filesystem full\n");
            else {
                fs_set_file((UINTN)destinationNode, gNodes[sourceNode].data);
                fs_commit();
            }
        }
    } else if (starts_with(command, "mv ") || starts_with(command, "rename ")) {
        int renameCommand = starts_with(command, "rename ");
        char *destination;
        char *source = next_argument(command + (renameCommand ? 7 : 3), &destination);
        int sourceNode = source ? fs_resolve(source) : -1;
        UINTN parent = FS_ROOT;
        char name[FS_NAME_BYTES];
        int destinationNode = *destination ? fs_resolve(destination) : -1;
        if (sourceNode <= 0) print("mv: source not found or root\n");
        else if (fs_is_protected((UINTN)sourceNode) && !gProtectionUnlocked) print("mv: protected system node (use 'protect unlock')\n");
        else {
            if (destinationNode >= 0 && gNodes[destinationNode].type == FS_DIRECTORY) {
                parent = (UINTN)destinationNode;
                string_copy(name, gNodes[sourceNode].name, sizeof(name));
            } else if (!*destination || !fs_resolve_parent(destination, &parent, name)) {
                print("mv: invalid destination\n");
                return;
            }
            if (fs_is_protected(parent) && !gProtectionUnlocked) print("mv: protected destination (use 'protect unlock')\n");
            else if (fs_find_child(parent, name) >= 0) print("mv: destination already exists\n");
            else if (gNodes[sourceNode].type == FS_DIRECTORY && fs_is_ancestor((UINTN)sourceNode, parent)) print("mv: cannot move a directory inside itself\n");
            else {
                gNodes[sourceNode].parent = parent;
                string_copy(gNodes[sourceNode].name, name, FS_NAME_BYTES);
                fs_update((UINTN)sourceNode);
                fs_commit();
            }
        }
    } else if (starts_with(command, "stat ")) {
        int node = fs_resolve(skip_spaces(command + 5));
        if (node < 0) print("stat: path not found\n");
        else {
            char path[FS_PATH_BYTES];
            fs_path((UINTN)node, path, sizeof(path));
            print("path: "); print(path);
            print("\ntype: "); print(gNodes[node].type == FS_DIRECTORY ? "directory" : "file");
            print("\nsize: "); print_u64(gNodes[node].size);
            print("\nchecksum: "); print_hex(gNodes[node].checksum);
            print("\nprotected: "); print(fs_is_protected((UINTN)node) ? "yes\n" : "no\n");
        }
    } else if (streq(command, "df")) {
        UINTN index;
        UINTN nodes = 0;
        UINTN bytes = 0;
        for (index = 0; index < FS_MAX_NODES; index++) if (gNodes[index].used) {
            nodes++;
            if (gNodes[index].type == FS_FILE) bytes += gNodes[index].size;
        }
        print_u64(nodes); print("/"); print_u64(FS_MAX_NODES); print(" nodes, ");
        print_u64(bytes); print("/"); print_u64((FS_MAX_NODES - 1) * (FS_DATA_BYTES - 1)); print(" data bytes\n");
    } else if (streq(command, "sync")) {
        print(storage_sync() ? "snapshot saved\n" : "sync unavailable or failed\n");
    } else if (streq(command, "fsck")) {
        fs_check(0, 1);
    } else if (starts_with(command, "fault ")) {
        int node = fs_resolve(skip_spaces(command + 6));
        if (node < 0 || gNodes[node].type != FS_FILE) print("fault: file not found\n");
        else {
            gNodes[node].checksum ^= 0x13579bdfU;
            print("test fault injected; run fsck or recovery\n");
        }
    } else if (streq(command, "doom") || streq(command, "freedoom") || streq(command, "run doom")) {
        print("Freedoom controls: WASD move, arrows turn, F fire, E use, Enter select, Esc menu.\n");
        print("Press Q (or F12) at any time to return to TinyArmOS. Starting...\n");
        delay_ms(500);
        doom_run();
    } else if (starts_with(command, "run ")) {
        print("run: application not found; use 'apps' to browse installed apps\n");
    } else if (streq(command, "protect") || streq(command, "protect status")) {
        print(gProtectionUnlocked ? "protected nodes are UNLOCKED until reboot\n" : "protected nodes are locked\n");
    } else if (streq(command, "protect unlock")) {
        char answer[16];
        print("Type UNLOCK to allow changes under /system, /apps, and /recovery this boot: ");
        read_line(answer, sizeof(answer));
        if (streq(answer, "UNLOCK")) { gProtectionUnlocked = 1; print("protection unlocked; run 'protect lock' when finished\n"); }
        else print("unlock cancelled\n");
    } else if (streq(command, "protect lock")) {
        gProtectionUnlocked = 0;
        print("protected nodes locked\n");
    } else if (streq(command, "recovery")) {
        recovery_agent();
        gST->ConOut->ClearScreen(gST->ConOut);
        print("Returned from Recovery Agent.\n");
    } else if (streq(command, "reboot")) {
        fs_commit();
        gST->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, (void *)0);
        for (;;) __asm__ volatile("wfe");
    } else if (streq(command, "shutdown") || streq(command, "poweroff")) {
        fs_commit();
        gST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, (void *)0);
        for (;;) __asm__ volatile("wfe");
    } else {
        print("Unknown command: ");
        print(command);
        print("\nType 'help' for commands.\n");
    }
}

__attribute__((used))
EFI_STATUS EFIAPI EfiMain(EFI_HANDLE image, EFI_SYSTEM_TABLE *systemTable) {
    char line[FS_PATH_BYTES];
    char path[FS_PATH_BYTES];
    int recoveryRequested;
    gST = systemTable;
    if (gST->BootServices->SetWatchdogTimer) gST->BootServices->SetWatchdogTimer(0, 0, 0, (CHAR16 *)0);
    gTimerHz = timer_frequency();
    gStartTicks = timer_count();
    gCommands = 0;
    gGeneration = 0;
    gProtectionUnlocked = 0;
    gScrollbackEnabled = 0;
    gDoomStarted = 0;
    gCwd = FS_ROOT;
    gPreviousCwd = FS_ROOT;
    recoveryRequested = boot_screen(image);
    if (recoveryRequested) recovery_agent();

    scrollback_enable();
    gST->ConOut->ClearScreen(gST->ConOut);
    gST->ConOut->SetAttribute(gST->ConOut, 0x0b);
    print("TinyArmOS 0.4");
    gST->ConOut->SetAttribute(gST->ConOut, 0x07);
    print(" - ARM64 shell + MiniFS2\n");
    print("Recovery Agent: healthy. Type 'help'.\n\n");
    for (;;) {
        fs_path(gCwd, path, sizeof(path));
        print("tinyarm:");
        print(path);
        print("> ");
        read_line(line, sizeof(line));
        run_command(line);
    }
}
