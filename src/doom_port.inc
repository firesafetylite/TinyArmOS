// PureDOOM is GPL-2.0. See third_party/PureDOOM/LICENSE.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wparentheses"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wenum-compare"
#pragma clang diagnostic ignored "-Wunused-but-set-parameter"
#pragma clang diagnostic ignored "-Wdeprecated-non-prototype"
#pragma clang diagnostic ignored "-Wsometimes-uninitialized"
#define DOOM_IMPLEMENTATION
#include "../third_party/PureDOOM/PureDOOM.h"
#pragma clang diagnostic pop

typedef long long TINY_INT64;

typedef struct {
    EFI_FILE_PROTOCOL *file;
    UINT64 position;
    UINT64 size;
} TINY_DOOM_FILE;

typedef struct {
    doom_key_t key;
    UINT64 releaseAt;
    UINT8 active;
} TINY_DOOM_HELD_KEY;

static EFI_GRAPHICS_OUTPUT_PROTOCOL *gDoomGop;
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL *gDoomPixels;
static UINTN gDoomWidth;
static UINTN gDoomHeight;
static UINTN gDoomX;
static UINTN gDoomY;
static UINTN gDoomScale;
static UINT8 gDoomExitRequested;
static UINT8 gDoomStarted;
static TINY_DOOM_HELD_KEY gDoomHeld[16];

static void *doom_platform_malloc(int size) {
    void *memory = (void *)0;
    if (size <= 0) return (void *)0;
    if (gST->BootServices->AllocatePool(2, (UINTN)size, &memory) != EFI_SUCCESS) return (void *)0;
    return memory;
}

static void doom_platform_free(void *memory) {
    if (memory) gST->BootServices->FreePool(memory);
}

static void doom_platform_print(const char *message) {
    (void)message; // Keep the graphics screen clean; fatal launch checks happen before doom_init().
}

static void doom_ascii_path(const char *source, CHAR16 *destination, UINTN capacity) {
    UINTN used = 0;
    while (source[0] == '.' && (source[1] == '/' || source[1] == '\\')) source += 2;
    if (streq(source, ".doomrc")) source = "DOOM.CFG";
    if (capacity > 1) destination[used++] = '\\';
    while (*source && used + 1 < capacity) {
        char ch = *source++;
        destination[used++] = (CHAR16)(UINT8)(ch == '/' ? '\\' : ch);
    }
    destination[used] = 0;
}

static void *doom_platform_open(const char *filename, const char *mode) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    TINY_DOOM_FILE *handle;
    CHAR16 path[128];
    UINT64 openMode = EFI_FILE_MODE_READ;
    int writing = mode && (mode[0] == 'w' || mode[0] == 'a' || mode[1] == '+');
    doom_ascii_path(filename, path, sizeof(path) / sizeof(path[0]));
    if (!gVolumeRoot) return (void *)0;
    if (writing) openMode |= EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE;
    if (mode && mode[0] == 'w') {
        EFI_FILE_PROTOCOL *oldFile = (EFI_FILE_PROTOCOL *)0;
        if (gVolumeRoot->Open(gVolumeRoot, &oldFile, path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0) == EFI_SUCCESS && oldFile) {
            oldFile->Delete(oldFile);
        }
    }
    if (gVolumeRoot->Open(gVolumeRoot, &file, path, openMode, 0) != EFI_SUCCESS || !file) return (void *)0;
    handle = (TINY_DOOM_FILE *)doom_platform_malloc((int)sizeof(TINY_DOOM_FILE));
    if (!handle) {
        file->Close(file);
        return (void *)0;
    }
    handle->file = file;
    handle->position = 0;
    handle->size = 0;
    if (file->SetPosition(file, 0xffffffffffffffffULL) == EFI_SUCCESS) file->GetPosition(file, &handle->size);
    if (mode && mode[0] == 'a') {
        file->SetPosition(file, handle->size);
        handle->position = handle->size;
    } else file->SetPosition(file, 0);
    return handle;
}

static void doom_platform_close(void *opaque) {
    TINY_DOOM_FILE *handle = (TINY_DOOM_FILE *)opaque;
    if (!handle) return;
    if (handle->file) {
        handle->file->Flush(handle->file);
        handle->file->Close(handle->file);
    }
    doom_platform_free(handle);
}

static int doom_platform_read(void *opaque, void *buffer, int count) {
    TINY_DOOM_FILE *handle = (TINY_DOOM_FILE *)opaque;
    UINTN bytes;
    if (!handle || !handle->file || count <= 0) return 0;
    bytes = (UINTN)count;
    if (handle->file->Read(handle->file, &bytes, buffer) != EFI_SUCCESS) return 0;
    handle->position += bytes;
    return (int)bytes;
}

static int doom_platform_write(void *opaque, const void *buffer, int count) {
    TINY_DOOM_FILE *handle = (TINY_DOOM_FILE *)opaque;
    UINTN bytes;
    if (!handle || !handle->file || count <= 0) return 0;
    bytes = (UINTN)count;
    if (handle->file->Write(handle->file, &bytes, (void *)buffer) != EFI_SUCCESS) return 0;
    handle->position += bytes;
    if (handle->position > handle->size) handle->size = handle->position;
    return (int)bytes;
}

static int doom_platform_seek(void *opaque, int offset, doom_seek_t origin) {
    TINY_DOOM_FILE *handle = (TINY_DOOM_FILE *)opaque;
    TINY_INT64 target;
    if (!handle || !handle->file) return -1;
    if (origin == DOOM_SEEK_CUR) target = (TINY_INT64)handle->position + offset;
    else if (origin == DOOM_SEEK_END) target = (TINY_INT64)handle->size + offset;
    else target = offset;
    if (target < 0 || handle->file->SetPosition(handle->file, (UINT64)target) != EFI_SUCCESS) return -1;
    handle->position = (UINT64)target;
    return 0;
}

static int doom_platform_tell(void *opaque) {
    TINY_DOOM_FILE *handle = (TINY_DOOM_FILE *)opaque;
    return handle ? (int)handle->position : -1;
}

static int doom_platform_eof(void *opaque) {
    TINY_DOOM_FILE *handle = (TINY_DOOM_FILE *)opaque;
    return !handle || handle->position >= handle->size;
}

static void doom_platform_gettime(int *seconds, int *microseconds) {
    UINT64 ticks = timer_count();
    if (!gTimerHz) {
        *seconds = 0;
        *microseconds = 0;
        return;
    }
    *seconds = (int)(ticks / gTimerHz);
    *microseconds = (int)(((ticks % gTimerHz) * 1000000ULL) / gTimerHz);
}

static void doom_platform_exit(int code) {
    (void)code;
    gDoomExitRequested = 1;
}

static char *doom_platform_getenv(const char *name) {
    if (streq(name, "HOME") || streq(name, "DOOMWADDIR")) return (char *)".";
    return (char *)0;
}

static int doom_wad_available(void) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    CHAR16 path[] = {'\\','D','O','O','M','U','.','W','A','D',0};
    if (!gVolumeRoot) return 0;
    if (gVolumeRoot->Open(gVolumeRoot, &file, path, EFI_FILE_MODE_READ, 0) != EFI_SUCCESS || !file) return 0;
    file->Close(file);
    return 1;
}

static int doom_graphics_init(void) {
    static EFI_GUID gopGuid = {0x9042a9de, 0x23dc, 0x4a38, {0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}};
    UINTN horizontal;
    UINTN vertical;
    UINTN scale;
    UINTN pixelCount;
    if (gST->BootServices->LocateProtocol(&gopGuid, (void *)0, (void **)&gDoomGop) != EFI_SUCCESS ||
        !gDoomGop || !gDoomGop->Mode || !gDoomGop->Mode->Info) return 0;
    horizontal = gDoomGop->Mode->Info->HorizontalResolution;
    vertical = gDoomGop->Mode->Info->VerticalResolution;
    scale = horizontal / 320;
    if (vertical / 200 < scale) scale = vertical / 200;
    if (scale > 4) scale = 4;
    if (!scale) return 0;
    gDoomScale = scale;
    gDoomWidth = 320 * scale;
    gDoomHeight = 200 * scale;
    gDoomX = (horizontal - gDoomWidth) / 2;
    gDoomY = (vertical - gDoomHeight) / 2;
    pixelCount = gDoomWidth * gDoomHeight;
    if (gST->BootServices->AllocatePool(2, pixelCount * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL), (void **)&gDoomPixels) != EFI_SUCCESS) {
        gDoomPixels = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)0;
        return 0;
    }
    return 1;
}

static void doom_present(void) {
    const unsigned char *source = doom_get_framebuffer(4);
    UINTN y;
    UINTN x;
    UINTN sy;
    UINTN sx;
    if (!source || !gDoomPixels) return;
    for (y = 0; y < 200; y++) {
        for (sy = 0; sy < gDoomScale; sy++) {
            EFI_GRAPHICS_OUTPUT_BLT_PIXEL *row = gDoomPixels + (y * gDoomScale + sy) * gDoomWidth;
            for (x = 0; x < 320; x++) {
                const unsigned char *pixel = source + (y * 320 + x) * 4;
                for (sx = 0; sx < gDoomScale; sx++) {
                    row[x * gDoomScale + sx].Red = pixel[0];
                    row[x * gDoomScale + sx].Green = pixel[1];
                    row[x * gDoomScale + sx].Blue = pixel[2];
                    row[x * gDoomScale + sx].Reserved = 0;
                }
            }
        }
    }
    gDoomGop->Blt(gDoomGop, gDoomPixels, 2, 0, 0, gDoomX, gDoomY, gDoomWidth, gDoomHeight, 0);
}

static doom_key_t doom_map_key(EFI_INPUT_KEY *input) {
    if (input->UnicodeChar >= 'A' && input->UnicodeChar <= 'Z') input->UnicodeChar += 'a' - 'A';
    if (input->UnicodeChar >= 32 && input->UnicodeChar <= 126) return (doom_key_t)input->UnicodeChar;
    if (input->UnicodeChar == '\r' || input->UnicodeChar == '\n') return DOOM_KEY_ENTER;
    if (input->UnicodeChar == '\t') return DOOM_KEY_TAB;
    if (input->UnicodeChar == 8 || input->UnicodeChar == 127) return DOOM_KEY_BACKSPACE;
    if (input->ScanCode == 1) return DOOM_KEY_UP_ARROW;
    if (input->ScanCode == 2) return DOOM_KEY_DOWN_ARROW;
    if (input->ScanCode == 3) return DOOM_KEY_RIGHT_ARROW;
    if (input->ScanCode == 4) return DOOM_KEY_LEFT_ARROW;
    if (input->ScanCode == 11) return DOOM_KEY_F1;
    if (input->ScanCode == 12) return DOOM_KEY_F2;
    if (input->ScanCode == 13) return DOOM_KEY_F3;
    if (input->ScanCode == 14) return DOOM_KEY_F4;
    if (input->ScanCode == 15) return DOOM_KEY_F5;
    if (input->ScanCode == 16) return DOOM_KEY_F6;
    if (input->ScanCode == 17) return DOOM_KEY_F7;
    if (input->ScanCode == 18) return DOOM_KEY_F8;
    if (input->ScanCode == 19) return DOOM_KEY_F9;
    if (input->ScanCode == 20) return DOOM_KEY_F10;
    if (input->ScanCode == 21) return DOOM_KEY_F11;
    if (input->ScanCode == 23 || input->UnicodeChar == 27) return DOOM_KEY_ESCAPE;
    return DOOM_KEY_UNKNOWN;
}

static void doom_press_key(doom_key_t key) {
    UINTN index;
    UINT64 deadline = timer_count() + gTimerHz / 7;
    if (key == DOOM_KEY_UNKNOWN) return;
    for (index = 0; index < sizeof(gDoomHeld) / sizeof(gDoomHeld[0]); index++) {
        if (gDoomHeld[index].active && gDoomHeld[index].key == key) {
            gDoomHeld[index].releaseAt = deadline;
            return;
        }
    }
    for (index = 0; index < sizeof(gDoomHeld) / sizeof(gDoomHeld[0]); index++) {
        if (!gDoomHeld[index].active) {
            gDoomHeld[index].active = 1;
            gDoomHeld[index].key = key;
            gDoomHeld[index].releaseAt = deadline;
            doom_key_down(key);
            return;
        }
    }
}

static void doom_release_expired_keys(void) {
    UINTN index;
    UINT64 now = timer_count();
    for (index = 0; index < sizeof(gDoomHeld) / sizeof(gDoomHeld[0]); index++) {
        if (gDoomHeld[index].active && now >= gDoomHeld[index].releaseAt) {
            doom_key_up(gDoomHeld[index].key);
            gDoomHeld[index].active = 0;
        }
    }
}

static void doom_release_all_keys(void) {
    UINTN index;
    for (index = 0; index < sizeof(gDoomHeld) / sizeof(gDoomHeld[0]); index++) {
        if (gDoomHeld[index].active) doom_key_up(gDoomHeld[index].key);
        gDoomHeld[index].active = 0;
    }
}

static int doom_run(void) {
    char arg0[] = "doom";
    char *argv[] = {arg0};
    EFI_INPUT_KEY input;
    if (gDoomStarted) {
        print("Doom can run once per boot; reboot TinyGPT to play again.\n");
        return 0;
    }
    if (!doom_wad_available()) {
        print("doom: DOOMU.WAD (Freedoom Phase 1) is missing from the UEFI disk\n");
        return 0;
    }
    if (!doom_graphics_init()) {
        print("doom: UEFI Graphics Output Protocol is unavailable\n");
        return 0;
    }
    gDoomStarted = 1;
    gDoomExitRequested = 0;
    memory_zero(gDoomHeld, sizeof(gDoomHeld));
    doom_set_print(doom_platform_print);
    doom_set_malloc(doom_platform_malloc, doom_platform_free);
    doom_set_file_io(doom_platform_open, doom_platform_close, doom_platform_read, doom_platform_write,
                     doom_platform_seek, doom_platform_tell, doom_platform_eof);
    doom_set_gettime(doom_platform_gettime);
    doom_set_exit(doom_platform_exit);
    doom_set_getenv(doom_platform_getenv);
    doom_set_resolution(320, 200);
    doom_set_default_int("key_up", DOOM_KEY_W);
    doom_set_default_int("key_down", DOOM_KEY_S);
    doom_set_default_int("key_strafeleft", DOOM_KEY_A);
    doom_set_default_int("key_straferight", DOOM_KEY_D);
    doom_set_default_int("key_fire", DOOM_KEY_F);
    doom_set_default_int("key_use", DOOM_KEY_E);
    doom_set_default_int("always_run", 1);
    doom_set_default_int("use_mouse", 0);
    doom_init(1, argv, DOOM_FLAG_HIDE_MOUSE_OPTIONS | DOOM_FLAG_HIDE_SOUND_OPTIONS |
              DOOM_FLAG_HIDE_MUSIC_OPTIONS | DOOM_FLAG_MENU_DARKEN_BG);
    {
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL black = {0, 0, 0, 0};
        gDoomGop->Blt(gDoomGop, &black, 0, 0, 0, 0, 0,
            gDoomGop->Mode->Info->HorizontalResolution,
            gDoomGop->Mode->Info->VerticalResolution, 0);
    }
    while (!gDoomExitRequested) {
        while (gST->ConIn->ReadKeyStroke(gST->ConIn, &input) == EFI_SUCCESS) {
            if (input.ScanCode == 22 || input.UnicodeChar == 'q' || input.UnicodeChar == 'Q') {
                gDoomExitRequested = 1;
                break;
            }
            doom_press_key(doom_map_key(&input));
        }
        doom_release_expired_keys();
        doom_update();
        doom_present();
        __asm__ volatile("yield");
    }
    doom_release_all_keys();
    if (gDoomPixels) gST->BootServices->FreePool(gDoomPixels);
    gDoomPixels = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)0;
    gST->ConOut->ClearScreen(gST->ConOut);
    print("Returned from Freedoom. Reboot to launch it again.\n");
    return 1;
}
