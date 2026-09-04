// TinyGPT: a freestanding ARM64 UEFI shell with a persistent mini filesystem.
typedef unsigned char      UINT8;
typedef unsigned short     UINT16;
typedef unsigned int       UINT32;
typedef unsigned long long UINT64;
typedef unsigned long long UINTN;
typedef signed short       INT16;
typedef int                INT32;
typedef UINT16             CHAR16;
typedef void              *EFI_HANDLE;
typedef UINT64             EFI_STATUS;

#define EFI_SUCCESS            0
#define EFI_ERROR_MASK         0x8000000000000000ULL
#define EFI_UNSUPPORTED        (EFI_ERROR_MASK | 3ULL)
#define EFI_BUFFER_TOO_SMALL   (EFI_ERROR_MASK | 5ULL)
#define EFI_NOT_READY          (EFI_ERROR_MASK | 6ULL)
#define EFI_DEVICE_ERROR       (EFI_ERROR_MASK | 7ULL)
#define EFI_OUT_OF_RESOURCES   (EFI_ERROR_MASK | 9ULL)
#define EFI_NOT_FOUND          (EFI_ERROR_MASK | 14ULL)
#define EFI_ACCESS_DENIED      (EFI_ERROR_MASK | 15ULL)
#define EFI_NO_MAPPING         (EFI_ERROR_MASK | 17ULL)
#define EFI_TIMEOUT            (EFI_ERROR_MASK | 18ULL)
#define EFI_ABORTED            (EFI_ERROR_MASK | 21ULL)
#define EFI_SECURITY_VIOLATION (EFI_ERROR_MASK | 26ULL)
#define EFI_HTTP_ERROR         (EFI_ERROR_MASK | 35ULL)
#define EFIAPI

#define EVT_NOTIFY_SIGNAL 0x00000200U
#define TPL_CALLBACK      8U
#define TINYGPT_VERSION "0.1.6"
#ifndef TINYGPT_DISPLAY_VERSION
#define TINYGPT_DISPLAY_VERSION TINYGPT_VERSION
#endif
#ifndef TINYGPT_BUILD_CHANNEL
#define TINYGPT_BUILD_CHANNEL "main"
#endif

static const char gTinyGPTBuildMetadata[] __attribute__((used)) =
    "TinyGPTBuildVersion=" TINYGPT_VERSION "\n"
    "TinyGPTBuildChannel=" TINYGPT_BUILD_CHANNEL "\n";

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

typedef void *EFI_EVENT;
typedef void (EFIAPI *EFI_EVENT_NOTIFY)(EFI_EVENT, void *);
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(UINT32, UINTN, void **);
typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(void *);
typedef EFI_STATUS (EFIAPI *EFI_CREATE_EVENT)(UINT32, UINTN, EFI_EVENT_NOTIFY, void *, EFI_EVENT *);
typedef EFI_STATUS (EFIAPI *EFI_CLOSE_EVENT)(EFI_EVENT);
typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(EFI_HANDLE, EFI_GUID *, void **);
typedef EFI_STATUS (EFIAPI *EFI_STALL)(UINTN);
typedef EFI_STATUS (EFIAPI *EFI_SET_WATCHDOG_TIMER)(UINTN, UINT64, UINTN, CHAR16 *);
typedef EFI_STATUS (EFIAPI *EFI_CONNECT_CONTROLLER)(EFI_HANDLE, EFI_HANDLE *, void *, UINT8);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(UINT32, EFI_GUID *, void *, UINTN *, EFI_HANDLE **);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(EFI_GUID *, void *, void **);
typedef struct {
    EFI_TABLE_HEADER Hdr;
    void *BeforeAllocatePool[5];
    EFI_ALLOCATE_POOL AllocatePool;
    EFI_FREE_POOL FreePool;
    EFI_CREATE_EVENT CreateEvent;
    void *SetTimer;
    void *WaitForEvent;
    void *SignalEvent;
    EFI_CLOSE_EVENT CloseEvent;
    void *CheckEvent;
    void *BeforeHandleProtocol[3];
    EFI_HANDLE_PROTOCOL HandleProtocol;
    void *BeforeStall[11];
    EFI_STALL Stall;
    EFI_SET_WATCHDOG_TIMER SetWatchdogTimer;
    EFI_CONNECT_CONTROLLER ConnectController;
    void *BeforeLocateHandleBuffer[5];
    EFI_LOCATE_HANDLE_BUFFER LocateHandleBuffer;
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

typedef struct {
    UINT16 Year;
    UINT8 Month;
    UINT8 Day;
    UINT8 Hour;
    UINT8 Minute;
    UINT8 Second;
    UINT8 Pad1;
    UINT32 Nanosecond;
    INT16 TimeZone;
    UINT8 Daylight;
    UINT8 Pad2;
} EFI_TIME;

typedef struct {
    UINT64 Size;
    UINT64 FileSize;
    UINT64 PhysicalSize;
    EFI_TIME CreateTime;
    EFI_TIME LastAccessTime;
    EFI_TIME ModificationTime;
    UINT64 Attribute;
    CHAR16 FileName[1];
} EFI_FILE_INFO;

typedef struct {
    UINT64 Size;
    UINT8 ReadOnly;
    UINT8 Reserved[7];
    UINT64 VolumeSize;
    UINT64 FreeSpace;
    UINT32 BlockSize;
    CHAR16 VolumeLabel[1];
} EFI_FILE_SYSTEM_INFO;

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

#define EFI_FILE_MODE_READ      0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE     0x0000000000000002ULL
#define EFI_FILE_MODE_CREATE    0x8000000000000000ULL
#define EFI_FILE_READ_ONLY      0x0000000000000001ULL
#define EFI_FILE_DIRECTORY      0x0000000000000010ULL
#define EFI_FILE_INFO_NAME_BASE 80U
#define EFI_FILE_INFO_CAPACITY  2048U

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
#define SETTINGS_DEFAULT_TEXT_COLOR 7U
#define SETTINGS_DEFAULT_ACCENT_COLOR 11U
#define SETTINGS_DEFAULT_BACKGROUND_COLOR 0U
#define PARTITION_MAX 16U
#define PARTITION_REGISTRY_MAGIC 0x31545250U

static EFI_SYSTEM_TABLE *gST;
static EFI_FILE_PROTOCOL *gVolumeRoot;
static EFI_FILE_PROTOCOL *gBootVolumeRoot;
static EFI_FILE_PROTOCOL *gPartitionRoots[PARTITION_MAX];
static char gPartitionNames[PARTITION_MAX][12];
static UINTN gActivePartition;
static UINT64 gStartTicks;
static UINT64 gTimerHz;
static UINT64 gGeneration;
static UINT64 gSlotGeneration[2];
static UINT8 gSlotValid[2];
static UINT8 gStorageReady;
static UINT8 gDedicatedStorage;
static UINT8 gLegacySinglePartition;
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
    UINT8 textColor;
    UINT8 accentColor;
    UINT8 backgroundColor;
    UINT8 showPromptPath;
    UINT8 startupHome;
    UINT8 scrollback;
} SHELL_SETTINGS;

static SHELL_SETTINGS gSettings;

static void settings_use_default_color(void);
static void settings_use_accent_color(void);

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
static const CHAR16 gBootPath[] = {
    '\\','E','F','I','\\','B','O','O','T','\\','B','O','O','T','A','A','6','4','.','E','F','I',0
};
static const CHAR16 gBootBackupPath[] = {
    '\\','E','F','I','\\','B','O','O','T','\\','B','O','O','T','A','A','6','4','.','B','A','K',0
};
static const CHAR16 gBootStagePath[] = {
    '\\','E','F','I','\\','B','O','O','T','\\','B','O','O','T','A','A','6','4','.','N','E','W',0
};
static const CHAR16 gOsMissingPath[] = {'\\','T','I','N','Y','O','S','.','O','F','F',0};
static const CHAR16 gFactoryInstallPath[] = {'\\','T','I','N','Y','O','S','.','N','E','W',0};
static const CHAR16 gBootOrderPath[] = {'\\','B','O','O','T','O','R','D','.','C','F','G',0};
static const CHAR16 gPartitionRegistryPath[] = {'\\','P','A','R','T','S','.','C','F','G',0};
static const CHAR16 gStartupPath[] = {'\\','S','T','A','R','T','U','P','.','N','S','H',0};
static const CHAR16 gDoomWadPath[] = {'\\','D','O','O','M','U','.','W','A','D',0};
static const CHAR16 gDoomConfigPath[] = {'\\','D','O','O','M','.','C','F','G',0};
static EFI_GUID gFileInfoGuid = {0x09576e92, 0x6d3f, 0x11d2, {0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
static EFI_GUID gFileSystemInfoGuid = {0x09576e93, 0x6d3f, 0x11d2, {0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};

static int char16_equals_ascii(const CHAR16 *wide, const char *ascii) {
    UINTN index = 0;
    while (wide[index] && ascii[index]) {
        CHAR16 left = wide[index];
        char right = ascii[index];
        if (left >= 'a' && left <= 'z') left = (CHAR16)(left - ('a' - 'A'));
        if (right >= 'a' && right <= 'z') right = (char)(right - ('a' - 'A'));
        if (left != (CHAR16)(UINT8)right) return 0;
        index++;
    }
    return wide[index] == 0 && ascii[index] == 0;
}

static int storage_path_exists(const CHAR16 *path) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    EFI_STATUS status;
    if (!gVolumeRoot || !path || !path[0]) return 0;
    status = gVolumeRoot->Open(gVolumeRoot, &file, (CHAR16 *)path, EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS || !file) return 0;
    file->Close(file);
    return 1;
}

static int storage_volume_has_label(EFI_FILE_PROTOCOL *root, const char *label) {
    UINT64 storage[64];
    EFI_FILE_SYSTEM_INFO *information = (EFI_FILE_SYSTEM_INFO *)(void *)storage;
    UINTN bytes = sizeof(storage);
    EFI_STATUS status;
    if (!root) return 0;
    status = root->GetInfo(root, &gFileSystemInfoGuid, &bytes, information);
    if (status != EFI_SUCCESS || bytes < 38U || information->Size < 38U || information->Size > bytes) return 0;
    return char16_equals_ascii(information->VolumeLabel, label);
}

static int storage_volume_is_dedicated(void) {
    return gActivePartition >= 2U;
}

typedef struct {
    UINT32 magic;
    UINT32 version;
    char names[PARTITION_MAX][12];
    UINT32 checksum;
} PARTITION_REGISTRY;

static void partition_registry_defaults(void) {
    memory_zero(gPartitionNames, sizeof(gPartitionNames));
    string_copy(gPartitionNames[0], "TINYRECOV", sizeof(gPartitionNames[0]));
    string_copy(gPartitionNames[1], "TINYGPT", sizeof(gPartitionNames[1]));
}

static void partition_registry_load(void) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    PARTITION_REGISTRY registry;
    UINTN bytes = sizeof(registry);
    EFI_STATUS status;
    partition_registry_defaults();
    if (!gBootVolumeRoot) return;
    status = gBootVolumeRoot->Open(gBootVolumeRoot, &file,
        (CHAR16 *)gPartitionRegistryPath, EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS || !file) return;
    status = file->Read(file, &bytes, &registry);
    file->Close(file);
    if (status != EFI_SUCCESS || bytes != sizeof(registry) ||
        registry.magic != PARTITION_REGISTRY_MAGIC || registry.version != 1U) return;
    {
        UINT32 storedChecksum = registry.checksum;
        registry.checksum = 0;
        if (storedChecksum != hash_bytes(&registry, sizeof(registry))) return;
    }
    memory_copy(gPartitionNames, registry.names, sizeof(gPartitionNames));
    for (UINTN index = 0; index < PARTITION_MAX; index++)
        gPartitionNames[index][sizeof(gPartitionNames[index]) - 1U] = 0;
    string_copy(gPartitionNames[0], "TINYRECOV", sizeof(gPartitionNames[0]));
}

static int partition_registry_save(void) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    PARTITION_REGISTRY registry;
    UINTN bytes = sizeof(registry);
    EFI_STATUS status;
    if (!gBootVolumeRoot) return 0;
    registry.magic = PARTITION_REGISTRY_MAGIC;
    registry.version = 1U;
    memory_copy(registry.names, gPartitionNames, sizeof(gPartitionNames));
    registry.checksum = 0;
    registry.checksum = hash_bytes(&registry, sizeof(registry));
    status = gBootVolumeRoot->Open(gBootVolumeRoot, &file,
        (CHAR16 *)gPartitionRegistryPath,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (status != EFI_SUCCESS || !file) return 0;
    status = file->SetPosition(file, 0);
    if (status == EFI_SUCCESS) status = file->Write(file, &bytes, &registry);
    if (status == EFI_SUCCESS && bytes == sizeof(registry)) status = file->Flush(file);
    file->Close(file);
    return status == EFI_SUCCESS && bytes == sizeof(registry);
}

static UINTN boot_order_default_partition(void) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    UINT8 value = 0;
    UINTN bytes = 1;
    EFI_STATUS status;
    if (!gBootVolumeRoot) return 2U;
    status = gBootVolumeRoot->Open(gBootVolumeRoot, &file, (CHAR16 *)gBootOrderPath,
        EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS || !file) return 2U;
    status = file->Read(file, &bytes, &value);
    file->Close(file);
    if (status != EFI_SUCCESS || bytes != 1U) return 2U;
    if (value == 'R') return 1U;
    if (value == 'S') return 2U;
    if (value >= 1U && value <= PARTITION_MAX) return (UINTN)value;
    return 2U;
}

static int boot_order_save(UINTN partition) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    UINT8 value = (UINT8)partition;
    UINTN bytes = 1;
    EFI_STATUS status;
    if (!gBootVolumeRoot || partition < 1U || partition > PARTITION_MAX) return 0;
    status = gBootVolumeRoot->Open(gBootVolumeRoot, &file, (CHAR16 *)gBootOrderPath,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (status != EFI_SUCCESS || !file) return 0;
    status = file->SetPosition(file, 0);
    if (status == EFI_SUCCESS) status = file->Write(file, &bytes, &value);
    if (status == EFI_SUCCESS && bytes == 1U) status = file->Flush(file);
    file->Close(file);
    return status == EFI_SUCCESS && bytes == 1U;
}

static int storage_init(EFI_HANDLE imageHandle) {
    static EFI_GUID loadedImageGuid = {0x5b1b31a1, 0x9562, 0x11d2, {0x8e,0x3f,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
    static EFI_GUID simpleFsGuid = {0x964e5b22, 0x6459, 0x11d2, {0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
    EFI_LOADED_IMAGE_PROTOCOL *loadedImage = (EFI_LOADED_IMAGE_PROTOCOL *)0;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *filesystem = (EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *)0;
    EFI_HANDLE *handles = (EFI_HANDLE *)0;
    UINTN handleCount = 0;
    UINTN index;
    EFI_STATUS status;
    gVolumeRoot = (EFI_FILE_PROTOCOL *)0;
    gBootVolumeRoot = (EFI_FILE_PROTOCOL *)0;
    memory_zero(gPartitionRoots, sizeof(gPartitionRoots));
    partition_registry_defaults();
    gActivePartition = 2U;
    gDedicatedStorage = 0;
    gLegacySinglePartition = 0;
    status = gST->BootServices->HandleProtocol(imageHandle, &loadedImageGuid, (void **)&loadedImage);
    if (status != EFI_SUCCESS || !loadedImage) return 0;
    status = gST->BootServices->HandleProtocol(loadedImage->DeviceHandle, &simpleFsGuid, (void **)&filesystem);
    if (status != EFI_SUCCESS || !filesystem) return 0;
    status = filesystem->OpenVolume(filesystem, &gBootVolumeRoot);
    if (status != EFI_SUCCESS || !gBootVolumeRoot) return 0;
    partition_registry_load();
    gPartitionRoots[0] = gBootVolumeRoot;
    if (storage_volume_has_label(gBootVolumeRoot, "TINYGPT")) {
        gPartitionRoots[1] = gBootVolumeRoot;
        gVolumeRoot = gBootVolumeRoot;
        gLegacySinglePartition = 1;
    } else {
        status = gST->BootServices->LocateHandleBuffer(0, (EFI_GUID *)0, (void *)0,
            &handleCount, &handles);
        if (status == EFI_SUCCESS && handles) {
            for (index = 0; index < handleCount; index++)
                gST->BootServices->ConnectController(handles[index], (EFI_HANDLE *)0, (void *)0, 1);
            gST->BootServices->FreePool(handles);
            handles = (EFI_HANDLE *)0;
            handleCount = 0;
        }
        status = gST->BootServices->LocateHandleBuffer(2, &simpleFsGuid, (void *)0,
            &handleCount, &handles);
        if (status == EFI_SUCCESS && handles) {
            for (index = 0; index < handleCount; index++) {
                EFI_FILE_PROTOCOL *candidateRoot = (EFI_FILE_PROTOCOL *)0;
                UINTN partition;
                int matched = 0;
                filesystem = (EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *)0;
                if (gST->BootServices->HandleProtocol(handles[index], &simpleFsGuid,
                        (void **)&filesystem) != EFI_SUCCESS || !filesystem ||
                    filesystem->OpenVolume(filesystem, &candidateRoot) != EFI_SUCCESS ||
                    !candidateRoot) continue;
                for (partition = 1U; partition < PARTITION_MAX; partition++) {
                    if (gPartitionNames[partition][0] &&
                        storage_volume_has_label(candidateRoot, gPartitionNames[partition])) {
                        gPartitionRoots[partition] = candidateRoot;
                        matched = 1;
                        break;
                    }
                }
                if (!matched) candidateRoot->Close(candidateRoot);
            }
            gST->BootServices->FreePool(handles);
        }
    }
    if (!gLegacySinglePartition) {
        gVolumeRoot = gPartitionRoots[1];
        if (!gVolumeRoot) {
            for (index = 2U; index < PARTITION_MAX; index++) {
                if (gPartitionRoots[index]) {
                    gVolumeRoot = gPartitionRoots[index];
                    gActivePartition = index + 1U;
                    break;
                }
            }
        }
    }
    if (!gVolumeRoot) return 0;
    gDedicatedStorage = (UINT8)storage_volume_is_dedicated();
    return 1;
}

typedef struct {
    UINT64 attribute;
    CHAR16 name[260];
} STORAGE_ENTRY;

static int storage_file_info_valid(EFI_FILE_INFO *information, UINTN bytes, UINTN *nameCharacters) {
    UINTN available;
    UINTN index;
    if (bytes < EFI_FILE_INFO_NAME_BASE + sizeof(CHAR16) ||
        information->Size < EFI_FILE_INFO_NAME_BASE + sizeof(CHAR16) ||
        information->Size > bytes) return 0;
    available = (information->Size - EFI_FILE_INFO_NAME_BASE) / sizeof(CHAR16);
    for (index = 0; index < available; index++) {
        if (!information->FileName[index]) {
            *nameCharacters = index;
            return index != 0;
        }
    }
    return 0;
}

static int storage_dot_entry(const CHAR16 *name) {
    return name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2]));
}

static int storage_collect_entries(EFI_FILE_PROTOCOL *directory, STORAGE_ENTRY **entriesOut, UINTN *countOut) {
    void *informationBuffer = (void *)0;
    UINTN informationCapacity = EFI_FILE_INFO_CAPACITY;
    STORAGE_ENTRY *entries = (STORAGE_ENTRY *)0;
    UINTN entryCapacity = 0;
    UINTN count = 0;
    EFI_STATUS status;
    if (gST->BootServices->AllocatePool(2, informationCapacity, &informationBuffer) != EFI_SUCCESS) return 0;
    status = directory->SetPosition(directory, 0);
    if (status != EFI_SUCCESS) goto failure;
    for (;;) {
        EFI_FILE_INFO *information;
        UINTN bytes = informationCapacity;
        UINTN nameCharacters = 0;
        status = directory->Read(directory, &bytes, informationBuffer);
        if (status == EFI_BUFFER_TOO_SMALL) {
            void *larger = (void *)0;
            if (bytes <= informationCapacity || bytes > 65536U ||
                gST->BootServices->AllocatePool(2, bytes, &larger) != EFI_SUCCESS) goto failure;
            gST->BootServices->FreePool(informationBuffer);
            informationBuffer = larger;
            informationCapacity = bytes;
            continue;
        }
        if (status != EFI_SUCCESS) goto failure;
        if (!bytes) break;
        information = (EFI_FILE_INFO *)informationBuffer;
        if (!storage_file_info_valid(information, bytes, &nameCharacters)) goto failure;
        if (storage_dot_entry(information->FileName)) continue;
        if (nameCharacters + 1 > sizeof(entries[0].name) / sizeof(entries[0].name[0])) goto failure;
        if (count == entryCapacity) {
            STORAGE_ENTRY *larger;
            UINTN newCapacity = entryCapacity ? entryCapacity * 2 : 8;
            if (newCapacity < entryCapacity ||
                gST->BootServices->AllocatePool(2, newCapacity * sizeof(STORAGE_ENTRY), (void **)&larger) != EFI_SUCCESS) goto failure;
            if (entries) {
                memory_copy(larger, entries, count * sizeof(STORAGE_ENTRY));
                gST->BootServices->FreePool(entries);
            }
            entries = larger;
            entryCapacity = newCapacity;
        }
        entries[count].attribute = information->Attribute;
        memory_copy(entries[count].name, information->FileName, (nameCharacters + 1) * sizeof(CHAR16));
        count++;
    }
    gST->BootServices->FreePool(informationBuffer);
    *entriesOut = entries;
    *countOut = count;
    return 1;
failure:
    if (entries) gST->BootServices->FreePool(entries);
    gST->BootServices->FreePool(informationBuffer);
    *entriesOut = (STORAGE_ENTRY *)0;
    *countOut = 0;
    return 0;
}

static int storage_clear_read_only(EFI_FILE_PROTOCOL *file) {
    EFI_FILE_INFO *information = (EFI_FILE_INFO *)0;
    UINTN bytes = 0;
    UINTN nameCharacters = 0;
    EFI_STATUS status = file->GetInfo(file, &gFileInfoGuid, &bytes, (void *)0);
    if (status != EFI_BUFFER_TOO_SMALL || bytes > 65536U ||
        gST->BootServices->AllocatePool(2, bytes, (void **)&information) != EFI_SUCCESS) return 0;
    status = file->GetInfo(file, &gFileInfoGuid, &bytes, information);
    if (status == EFI_SUCCESS && storage_file_info_valid(information, bytes, &nameCharacters)) {
        if (information->Attribute & EFI_FILE_READ_ONLY) {
            information->Attribute &= ~EFI_FILE_READ_ONLY;
            status = file->SetInfo(file, &gFileInfoGuid, information->Size, information);
        }
    }
    gST->BootServices->FreePool(information);
    return status == EFI_SUCCESS;
}

static EFI_STATUS storage_open_for_delete(EFI_FILE_PROTOCOL *directory, CHAR16 *path, EFI_FILE_PROTOCOL **file) {
    EFI_STATUS status = directory->Open(directory, file, path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (status == EFI_SUCCESS && *file) return EFI_SUCCESS;
    *file = (EFI_FILE_PROTOCOL *)0;
    status = directory->Open(directory, file, path, EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS || !*file) return status;
    storage_clear_read_only(*file);
    (*file)->Close(*file);
    *file = (EFI_FILE_PROTOCOL *)0;
    return directory->Open(directory, file, path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
}

static int storage_wipe_directory(EFI_FILE_PROTOCOL *directory, UINTN depth,
                                  UINTN *removed, UINTN *failures) {
    STORAGE_ENTRY *entries = (STORAGE_ENTRY *)0;
    UINTN count = 0;
    UINTN index;
    int complete = 1;
    if (!storage_collect_entries(directory, &entries, &count)) {
        (*failures)++;
        return 0;
    }
    for (index = 0; index < count; index++) {
        EFI_FILE_PROTOCOL *child = (EFI_FILE_PROTOCOL *)0;
        int keepDirectory =
            (depth == 0 && char16_equals_ascii(entries[index].name, "EFI")) ||
            (depth == 1 && char16_equals_ascii(entries[index].name, "BOOT"));
        int keepManager = depth == 2 &&
            char16_equals_ascii(entries[index].name, "BOOTAA64.EFI");
        EFI_STATUS status = storage_open_for_delete(directory, entries[index].name, &child);
        if (status != EFI_SUCCESS || !child) {
            (*failures)++;
            complete = 0;
            continue;
        }
        if (keepManager) {
            child->Close(child);
            continue;
        }
        if (entries[index].attribute & EFI_FILE_READ_ONLY) storage_clear_read_only(child);
        if (entries[index].attribute & EFI_FILE_DIRECTORY) {
            if (!storage_wipe_directory(child, keepDirectory ? depth + 1U : 99U,
                                        removed, failures)) complete = 0;
        }
        if (keepDirectory) {
            child->Close(child);
            continue;
        }
        status = child->Delete(child);
        if (status == EFI_SUCCESS) (*removed)++;
        else {
            (*failures)++;
            complete = 0;
        }
    }
    if (entries) gST->BootServices->FreePool(entries);
    return complete;
}

static int storage_delete_path(const CHAR16 *path, UINTN *removed, UINTN *failures) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    EFI_STATUS status = storage_open_for_delete(gVolumeRoot, (CHAR16 *)path, &file);
    if (status == EFI_NOT_FOUND) return 1;
    if (status != EFI_SUCCESS || !file) {
        (*failures)++;
        return 0;
    }
    storage_clear_read_only(file);
    status = file->Delete(file);
    if (status == EFI_SUCCESS) {
        (*removed)++;
        return 1;
    }
    (*failures)++;
    return 0;
}

static void storage_delete_owned_startup(UINTN *removed, UINTN *failures) {
    static const UINT8 expected[] = "fs0:\\EFI\\BOOT\\BOOTAA64.EFI\r\n";
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    UINT8 contents[sizeof(expected)];
    UINTN bytes = sizeof(contents);
    UINTN index;
    int matches = 1;
    EFI_STATUS status = storage_open_for_delete(gVolumeRoot, (CHAR16 *)gStartupPath, &file);
    if (status == EFI_NOT_FOUND) return;
    if (status != EFI_SUCCESS || !file) {
        (*failures)++;
        return;
    }
    status = file->Read(file, &bytes, contents);
    if (status != EFI_SUCCESS || bytes != sizeof(expected) - 1) matches = 0;
    for (index = 0; matches && index < bytes; index++) {
        if (contents[index] != expected[index]) matches = 0;
    }
    if (!matches) {
        file->Close(file);
        return;
    }
    status = file->Delete(file);
    if (status == EFI_SUCCESS) (*removed)++;
    else (*failures)++;
}

static int storage_set_os_missing(void) {
    static const UINT8 marker[] = "TinyGPT is not installed\n";
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    UINTN bytes = sizeof(marker) - 1U;
    EFI_STATUS status;
    if (!gVolumeRoot) return 0;
    status = gVolumeRoot->Open(gVolumeRoot, &file, (CHAR16 *)gOsMissingPath,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (status != EFI_SUCCESS || !file) return 0;
    status = file->SetPosition(file, 0);
    if (status == EFI_SUCCESS) status = file->Write(file, &bytes, (void *)marker);
    if (status == EFI_SUCCESS && bytes == sizeof(marker) - 1U) status = file->Flush(file);
    file->Close(file);
    return status == EFI_SUCCESS && bytes == sizeof(marker) - 1U;
}

static int storage_delete_marker(const CHAR16 *path) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    EFI_STATUS status;
    if (!gVolumeRoot) return 0;
    status = storage_open_for_delete(gVolumeRoot, (CHAR16 *)path, &file);
    if (status == EFI_NOT_FOUND) return 1;
    if (status != EFI_SUCCESS || !file) return 0;
    return file->Delete(file) == EFI_SUCCESS;
}

static int storage_clear_os_missing(void) {
    return storage_delete_marker(gOsMissingPath);
}

static int storage_os_missing(void) {
    if (!gStorageReady || storage_path_exists(gOsMissingPath)) return 1;
    return !storage_path_exists(gSlot0Path) && !storage_path_exists(gSlot1Path) &&
        !storage_path_exists(gFactoryInstallPath);
}

static int storage_wipe_owned_files(UINTN *removed, UINTN *failures) {
    CHAR16 savePath[] = {'\\','D','O','O','M','S','A','V','0','.','D','S','G',0};
    UINTN index;
    storage_delete_path(gSlot0Path, removed, failures);
    storage_delete_path(gSlot1Path, removed, failures);
    storage_delete_path(gDoomWadPath, removed, failures);
    storage_delete_path(gDoomConfigPath, removed, failures);
    for (index = 0; index < 10; index++) {
        savePath[8] = (CHAR16)('0' + index);
        storage_delete_path(savePath, removed, failures);
    }
    storage_delete_owned_startup(removed, failures);
    storage_delete_path(gBootBackupPath, removed, failures);
    storage_delete_path(gBootStagePath, removed, failures);
    storage_delete_path(gFactoryInstallPath, removed, failures);
    if (!storage_set_os_missing()) (*failures)++;
    return *failures == 0 && storage_path_exists(gBootPath) && storage_path_exists(gOsMissingPath);
}

static int storage_wipe_os(UINTN *removed, UINTN *failures) {
    int complete;
    STORAGE_ENTRY *remaining = (STORAGE_ENTRY *)0;
    UINTN remainingCount = 0;
    *removed = 0;
    *failures = 0;
    if (!gVolumeRoot) return 0;
    if (!gLegacySinglePartition) {
        complete = storage_wipe_directory(gVolumeRoot, 99U, removed, failures);
        if (!storage_collect_entries(gVolumeRoot, &remaining, &remainingCount)) {
            (*failures)++;
            complete = 0;
        } else if (remainingCount) {
            (*failures) += remainingCount;
            complete = 0;
        }
        if (remaining) gST->BootServices->FreePool(remaining);
        return complete && *failures == 0;
    }
    if (!gDedicatedStorage) return storage_wipe_owned_files(removed, failures);
    complete = storage_wipe_directory(gVolumeRoot, 0, removed, failures);
    if (!storage_set_os_missing()) {
        (*failures)++;
        complete = 0;
    }
    return complete && *failures == 0 &&
        storage_path_exists(gBootPath) && storage_path_exists(gOsMissingPath);
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

static int storage_activate_partition(UINTN partition) {
    if (partition < 2U || partition > PARTITION_MAX || !gPartitionRoots[partition - 1U]) return 0;
    gVolumeRoot = gPartitionRoots[partition - 1U];
    gActivePartition = partition;
    gDedicatedStorage = 1U;
    gCwd = FS_ROOT;
    gPreviousCwd = FS_ROOT;
    gGeneration = 0;
    memory_zero(gSlotValid, sizeof(gSlotValid));
    memory_zero(gSlotGeneration, sizeof(gSlotGeneration));
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

static void fs_remove_recursive(UINTN node);
static int fs_is_ancestor(UINTN ancestor, UINTN node);

static int fs_remove_legacy_manager_trees(void) {
    int legacyRoot = fs_find_child(FS_ROOT, "recovery");
    int apps = fs_find_child(FS_ROOT, "apps");
    int legacyApp = apps >= 0 && gNodes[apps].type == FS_DIRECTORY
        ? fs_find_child((UINTN)apps, "recovery") : -1;
    int retiredBootManagerApp = apps >= 0 && gNodes[apps].type == FS_DIRECTORY
        ? fs_find_child((UINTN)apps, "bootmgr") : -1;
    int changed = 0;
    if (legacyRoot >= 0) {
        if (fs_is_ancestor((UINTN)legacyRoot, gCwd)) gCwd = FS_ROOT;
        if (fs_is_ancestor((UINTN)legacyRoot, gPreviousCwd)) gPreviousCwd = FS_ROOT;
        fs_remove_recursive((UINTN)legacyRoot);
        changed = 1;
    }
    if (legacyApp >= 0) {
        if (fs_is_ancestor((UINTN)legacyApp, gCwd)) gCwd = FS_ROOT;
        if (fs_is_ancestor((UINTN)legacyApp, gPreviousCwd)) gPreviousCwd = FS_ROOT;
        fs_remove_recursive((UINTN)legacyApp);
        changed = 1;
    }
    if (retiredBootManagerApp >= 0) {
        if (fs_is_ancestor((UINTN)retiredBootManagerApp, gCwd)) gCwd = FS_ROOT;
        if (fs_is_ancestor((UINTN)retiredBootManagerApp, gPreviousCwd)) gPreviousCwd = FS_ROOT;
        fs_remove_recursive((UINTN)retiredBootManagerApp);
        changed = 1;
    }
    return changed;
}

static int fs_restore_system(void) {
    int migrated = fs_remove_legacy_manager_trees();
    int system = fs_ensure_dir(FS_ROOT, "system", FS_PROTECTED);
    int apps = fs_ensure_dir(FS_ROOT, "apps", FS_PROTECTED);
    int home = fs_ensure_dir(FS_ROOT, "home", 0);
    int boot = -1;
    int config = -1;
    int drivers = -1;
    int runtime = -1;
    int kernel = -1;
    int firmware = -1;
    int security = -1;
    int doomApp = -1;
    int editorApp = -1;
    int shellApp = -1;
    const char *editorAppInfo =
        "name=TinyGPT Text Editor\nkind=native full-screen app\ncommand=textedit [PATH]\nfile_picker=interactive when PATH is omitted\nformat=ASCII text\ndisplay_wrap=soft at screen edge\nscroll=Up/Down arrow keys\nfile_limit=8191 bytes\nprotected_paths=require protect unlock";
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
            "TinyGPT " TINYGPT_DISPLAY_VERSION "\narchitecture=ARM64\nfirmware=UEFI\nkernel=freestanding\nfilesystem=MiniFS2\nunix=no", FS_PROTECTED);
        fs_ensure_file((UINTN)system, "manifest.txt",
            "Critical tree:\n/system/boot       loader and pre-OS handoff records\n/system/kernel     core, ABI, and memory records\n/system/firmware   UEFI interface records\n/system/config     boot and shell policy\n/system/drivers    hardware service records\n/system/runtime    MiniFS2 runtime records\n/system/security   integrity and protected paths\n/apps              installed native applications", FS_PROTECTED);
    }
    if (boot >= 0) {
        int retiredManagerInfo = fs_find_child((UINTN)boot, "boot-manager.info");
        if (retiredManagerInfo >= 0) {
            fs_remove_recursive((UINTN)retiredManagerInfo);
            migrated = 1;
        }
        fs_ensure_file((UINTN)boot, "BOOTAA64.EFI.info",
            "critical=yes\ntype=ARM64 UEFI application\nentry=EfiMain\nsource=/src/uefi.c\ndisk=EFI/BOOT/BOOTAA64.EFI", FS_PROTECTED);
        fs_ensure_file((UINTN)boot, "startup.nsh.info",
            "critical=yes\nfirmware fallback=FS0:\\EFI\\BOOT\\BOOTAA64.EFI", FS_PROTECTED);
        fs_ensure_file((UINTN)boot, "boot-chain.info",
            "UEFI firmware -> BOOTAA64.EFI pre-OS environment -> verified TinyGPT shell", FS_PROTECTED);
        fs_ensure_file((UINTN)boot, "pre-os.info",
            "name=TinyGPT Pre-OS Environment\nphase=before operating system\nboot hotkey=R\nmenu hotkey=Enter\npartition 1=protected recovery\ntargeted maintenance=yes\npartition creation=yes\nmissing OS=open automatically\nintegrity errors=open automatically\nscrollback=256 lines", FS_PROTECTED);
    }
    if (kernel >= 0) {
        fs_ensure_file((UINTN)kernel, "kernel.info",
            "critical=yes\nmodel=single-address-space\narchitecture=AArch64\nentry=EfiMain\nservices=shell,MiniFS2,apps", FS_PROTECTED);
        fs_ensure_file((UINTN)kernel, "abi.info",
            "freestanding C17\nunix_abi=no\nposix=no\nsyscalls=native TinyGPT services", FS_PROTECTED);
        fs_ensure_file((UINTN)kernel, "memory.map",
            "core=static image\nfilesystem=static checked nodes\napplications=UEFI pool\nboot_services=active", FS_PROTECTED);
    }
    if (firmware >= 0) {
        fs_ensure_file((UINTN)firmware, "uefi.info",
            "critical=yes\ninterface=UEFI ARM64\nloader=BOOTAA64.EFI\nwatchdog=disabled while OS runs", FS_PROTECTED);
        fs_ensure_file((UINTN)firmware, "protocols.info",
            "SimpleTextInput\nSimpleTextOutput\nSimpleFileSystem\nBlockIO\nLoadedImage\nGraphicsOutput\nHttpServiceBinding (optional)\nHttp/TLS (optional)\nRuntimeServices", FS_PROTECTED);
    }
    if (config >= 0) {
        fs_ensure_file((UINTN)config, "boot.cfg",
            "pre_os_environment=auto\npre_os_hotkey=R\nboot_menu_hotkey=Enter\npre_os_window=2s\npartition_default=BOOTORD.CFG\npartition_registry=PARTS.CFG\nsnapshots=2\nintegrity_scan=scan N\nwatchdog=disabled", FS_PROTECTED);
        fs_ensure_file((UINTN)config, "shell.cfg",
            "home=/home\napps=/apps\ntemporary=/tmp\nprompt=tinygpt\nsettings=/home/.tinygptrc\nnavigation=cd", FS_PROTECTED);
        fs_ensure_file((UINTN)config, "protection.cfg",
            "protected=/system,/apps,/lost+found\ndefault=locked\nunlock=protect unlock\nscope=current-boot", FS_PROTECTED);
    }
    if (drivers >= 0) {
        fs_ensure_file((UINTN)drivers, "graphics.info", "UEFI Graphics Output Protocol\nconsumer=Freedoom\nmode=firmware-native", FS_PROTECTED);
        fs_ensure_file((UINTN)drivers, "input.info", "UEFI Simple Text Input\nkeyboard=polling\nrelease=simulated", FS_PROTECTED);
        fs_ensure_file((UINTN)drivers, "network.info", "UEFI HTTP/TLS\ntransport=firmware\naddress=IPv4 DHCP\nupdate channels=main,nightly beta\nrequired=firmware HTTP and CA trust", FS_PROTECTED);
        fs_ensure_file((UINTN)drivers, "storage.info", "UEFI Simple File System + BlockIO\npartition 1=TINYRECOV protected FAT16\npartition 2+=system/data FAT\nsnapshots=TINYFS0.BIN,TINYFS1.BIN per target", FS_PROTECTED);
        fs_ensure_file((UINTN)drivers, "timer.info", "ARM generic virtual counter\nclock=monotonic", FS_PROTECTED);
    }
    if (runtime >= 0) {
        fs_ensure_file((UINTN)runtime, "minifs2.info", "hierarchical=yes\nchecksums=FNV-1a\nauto_repair=yes\nmax_nodes=96\nfile_limit=8191", FS_PROTECTED);
        fs_ensure_file((UINTN)runtime, "mounts.info", "/            MiniFS2 read-write\n/system      protected\n/apps        protected\nEFI FAT32    platform storage", FS_PROTECTED);
        fs_ensure_file((UINTN)runtime, "snapshots.info", "copies=2\nstrategy=alternating\nselection=newest-valid\nfiles=TINYFS0.BIN,TINYFS1.BIN", FS_PROTECTED);
    }
    if (security >= 0) {
        fs_ensure_file((UINTN)security, "integrity.policy",
            "critical nodes use FNV-1a checksums\nupdates require HTTPS, SHA-256, and ARM64 PE validation\nverify at boot with scan\nauto-repair metadata\nrollback to newest valid snapshot", FS_PROTECTED);
        fs_ensure_file((UINTN)security, "protected.paths",
            "/system\n/apps\n/lost+found\nUnlock requires exact UNLOCK confirmation and expires at reboot.", FS_PROTECTED);
    }
    if (apps >= 0) {
        int previousEditor = fs_find_child((UINTN)apps, "editor");
        int previousEditorInfo = previousEditor >= 0 && gNodes[previousEditor].type == FS_DIRECTORY
            ? fs_find_child((UINTN)previousEditor, "app.info") : -1;
        if (previousEditorInfo < 0 || gNodes[previousEditorInfo].type != FS_FILE ||
            !streq(gNodes[previousEditorInfo].data, editorAppInfo)) migrated = 1;
        doomApp = fs_ensure_dir((UINTN)apps, "doom", FS_PROTECTED);
        editorApp = fs_ensure_dir((UINTN)apps, "editor", FS_PROTECTED);
        shellApp = fs_ensure_dir((UINTN)apps, "shell", FS_PROTECTED);
        fs_ensure_file((UINTN)apps, "registry.txt",
            "doom      command: doom\neditor    command: textedit [PATH]\nshell     built-in interactive shell", FS_PROTECTED);
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
    if (editorApp >= 0) {
        fs_ensure_file((UINTN)editorApp, "app.info", editorAppInfo, FS_PROTECTED);
        fs_ensure_file((UINTN)editorApp, "controls.txt",
            "Left/Right move by character\nUp/Down move through wrapped rows and scroll\nBackspace/Delete remove text\nEnter inserts a line\nF2 or Ctrl+S saves\nEsc exits; press twice to discard changes\nHome/End/PageUp/PageDown are unused", FS_PROTECTED);
    }
    if (shellApp >= 0) fs_ensure_file((UINTN)shellApp, "app.info",
        "name=TinyGPT Shell\nkind=built-in\nfilesystem=MiniFS2\ncommands=help,settings,textedit", FS_PROTECTED);
    if (home >= 0) {
        int homeReadme;
        const char *legacyReadme =
            "Easy navigation: home, root, up, back, go system, go apps, dir, open PATH. Try: sysfiles or apps";
        const char *shortcutReadme =
            "Navigation: home, root, up, back, go system, go apps, ls, open PATH. Try: sysfiles or apps";
        const char *newReadme =
            "Navigation: cd /system, cd /apps, cd .., cd -, and ls. Inspect files with cat PATH.";
        fs_ensure_dir((UINTN)home, "notes", 0);
        homeReadme = fs_find_child((UINTN)home, "readme.txt");
        if (homeReadme < 0) fs_ensure_file((UINTN)home, "readme.txt", newReadme, 0);
        else if (gNodes[homeReadme].type == FS_FILE &&
                 (streq(gNodes[homeReadme].data, legacyReadme) ||
                  streq(gNodes[homeReadme].data, shortcutReadme))) {
            fs_set_file((UINTN)homeReadme, newReadme);
            migrated = 1;
        }
    }
    return migrated;
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
    (void)fs_restore_system();
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
    settings_use_default_color();
    if (gNodes[directory].type == FS_FILE) {
        print(gNodes[directory].name);
        print("  ");
        print_u64(gNodes[directory].size);
        print(" bytes\n");
        return;
    }
    for (index = 1; index < FS_MAX_NODES; index++) {
        if (gNodes[index].used && gNodes[index].parent == directory) {
            if (gNodes[index].type == FS_DIRECTORY) {
                settings_use_accent_color();
                print("  <DIR>  ");
                print(gNodes[index].name);
                settings_use_default_color();
            } else {
                print("         ");
                print(gNodes[index].name);
                print("  ");
                print_u64(gNodes[index].size);
                print(" B");
            }
            if (fs_is_protected(index)) {
                settings_use_accent_color();
                print("  [system]");
                settings_use_default_color();
            }
            print("\n");
            found++;
        }
    }
    if (!found) print("  <empty>\n");
    settings_use_default_color();
}

static void fs_tree_node(UINTN node, UINTN depth) {
    UINTN index;
    UINTN spaces;
    for (index = 1; index < FS_MAX_NODES; index++) {
        if (gNodes[index].used && gNodes[index].parent == node) {
            settings_use_default_color();
            for (spaces = 0; spaces < depth; spaces++) print("  ");
            if (gNodes[index].type == FS_DIRECTORY) {
                settings_use_accent_color();
                print("+ ");
                print(gNodes[index].name);
                settings_use_default_color();
            } else {
                print("- ");
                print(gNodes[index].name);
            }
            print("\n");
            if (gNodes[index].type == FS_DIRECTORY && depth < 12) fs_tree_node(index, depth + 1);
        }
    }
    settings_use_default_color();
}

static void fs_tree(UINTN node) {
    char path[FS_PATH_BYTES];
    fs_path(node, path, sizeof(path));
    if (gNodes[node].type == FS_DIRECTORY) settings_use_accent_color();
    else settings_use_default_color();
    print(path);
    settings_use_default_color();
    print("\n");
    if (gNodes[node].type == FS_DIRECTORY) fs_tree_node(node, 1);
}

static void fs_change_directory(const char *path, int previous) {
    int node = previous ? (int)gPreviousCwd : fs_resolve(path);
    if (node < 0 || (UINTN)node >= FS_MAX_NODES || !gNodes[node].used) {
        print("cd: path not found\n");
        return;
    }
    if (gNodes[node].type != FS_DIRECTORY) {
        print("cd: not a directory\n");
        return;
    }
    {
        UINTN old = gCwd;
        gCwd = (UINTN)node;
        gPreviousCwd = old;
    }
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
        (void)fs_restore_system();
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

static int fs_scan_integrity(int verbose) {
    int errors = fs_check(0, verbose);
    if (!gStorageReady) {
        if (verbose) print("snapshots: unavailable (volatile storage)\n");
        return errors;
    }
    storage_probe_slots();
    if (verbose) {
        print("snapshot A: ");
        print(gSlotValid[0] ? "valid generation " : "missing/corrupt\n");
        if (gSlotValid[0]) { print_u64(gSlotGeneration[0]); print("\n"); }
        print("snapshot B: ");
        print(gSlotValid[1] ? "valid generation " : "missing/corrupt\n");
        if (gSlotValid[1]) { print_u64(gSlotGeneration[1]); print("\n"); }
    }
    if (!gSlotValid[0] && !gSlotValid[1]) {
        if (verbose) print("scan: no valid persistent snapshot\n");
        errors++;
    } else if (verbose && !errors) {
        print("scan: active file integrity verified\n");
    }
    return errors;
}

static int fs_commit(void) {
    if (!gStorageReady) return 0;
    if (!storage_sync()) {
        print("warning: persistent snapshot failed; RAM copy is still active\n");
        return 0;
    }
    return 1;
}

static int poll_input_key(EFI_INPUT_KEY *key) {
    return gST->ConIn->ReadKeyStroke(gST->ConIn, key) == EFI_SUCCESS;
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

static const char *settings_color_name(UINT8 color) {
    switch (color) {
        case 0: return "black";
        case 1: return "blue";
        case 2: return "green";
        case 3: return "cyan";
        case 4: return "red";
        case 5: return "magenta";
        case 6: return "brown";
        case 7: return "light gray";
        case 8: return "dark gray";
        case 9: return "light blue";
        case 10: return "light green";
        case 11: return "light cyan";
        case 12: return "light red";
        case 13: return "light magenta";
        case 14: return "yellow";
        case 15: return "white";
        default: return "light gray";
    }
}

static int settings_parse_uint8(const char *text, UINT8 *value) {
    UINTN parsed = 0;
    if (!text || !*text) return 0;
    while (*text) {
        if (*text < '0' || *text > '9') return 0;
        parsed = parsed * 10U + (UINTN)(*text - '0');
        if (parsed > 255U) return 0;
        text++;
    }
    *value = (UINT8)parsed;
    return 1;
}

static void settings_defaults(void) {
    gSettings.textColor = SETTINGS_DEFAULT_TEXT_COLOR;
    gSettings.accentColor = SETTINGS_DEFAULT_ACCENT_COLOR;
    gSettings.backgroundColor = SETTINGS_DEFAULT_BACKGROUND_COLOR;
    gSettings.showPromptPath = 1;
    gSettings.startupHome = 0;
    gSettings.scrollback = 1;
}

static UINTN settings_text_attribute(UINT8 foreground, UINT8 background) {
    return (UINTN)foreground | ((UINTN)background << 4);
}

static void settings_use_default_color(void) {
    gST->ConOut->SetAttribute(
        gST->ConOut, settings_text_attribute(gSettings.textColor, gSettings.backgroundColor));
}

static void settings_use_accent_color(void) {
    gST->ConOut->SetAttribute(
        gST->ConOut, settings_text_attribute(gSettings.accentColor, gSettings.backgroundColor));
}

static void settings_parse_config_line(char *line) {
    char *value = line;
    UINT8 parsed;
    while (*value && *value != '=') value++;
    if (*value != '=') return;
    *value++ = 0;
    if (!settings_parse_uint8(value, &parsed)) return;
    if (streq(line, "text_color")) {
        if (parsed >= 1U && parsed <= 15U) gSettings.textColor = parsed;
    } else if (streq(line, "accent_color")) {
        if (parsed >= 1U && parsed <= 15U) gSettings.accentColor = parsed;
    } else if (streq(line, "background_color")) {
        if (parsed <= 7U) gSettings.backgroundColor = parsed;
    } else if (streq(line, "prompt_path")) {
        if (parsed <= 1U) gSettings.showPromptPath = parsed;
    } else if (streq(line, "startup_home")) {
        if (parsed <= 1U) gSettings.startupHome = parsed;
    } else if (streq(line, "scrollback")) {
        if (parsed <= 1U) gSettings.scrollback = parsed;
    }
}

static void settings_load(void) {
    int node;
    UINTN position = 0;
    settings_defaults();
    node = fs_resolve("/home/.tinygptrc");
    if (node < 0 || gNodes[node].type != FS_FILE) return;
    while (position < gNodes[node].size) {
        char line[64];
        UINTN used = 0;
        int overflow = 0;
        while (position < gNodes[node].size &&
               gNodes[node].data[position] != '\n' && gNodes[node].data[position] != '\r') {
            char ch = gNodes[node].data[position++];
            if (!ch) {
                position = gNodes[node].size;
                break;
            }
            if (used + 1U < sizeof(line)) line[used++] = ch;
            else overflow = 1;
        }
        while (position < gNodes[node].size &&
               (gNodes[node].data[position] == '\n' || gNodes[node].data[position] == '\r')) position++;
        line[used] = 0;
        if (!overflow && used) settings_parse_config_line(line);
    }
    if (gSettings.backgroundColor == gSettings.textColor ||
        gSettings.backgroundColor == gSettings.accentColor) {
        gSettings.backgroundColor = SETTINGS_DEFAULT_BACKGROUND_COLOR;
    }
}

static void settings_append_uint8(char *buffer, UINT8 value, UINTN capacity) {
    char number[4];
    UINTN used = 0;
    if (value >= 100U) number[used++] = (char)('0' + value / 100U);
    if (value >= 10U) number[used++] = (char)('0' + (value / 10U) % 10U);
    number[used++] = (char)('0' + value % 10U);
    number[used] = 0;
    string_append(buffer, number, capacity);
}

static int settings_save(void) {
    char data[192];
    int node = fs_resolve("/home/.tinygptrc");
    if (node < 0) {
        int home = fs_resolve("/home");
        if (home < 0) return 0;
        node = fs_alloc(FS_FILE, (UINTN)home, ".tinygptrc", 0);
    }
    if (node < 0 || gNodes[node].type != FS_FILE) return 0;
    data[0] = 0;
    string_append(data, "text_color=", sizeof(data));
    settings_append_uint8(data, gSettings.textColor, sizeof(data));
    string_append(data, "\naccent_color=", sizeof(data));
    settings_append_uint8(data, gSettings.accentColor, sizeof(data));
    string_append(data, "\nbackground_color=", sizeof(data));
    settings_append_uint8(data, gSettings.backgroundColor, sizeof(data));
    string_append(data, "\nprompt_path=", sizeof(data));
    settings_append_uint8(data, gSettings.showPromptPath, sizeof(data));
    string_append(data, "\nstartup_home=", sizeof(data));
    settings_append_uint8(data, gSettings.startupHome, sizeof(data));
    string_append(data, "\nscrollback=", sizeof(data));
    settings_append_uint8(data, gSettings.scrollback, sizeof(data));
    string_append(data, "\n", sizeof(data));
    fs_set_file((UINTN)node, data);
    return gStorageReady && storage_sync();
}

static void settings_apply_runtime(void) {
    settings_use_default_color();
    if (gSettings.scrollback) {
        if (!gScrollbackEnabled) scrollback_enable();
    } else {
        gScrollbackEnabled = 0;
    }
}

static void settings_print_toggle(UINT8 enabled) {
    print(enabled ? "on" : "off");
}

static void settings_show(const char *notice) {
    settings_use_accent_color();
    print("=== TinyGPT Settings ===\n");
    settings_use_default_color();
    print("Changes save automatically. Choose 0 when finished.\n\n");
    print("  1  Default text color : "); print(settings_color_name(gSettings.textColor)); print("\n");
    print("  2  Accent color       : "); print(settings_color_name(gSettings.accentColor)); print("\n");
    print("  3  Background color   : "); print(settings_color_name(gSettings.backgroundColor)); print("\n");
    print("  4  Show path in prompt: "); settings_print_toggle(gSettings.showPromptPath); print("\n");
    print("  5  Startup directory  : "); print(gSettings.startupHome ? "/home" : "/"); print("\n");
    print("  6  Scrollback         : "); settings_print_toggle(gSettings.scrollback); print("\n");
    print("  7  Restore defaults\n");
    print("  0  Return to shell\n");
    if (notice && *notice) {
        settings_use_accent_color();
        print("\n");
        print(notice);
        print("\n");
        settings_use_default_color();
    }
}

static void settings_show_colors(UINT8 selected) {
    UINT8 color;
    settings_use_accent_color();
    print("=== Choose a Text Color ===\n");
    settings_use_default_color();
    print("Black is unavailable as a foreground choice.\n\n");
    for (color = 1; color <= 15U; color++) {
        gST->ConOut->SetAttribute(
            gST->ConOut, settings_text_attribute(color, gSettings.backgroundColor));
        print(color == selected ? "  > " : "    ");
        print_u64(color);
        print("  ");
        print(settings_color_name(color));
        print("\n");
    }
    settings_use_default_color();
    print("\n  0  Cancel\n");
}

static int settings_choose_color(UINT8 *target) {
    char answer[32];
    UINT8 color;
    gST->ConOut->ClearScreen(gST->ConOut);
    settings_show_colors(*target);
    print("\nColor number: ");
    read_line(answer, sizeof(answer));
    if (!settings_parse_uint8(answer, &color)) return -1;
    if (!color) return 0;
    if (color > 15U) return -1;
    if (color == gSettings.backgroundColor) return -2;
    *target = color;
    return 1;
}

static void settings_show_backgrounds(UINT8 selected) {
    UINT8 color;
    settings_use_accent_color();
    print("=== Choose a Background Color ===\n");
    settings_use_default_color();
    print("Each row previews the available background.\n\n");
    for (color = 0; color <= 7U; color++) {
        UINT8 foreground = color == 7U ? 0U : 15U;
        gST->ConOut->SetAttribute(
            gST->ConOut, settings_text_attribute(foreground, color));
        print(color == selected ? "  > " : "    ");
        print_u64((UINT64)color + 1U);
        print("  ");
        print(settings_color_name(color));
        print("\n");
    }
    settings_use_default_color();
    print("\n  0  Cancel\n");
}

static int settings_choose_background(UINT8 *target) {
    char answer[32];
    UINT8 option;
    UINT8 color;
    gST->ConOut->ClearScreen(gST->ConOut);
    settings_show_backgrounds(*target);
    print("\nBackground number: ");
    read_line(answer, sizeof(answer));
    if (!settings_parse_uint8(answer, &option)) return -1;
    if (!option) return 0;
    if (option > 8U) return -1;
    color = (UINT8)(option - 1U);
    if (color == gSettings.textColor || color == gSettings.accentColor) return -2;
    *target = color;
    return 1;
}

static const char *settings_save_notice(void) {
    return settings_save() ? "Saved automatically to /home/.tinygptrc." :
           "Applied for this boot; automatic save failed.";
}

static void command_settings(void) {
    char choice[32];
    const char *notice = (const char *)0;
    UINT8 previousScrollback = gScrollbackEnabled;
    gScrollbackEnabled = 0;
    for (;;) {
        int changed = 0;
        gST->ConOut->ClearScreen(gST->ConOut);
        settings_show(notice);
        print("\nSelect: ");
        read_line(choice, sizeof(choice));
        if (streq(choice, "0")) {
            gScrollbackEnabled = previousScrollback;
            settings_apply_runtime();
            gST->ConOut->ClearScreen(gST->ConOut);
            settings_use_default_color();
            print("Returned from Settings.\n");
            return;
        }
        if (streq(choice, "1")) changed = settings_choose_color(&gSettings.textColor);
        else if (streq(choice, "2")) changed = settings_choose_color(&gSettings.accentColor);
        else if (streq(choice, "3")) changed = settings_choose_background(&gSettings.backgroundColor);
        else if (streq(choice, "4")) {
            gSettings.showPromptPath = (UINT8)!gSettings.showPromptPath;
            changed = 1;
        } else if (streq(choice, "5")) {
            gSettings.startupHome = (UINT8)!gSettings.startupHome;
            changed = 1;
        } else if (streq(choice, "6")) {
            gSettings.scrollback = (UINT8)!gSettings.scrollback;
            changed = 1;
        } else if (streq(choice, "7")) {
            settings_defaults();
            changed = 1;
        } else {
            notice = "Unknown selection; choose 0 through 7.";
            continue;
        }
        if (changed > 0) {
            settings_use_default_color();
            notice = settings_save_notice();
        } else if (changed == -2) notice = "Text and accent colors must differ from the background.";
        else if (changed < 0 && streq(choice, "3")) notice = "Invalid background; choose 1 through 8.";
        else if (changed < 0) notice = "Invalid color; choose 1 through 15.";
        else notice = "Color change canceled.";
    }
}

#include "editor.inc"

static void command_protect(const char *command) {
    if (streq(command, "protect") || streq(command, "protect status")) {
        print(gProtectionUnlocked ? "protected nodes are UNLOCKED until reboot\n" :
              "protected nodes are locked\n");
    } else if (streq(command, "protect unlock")) {
        char answer[16];
        print("Type UNLOCK to allow protected-node changes this boot: ");
        read_line(answer, sizeof(answer));
        if (streq(answer, "UNLOCK")) {
            gProtectionUnlocked = 1;
            print("protection unlocked; run 'protect lock' when finished\n");
        } else print("unlock cancelled\n");
    } else if (streq(command, "protect lock")) {
        gProtectionUnlocked = 0;
        print("protected nodes locked\n");
    } else {
        print("protect: use status, unlock, or lock\n");
    }
}

#include "partition.inc"
#include "update.inc"
#include "doom_port.inc"

static void boot_stage(UINTN step, const char *label, int okay) {
    print("  [");
    print_u64(step);
    print("/5] ");
    print(label);
    print(okay ? " ... OK\n" : " ... PRE-OS ENVIRONMENT REQUIRED\n");
    delay_ms(120);
}

static int pre_os_partition_registered(UINTN partition) {
    return partition >= 1U && partition <= PARTITION_MAX &&
        gPartitionNames[partition - 1U][0] != 0;
}

static int pre_os_parse_partition(const char *text, UINTN *partitionOut) {
    UINT8 value;
    if (!settings_parse_uint8(text, &value) || value < 1U || value > PARTITION_MAX) return 0;
    *partitionOut = value;
    return 1;
}

static int pre_os_mount_target(UINTN partition, int verbose) {
    if (partition == 1U) {
        if (verbose) print("target: partition 1 is protected recovery storage\n");
        return 0;
    }
    if (!gStorageReady || !storage_activate_partition(partition)) {
        if (verbose) print("target: partition is unavailable; reboot after adding or renaming it\n");
        return 0;
    }
    if (!storage_mount_latest()) {
        if (verbose) print("target: no valid TinyGPT snapshot is installed\n");
        return 0;
    }
    return 1;
}

static int pre_os_bootable(UINTN partition, int verbose) {
    if (!pre_os_mount_target(partition, verbose)) return 0;
    if (storage_os_missing()) {
        if (verbose) print("boot: TinyGPT is not installed on that partition\n");
        return 0;
    }
    if (fs_scan_integrity(verbose)) {
        if (verbose) print("boot: repair or rollback is required\n");
        return 0;
    }
    return 1;
}

static int pre_os_repair(UINTN partition) {
    int mounted;
    if (partition == 1U) {
        print("repair: partition 1 is protected recovery storage\n");
        return 0;
    }
    if (!gStorageReady || !storage_activate_partition(partition)) {
        print("repair: target partition is unavailable\n");
        return 0;
    }
    mounted = storage_mount_latest();
    if (!mounted) {
        fs_format();
        gGeneration = 0;
        print("repair: installing TinyGPT filesystem on empty partition\n");
    } else {
        fs_check(1, 1);
    }
    if (!fs_commit()) {
        print("repair: repaired state could not be saved\n");
        return 0;
    }
    if (!storage_clear_os_missing()) {
        print("repair: could not mark TinyGPT as installed\n");
        return 0;
    }
    print("repair: target partition is bootable\n");
    return 1;
}

static void pre_os_print_partitions(UINTN selected) {
    UINTN partition;
    for (partition = 1U; partition <= PARTITION_MAX; partition++) {
        if (!pre_os_partition_registered(partition)) continue;
        print(partition == selected ? "  > " : "    ");
        print_u64(partition);
        print("  ");
        print(gPartitionNames[partition - 1U]);
        if (partition == 1U) print("  Pre-OS Recovery (protected)");
        else if (!gPartitionRoots[partition - 1U]) print("  (reboot required/unavailable)");
        print("\n");
    }
}

static UINTN pre_os_next_partition(UINTN selected, int direction) {
    UINTN tries;
    for (tries = 0; tries < PARTITION_MAX; tries++) {
        if (direction > 0) selected = selected == PARTITION_MAX ? 1U : selected + 1U;
        else selected = selected == 1U ? PARTITION_MAX : selected - 1U;
        if (pre_os_partition_registered(selected)) return selected;
    }
    return 1U;
}

static void pre_os_draw_boot_menu(UINTN selected, const char *status) {
    gST->ConOut->ClearScreen(gST->ConOut);
    settings_use_accent_color();
    print("=== Partition Boot Manager ===\n");
    settings_use_default_color();
    print("Choose a partition to boot. Recovery is protected and always partition 1.\n\n");
    pre_os_print_partitions(selected);
    print("\nUp/Down select, Enter boot, S save default, R recovery");
    if (status && *status) {
        print("\n");
        print(status);
    }
    print("\n");
}

static UINTN pre_os_boot_menu(void) {
    EFI_INPUT_KEY key;
    UINTN selected = boot_order_default_partition();
    if (!pre_os_partition_registered(selected)) selected = 2U;
    if (!pre_os_partition_registered(selected)) selected = 1U;
    pre_os_draw_boot_menu(selected, (const char *)0);
    for (;;) {
        if (!poll_input_key(&key)) {
            __asm__ volatile("yield");
            continue;
        }
        if (key.ScanCode == 1) {
            selected = pre_os_next_partition(selected, -1);
            pre_os_draw_boot_menu(selected, (const char *)0);
        } else if (key.ScanCode == 2) {
            selected = pre_os_next_partition(selected, 1);
            pre_os_draw_boot_menu(selected, (const char *)0);
        } else if (key.UnicodeChar >= '1' && key.UnicodeChar <= '9' &&
                   pre_os_partition_registered((UINTN)(key.UnicodeChar - '0'))) {
            return (UINTN)(key.UnicodeChar - '0');
        } else if (key.UnicodeChar == 'r' || key.UnicodeChar == 'R') return 1U;
        else if (key.UnicodeChar == '\r' || key.UnicodeChar == '\n') return selected;
        else if (key.UnicodeChar == 's' || key.UnicodeChar == 'S')
            pre_os_draw_boot_menu(
                selected,
                boot_order_save(selected) ? "Default boot partition saved." :
                                               "Could not save the default boot partition."
            );
    }
}

static UINTN pre_os_boot_prompt(void) {
    EFI_INPUT_KEY key;
    UINTN selected = boot_order_default_partition();
    UINT64 deadline = timer_count() + gTimerHz * 2U;
    if (!pre_os_partition_registered(selected)) selected = 2U;
    if (!pre_os_partition_registered(selected)) selected = 1U;
    print("\n  Default partition "); print_u64(selected); print(": ");
    print(gPartitionNames[selected - 1U]); print("\n");
    print("  Press Enter to interrupt boot and open the partition menu, or R for recovery.\n");
    while (timer_count() < deadline) {
        if (!poll_input_key(&key)) {
            __asm__ volatile("yield");
            continue;
        }
        if (key.UnicodeChar == 'r' || key.UnicodeChar == 'R') return 1U;
        if (key.UnicodeChar == '\r' || key.UnicodeChar == '\n') return pre_os_boot_menu();
    }
    return selected;
}

static int boot_screen(EFI_HANDLE imageHandle) {
    int mounted = 0;
    int errors = 1;
    int osMissing = 1;
    int snapshotFiles = 0;
    int factoryInstall = 0;
    UINTN targetPartition = 2U;
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
    print("  TinyGPT " TINYGPT_DISPLAY_VERSION " firmware startup\n\n");
    boot_stage(1, "ARM64 UEFI firmware and timer", 1);
    gStorageReady = (UINT8)storage_init(imageHandle);
    boot_stage(2, "TinyGPT boot volume", gStorageReady);
    if (gStorageReady) targetPartition = pre_os_boot_prompt();
    if (targetPartition == 1U) return 1;
    if (gStorageReady && storage_activate_partition(targetPartition)) {
        osMissing = storage_os_missing();
        snapshotFiles = storage_path_exists(gSlot0Path) || storage_path_exists(gSlot1Path);
        factoryInstall = storage_path_exists(gFactoryInstallPath);
        if (!osMissing) {
            mounted = storage_mount_latest();
            if (!mounted && !snapshotFiles && factoryInstall) {
                fs_format();
                gGeneration = 0;
                mounted = storage_sync();
                if (mounted) storage_delete_marker(gFactoryInstallPath);
            } else if (!mounted && !snapshotFiles) {
                osMissing = 1;
            }
        }
    }
    boot_stage(3, "TinyGPT system snapshot", mounted);
    if (mounted) errors = fs_scan_integrity(0);
    boot_stage(4, "system integrity and checksums", mounted && errors == 0);
    if (mounted && errors == 0 && fs_restore_system()) fs_commit();
    boot_stage(5, "TinyGPT operating system", !osMissing && mounted && errors == 0);
    if (osMissing) {
        print("\n  OS MISSING - OPENING PRE-OS ENVIRONMENT\n");
        delay_ms(150);
        return 1;
    }
    if (!mounted || errors) {
        print("\n  RECOVERY REQUIRED - OPENING PRE-OS ENVIRONMENT\n");
        delay_ms(150);
        return 1;
    }
    return 0;
}

static void pre_os_help(void) {
    print(
        "Pre-OS commands:\n"
        "  help             show every pre-OS command\n"
        "  partitions       list every registered GPT partition\n"
        "  partition add MIB NAME  create and name a FAT partition\n"
        "  partition name N NAME  rename a non-protected partition\n"
        "  use N            select a partition for file navigation\n"
        "  order            show the default boot partition\n"
        "  order N          set a numbered partition as the default\n"
        "  scan N           verify a partition and both snapshots\n"
        "  repair N         repair or install TinyGPT on a partition\n"
        "  rollback N       load that partition's previous snapshot\n"
        "  pwd              print the current directory\n"
        "  ls [PATH]        list a directory or file\n"
        "  cd [PATH|-]      change directory; no path opens /home\n"
        "  cat PATH         print a file\n"
        "  stat PATH        show file or directory metadata\n"
        "  tree [PATH]      show a directory tree\n"
        "  reset N          erase and reinstall a target partition\n"
        "  scroll           show scrollback status and keyboard controls\n"
        "  scroll clear     erase retained scrollback\n"
        "  boot [N]         verify and start the selected partition\n"
        "  reboot           restart TinyGPT\n"
        "  shutdown         power off the machine\n"
    );
}

static void pre_os_environment(void) {
    char line[128];
    if (!gScrollbackEnabled) scrollback_enable();
    gST->ConOut->ClearScreen(gST->ConOut);
    settings_use_accent_color();
    print("=== TinyGPT Pre-OS Environment ===\n");
    settings_use_default_color();
    print("TinyGPT has not started. Firmware recovery tools are active.\n");
    print(storage_os_missing() ? "Status: operating system missing or storage unavailable.\n" :
          "Status: operating system present; use 'boot' to start it.\n");
    print("Two checksummed snapshots protect the persistent filesystem.\n");
    print("Scrollback: 256 lines; Up/Down line, PageUp/PageDown page, Home oldest, End/Esc live.\n");
    pre_os_help();
    for (;;) {
        print("preos> ");
        read_line(line, sizeof(line));
        if (streq(line, "help")) {
            pre_os_help();
        } else if (streq(line, "partitions")) {
            UINTN defaultPartition = boot_order_default_partition();
            pre_os_print_partitions(gActivePartition);
            print("Default: "); print_u64(defaultPartition);
            print("  Active target: "); print_u64(gActivePartition); print("\n");
        } else if (starts_with(line, "partition add ")) {
            char *arguments = skip_spaces(line + 14);
            char *separator = arguments;
            UINT8 mebibytes;
            UINTN created = 0;
            while (*separator && *separator != ' ') separator++;
            if (*separator) *separator++ = 0;
            separator = skip_spaces(separator);
            if (!settings_parse_uint8(arguments, &mebibytes) || !*separator ||
                !partition_add(mebibytes, separator, &created)) {
                print("partition add: "); print(partition_error_text()); print("\n");
            } else {
                print("Created partition "); print_u64(created); print(" named ");
                print(gPartitionNames[created - 1U]);
                print(". Reboot once; TinyGPT will initialize it automatically.\n");
            }
        } else if (starts_with(line, "partition name ")) {
            char *arguments = skip_spaces(line + 15);
            char *separator = arguments;
            UINTN partition;
            while (*separator && *separator != ' ') separator++;
            if (*separator) *separator++ = 0;
            separator = skip_spaces(separator);
            if (!pre_os_parse_partition(arguments, &partition) || !*separator) {
                print("partition name: use partition name N NAME\n");
            } else if (partition == 1U) {
                print("partition name: partition 1 is protected recovery storage\n");
            } else if (!partition_rename(partition, separator)) {
                print("partition name: target must exist and NAME must be unique (1-11 letters, digits, _ or -)\n");
            } else {
                print("Partition renamed. Reboot before selecting it by name.\n");
            }
        } else if (starts_with(line, "use ")) {
            UINTN partition;
            if (!pre_os_parse_partition(skip_spaces(line + 4), &partition))
                print("use: provide a partition number\n");
            else if (pre_os_mount_target(partition, 1)) {
                print("Active target is partition "); print_u64(partition); print(" (");
                print(gPartitionNames[partition - 1U]); print(").\n");
            }
        } else if (streq(line, "order")) {
            UINTN partition = boot_order_default_partition();
            print("Default boot partition: "); print_u64(partition); print(" ");
            print(pre_os_partition_registered(partition) ? gPartitionNames[partition - 1U] : "unavailable");
            print("\n");
        } else if (starts_with(line, "order ")) {
            UINTN partition;
            if (!pre_os_parse_partition(skip_spaces(line + 6), &partition) ||
                !pre_os_partition_registered(partition))
                print("order: provide a registered partition number\n");
            else print(boot_order_save(partition) ? "Default boot partition saved.\n" :
                  "Could not save the default boot partition.\n");
        } else if (starts_with(line, "scan ")) {
            UINTN partition;
            if (!pre_os_parse_partition(skip_spaces(line + 5), &partition))
                print("scan: provide a non-protected partition number\n");
            else if (pre_os_mount_target(partition, 1)) {
                print("TinyGPT installation: present on partition "); print_u64(partition); print("\n");
                fs_scan_integrity(1);
            }
        } else if (streq(line, "scan")) {
            print("scan: provide a non-protected partition number\n");
        } else if (starts_with(line, "repair ")) {
            UINTN partition;
            if (!pre_os_parse_partition(skip_spaces(line + 7), &partition))
                print("repair: provide a non-protected partition number\n");
            else pre_os_repair(partition);
        } else if (streq(line, "repair")) {
            print("repair: provide a non-protected partition number\n");
        } else if (starts_with(line, "rollback ")) {
            UINTN partition;
            if (!pre_os_parse_partition(skip_spaces(line + 9), &partition))
                print("rollback: provide a non-protected partition number\n");
            else if (pre_os_mount_target(partition, 1)) {
                if (!storage_rollback()) print("rollback: no older valid snapshot\n");
                else {
                    fs_check(1, 1);
                    if (fs_commit() && storage_clear_os_missing())
                        print("rollback: previous snapshot restored\n");
                    else print("rollback: restored state could not be made bootable\n");
                }
            }
        } else if (streq(line, "rollback")) {
            print("rollback: provide a non-protected partition number\n");
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
        } else if (starts_with(line, "cat ")) {
            int node = fs_resolve(skip_spaces(line + 4));
            if (node < 0) print("cat: file not found\n");
            else if (gNodes[node].type != FS_FILE) print("cat: not a file\n");
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
            else fs_tree((UINTN)node);
        } else if (starts_with(line, "reset ")) {
            UINTN partition;
            char answer[24];
            if (!pre_os_parse_partition(skip_spaces(line + 6), &partition) || partition == 1U)
                print("reset: provide a non-protected partition number\n");
            else if (!storage_activate_partition(partition))
                print("reset: target partition is unavailable\n");
            else {
                print("Type RESET "); print_u64(partition); print(" to erase that partition's user files: ");
                read_line(answer, sizeof(answer));
                if (starts_with(answer, "RESET ")) {
                    UINTN confirmed;
                    if (pre_os_parse_partition(skip_spaces(answer + 6), &confirmed) && confirmed == partition) {
                        if (!storage_mount_latest()) gGeneration = 0;
                        fs_format();
                        if (fs_commit() && storage_clear_os_missing())
                            print("MiniFS2 reset complete; target partition is bootable\n");
                        else print("reset failed: repaired state was not saved\n");
                    } else print("reset cancelled\n");
                } else print("reset cancelled\n");
            }
        } else if (streq(line, "reset")) {
            print("reset: provide a non-protected partition number\n");
        } else if (streq(line, "scroll")) {
            print("Scrollback stores ");
            print_u64(gScrollbackCount);
            print("/256 lines. Use Up/Down for lines, PageUp/PageDown for pages; End or Esc returns live.\n");
        } else if (streq(line, "scroll clear")) {
            scrollback_reset();
            print("scrollback cleared\n");
        } else if (streq(line, "boot")) {
            if (pre_os_bootable(gActivePartition, 1)) return;
        } else if (starts_with(line, "boot ")) {
            UINTN partition;
            if (!pre_os_parse_partition(skip_spaces(line + 5), &partition))
                print("boot: provide a non-protected partition number\n");
            else if (pre_os_bootable(partition, 1)) return;
        } else if (streq(line, "reboot")) {
            gST->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, (void *)0);
            for (;;) __asm__ volatile("wfe");
        } else if (streq(line, "shutdown")) {
            gST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, (void *)0);
            for (;;) __asm__ volatile("wfe");
        } else if (*line) {
            print("Unknown pre-OS command. Type help.\n");
        }
    }
}

static void command_help(void) {
    print(
        "Shell commands:\n"
        "  help                 show this complete command reference\n"
        "  clear                clear the screen\n"
        "  scroll               show scrollback status and keyboard controls\n"
        "  scroll clear         erase retained scrollback\n"
        "  echo [TEXT]          print text or a blank line\n"
        "  info                 show OS, firmware, storage, and runtime details\n"
        "  uptime               show seconds since boot\n"
        "Navigation and discovery:\n"
        "  partitions           view disk partitions (manage them in pre-OS)\n"
        "  pwd                  print the current directory\n"
        "  ls [PATH]            list a directory or file\n"
        "  tree [PATH]          show a directory tree\n"
        "  cd [PATH|-]          change directory; no path opens /home\n"
        "Filesystem commands:\n"
        "  cat PATH             print a file\n"
        "  write PATH [TEXT]    create or replace a file; omit TEXT for an empty file\n"
        "  append PATH TEXT     append text to a file\n"
        "  mkdir PATH           create a directory\n"
        "  rm PATH              remove a file\n"
        "  rm -rf PATH          recursively remove a directory tree\n"
        "  rmdir PATH           remove an empty directory\n"
        "  cp SOURCE DEST       copy a file\n"
        "  mv SOURCE DEST       move or rename a node\n"
        "  stat PATH            show file or directory metadata\n"
        "  df                   show MiniFS node and byte usage\n"
        "  fsck                 verify filesystem structure and checksums\n"
        "Application and system commands:\n"
        "  textedit [PATH]      text editor; omit PATH for the interactive file picker\n"
        "  doom                 launch Freedoom; Q or F12 returns to the shell\n"
        "  settings             open the full-screen persistent settings UI\n"
        "  protect [status|unlock|lock] manage protected-node writes\n"
        "  update [check] [main|nightly]\n"
        "                       select and check/install an update channel\n"
        "  reboot               save MiniFS and restart TinyGPT\n"
        "  shutdown             save MiniFS and power off the machine\n"
        "Keyboard: Up/Down scroll lines; PageUp/PageDown scroll pages;\n"
        "          Home shows oldest output; End/Esc returns to live output.\n"
    );
}

static void command_info(void) {
    print("TinyGPT " TINYGPT_DISPLAY_VERSION "\n");
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
    if (!*command) return;
    if (streq(command, "help")) {
        command_help();
    } else if (streq(command, "clear")) {
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
    } else if (streq(command, "info")) {
        command_info();
    } else if (streq(command, "uptime")) {
        UINT64 elapsed = timer_count() - gStartTicks;
        print_u64(gTimerHz ? elapsed / gTimerHz : 0);
        print(" seconds\n");
    } else if (streq(command, "partitions")) {
        print("Disk partitions (read-only from TinyGPT):\n");
        pre_os_print_partitions(gActivePartition);
        print("Reboot and press R to enter the pre-OS recovery environment to manage partitions.\n");
    } else if (streq(command, "pwd")) {
        char path[FS_PATH_BYTES];
        fs_path(gCwd, path, sizeof(path));
        print(path);
        print("\n");
    } else if (streq(command, "ls") || starts_with(command, "ls ")) {
        char *path = streq(command, "ls") ? (char *)"" : skip_spaces(command + 3);
        int node = fs_resolve(path);
        if (node < 0) print("ls: path not found\n");
        else fs_list((UINTN)node);
    } else if (streq(command, "tree") || starts_with(command, "tree ")) {
        char *path = streq(command, "tree") ? (char *)"" : skip_spaces(command + 5);
        int node = fs_resolve(path);
        if (node < 0) print("tree: path not found\n");
        else fs_tree((UINTN)node);
    } else if (streq(command, "cd")) {
        fs_change_directory("/home", 0);
    } else if (starts_with(command, "cd ")) {
        char *path = skip_spaces(command + 3);
        fs_change_directory(path, streq(path, "-"));
    } else if (starts_with(command, "cat ")) {
        int node = fs_resolve(skip_spaces(command + 4));
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
        if (!path) print("write: expected PATH [TEXT]\n");
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
    } else if (starts_with(command, "mkdir ")) {
        char *path = skip_spaces(command + 6);
        UINTN parent;
        char name[FS_NAME_BYTES];
        int existing = fs_resolve(path);
        if (existing >= 0) print(gNodes[existing].type != FS_DIRECTORY ? "mkdir: file exists\n" : "already exists\n");
        else if (!fs_resolve_parent(path, &parent, name)) print("mkdir: invalid path\n");
        else if (fs_is_protected(parent) && !gProtectionUnlocked) print("mkdir: protected system path (use 'protect unlock')\n");
        else if (fs_alloc(FS_DIRECTORY, parent, name, 0) < 0) print("mkdir: filesystem full\n");
        else fs_commit();
    } else if (starts_with(command, "rm ") || starts_with(command, "rmdir ")) {
        int recursive = starts_with(command, "rm -rf ");
        int directory = recursive || starts_with(command, "rmdir ");
        char *path = skip_spaces(command + (recursive ? 7 : (directory ? 6 : 3)));
        int rootRequest = recursive && streq(path, "/");
        int node = rootRequest ? (int)FS_ROOT : (*path ? fs_resolve(path) : -1);
        if (node < 0) print("remove: path not found\n");
        else if (rootRequest && !gProtectionUnlocked) {
            print("rm -rf /: protected system is locked (use 'protect unlock')\n");
        } else if (rootRequest) {
            UINTN removed = 0;
            UINTN failures = 0;
            int complete;
            EFI_FILE_PROTOCOL *volume;
            if (!gVolumeRoot) {
                print("rm -rf /: EFI storage is unavailable; TinyGPT was not erased\n");
                return;
            }
            gCwd = FS_ROOT;
            gPreviousCwd = FS_ROOT;
            fs_remove_recursive(FS_ROOT);
            complete = storage_wipe_os(&removed, &failures);
            gStorageReady = 0;
            gSlotValid[0] = 0;
            gSlotValid[1] = 0;
            gSlotGeneration[0] = 0;
            gSlotGeneration[1] = 0;
            gGeneration = 0;
            print("removed ");
            print_u64(removed);
            print(" system-partition entries\n");
            if (!complete) {
                print("rm -rf /: PARTIAL FAILURE; ");
                print_u64(failures);
                print(" deletion operation(s) failed\n");
                print("Storage saving is disabled. Run rm -rf / again to retry.\n");
                return;
            }
            volume = gVolumeRoot;
            gVolumeRoot = (EFI_FILE_PROTOCOL *)0;
            gDedicatedStorage = 0;
            volume->Flush(volume);
            volume->Close(volume);
            print("TinyGPT files, snapshots, updater backups, and Freedoom data are gone.\n");
            print("The pre-OS environment remains and will open at the next boot. Powering off.\n");
            delay_ms(2000);
            gST->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, (void *)0);
            for (;;) __asm__ volatile("wfe");
        } else if ((UINTN)node == FS_ROOT) print("remove: only exact rm -rf / can erase root\n");
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
    } else if (starts_with(command, "mv ")) {
        char *destination;
        char *source = next_argument(command + 3, &destination);
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
    } else if (streq(command, "fsck")) {
        fs_check(0, 1);
    } else if (streq(command, "textedit") || starts_with(command, "textedit ")) {
        command_textedit(command);
    } else if (streq(command, "doom")) {
        print("Freedoom controls: WASD move, arrows turn, F fire, E use, Enter select, Esc menu.\n");
        print("Press Q (or F12) at any time to return to TinyGPT. Starting...\n");
        delay_ms(500);
        doom_run();
        settings_use_default_color();
    } else if (streq(command, "settings")) {
        command_settings();
    } else if (streq(command, "protect") || starts_with(command, "protect ")) {
        command_protect(command);
    } else if (streq(command, "update") || streq(command, "update main") ||
               streq(command, "update nightly") || streq(command, "update check") ||
               streq(command, "update check main") || streq(command, "update check nightly")) {
        int checkOnly = streq(command, "update check") ||
                        streq(command, "update check main") ||
                        streq(command, "update check nightly");
        int nightly = streq(command, "update nightly") ||
                      streq(command, "update check nightly");
        command_update(checkOnly, nightly);
    } else if (streq(command, "reboot")) {
        fs_commit();
        gST->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, (void *)0);
        for (;;) __asm__ volatile("wfe");
    } else if (streq(command, "shutdown")) {
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
    int preOsRequested;
    int startupNode;
    gST = systemTable;
    if (gST->BootServices->SetWatchdogTimer) gST->BootServices->SetWatchdogTimer(0, 0, 0, (CHAR16 *)0);
    gTimerHz = timer_frequency();
    gStartTicks = timer_count();
    gGeneration = 0;
    gProtectionUnlocked = 0;
    gScrollbackEnabled = 0;
    settings_defaults();
    gDoomStarted = 0;
    gCwd = FS_ROOT;
    gPreviousCwd = FS_ROOT;
    preOsRequested = boot_screen(image);
    if (preOsRequested) pre_os_environment();
    settings_load();
    startupNode = fs_resolve(gSettings.startupHome ? "/home" : "/");
    if (startupNode >= 0 && gNodes[startupNode].type == FS_DIRECTORY) {
        gCwd = (UINTN)startupNode;
        gPreviousCwd = gCwd;
    }

    settings_apply_runtime();
    gST->ConOut->ClearScreen(gST->ConOut);
    settings_use_accent_color();
    print("TinyGPT " TINYGPT_DISPLAY_VERSION);
    settings_use_default_color();
    print(" - ARM64 shell + MiniFS2\n");
    print("Pre-OS recovery: press R during firmware startup.\n\n");
    for (;;) {
        fs_path(gCwd, path, sizeof(path));
        settings_use_accent_color();
        print("tinygpt");
        settings_use_default_color();
        if (gSettings.showPromptPath) {
            print(":");
            print(path);
        }
        print("> ");
        read_line(line, sizeof(line));
        run_command(line);
    }
}
