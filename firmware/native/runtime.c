/* Native TinyGPT system and firmware share the application's interface adapter,
 * but have separate images, static state and heaps. No EDK II is linked. */
#include "api.h"
#define TINYGPT_NATIVE 1
static int native_install_system(void);
static unsigned long long native_selected_partition(void);
static int native_system_exists(void);
#include "../../src/uefi.c"
#include "platform.inc"

int memcmp(const void *left, const void *right, size_t bytes) {
    const unsigned char *a=left,*b=right;
    while (bytes--) { if (*a!=*b) return *a-*b; a++;b++; } return 0;
}
char *strchr(const char *text, int ch) { do { if (*text==(char)ch) return (char *)text; } while (*text++); return 0; }
size_t strlen(const char *text) { size_t bytes=0; while (text[bytes]) bytes++; return bytes; }
#ifndef TINYGPT_FIRMWARE
void *memcpy(void *destination, const void *source, size_t bytes) { memory_copy(destination,source,bytes); return destination; }
void *memset(void *destination, int value, size_t bytes) { unsigned char *out=destination; while(bytes--) *out++=(unsigned char)value; return destination; }
#endif
static unsigned long long native_selected_partition(void) { return native_api->boot_partition; }
static const CHAR16 native_system_path[]={'\\','T','I','N','Y','G','P','T','.','E','L','F',0};
static int native_system_exists(void) { return storage_path_exists(native_system_path); }
#ifdef TINYGPT_FIRMWARE
#include "../bios/bios.h"
extern int disk_init(void);
extern int disk_write_sector(uint64_t, const uint8_t *);
extern int disk_flush(void);
extern int disk_writable(void);
extern uint32_t *board_framebuffer(void);
extern int board_keyboard_init(void);
extern int board_keyboard_read(uint16_t *, uint16_t *);
extern const unsigned char native_os_start[], native_os_end[];
static NativeApi firmware_api;
static int native_install_system(void) {
    EFI_FILE_PROTOCOL *file=0;
    UINTN bytes=(UINTN)(native_os_end-native_os_start), written=bytes;
    if (!bytes || bytes>BIOS_FILE_LIMIT || !gVolumeRoot) return 0;
    EFI_STATUS result=gVolumeRoot->Open(gVolumeRoot,&file,(CHAR16 *)native_system_path,
        EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,0);
    if (result!=EFI_SUCCESS || !file) return 0;
    result=file->Write(file,&written,(void *)native_os_start);
    if (result==EFI_SUCCESS && written==bytes) {
        UINT64 information[96]; UINTN length=sizeof(information);
        result=file->GetInfo(file,&gFileInfoGuid,&length,information);
        if (result==EFI_SUCCESS) {
            EFI_FILE_INFO *info=(EFI_FILE_INFO *)information; info->FileSize=bytes;
            result=file->SetInfo(file,&gFileInfoGuid,info->Size,info);
        }
        if (result==EFI_SUCCESS) result=file->Flush(file);
    }
    file->Close(file);
    if (result!=EFI_SUCCESS || written!=bytes) return 0;
    /* Read-back SHA-256 verification before claiming that the native OS exists. */
    if (gVolumeRoot->Open(gVolumeRoot,&file,(CHAR16 *)native_system_path,EFI_FILE_MODE_READ,0)!=EFI_SUCCESS || !file) return 0;
    UINT8 buffer[4096], expected[32], actual[32]; SHA256_CONTEXT digest; sha256_init(&digest);
    UINTN total=0;
    for (;;) {
        UINTN amount=sizeof(buffer); result=file->Read(file,&amount,buffer);
        if (result!=EFI_SUCCESS || !amount) break;
        sha256_add(&digest,buffer,amount); total+=amount;
        if (total>bytes) { result=EFI_DEVICE_ERROR; break; }
    }
    file->Close(file); sha256_finish(&digest,actual); sha256_bytes(native_os_start,bytes,expected);
    return result==EFI_SUCCESS && total==bytes && memory_equal(actual,expected,32);
}
static int native_boot_system(void) {
    EFI_FILE_PROTOCOL *file=0; UINT8 *image=(UINT8 *)BIOS_STAGE; UINTN bytes=BIOS_FILE_LIMIT;
    if (!gVolumeRoot || gVolumeRoot->Open(gVolumeRoot,&file,(CHAR16 *)native_system_path,EFI_FILE_MODE_READ,0)!=EFI_SUCCESS || !file) return 0;
    EFI_STATUS result=file->Read(file,&bytes,image); UINT8 extra; UINTN trailing=1;
    if (result==EFI_SUCCESS) result=file->Read(file,&trailing,&extra);
    file->Close(file);
    Executable executable;
    if (result!=EFI_SUCCESS || trailing || !executable_parse(image,bytes,&executable)) return 0;
    static const char abi[]="TinyGPTNativeABI=3\n";
    int compatible=0;
    for (UINTN i=0;i+sizeof(abi)-1<=bytes;i++)
        if (image[i]=='T' && memory_equal(image+i,abi,sizeof(abi)-1)) { compatible=1; break; }
    if (!compatible) return 0;
    for (UINTN i=0;i<executable.count;i++) {
        Segment *segment=&executable.segments[i];
        memory_copy((void *)(UINTN)segment->destination,image+segment->offset,(UINTN)segment->files);
        memory_zero((void *)(UINTN)(segment->destination+segment->files),(UINTN)(segment->memory-segment->files));
    }
    native_api->flush();
    firmware_api.boot_partition=gActivePartition;
    print("Starting native TinyGPT from disk (no EDK II)...\n");
    __asm__ volatile("dsb sy; ic iallu; dsb sy; isb" ::: "memory");
    ((void (*)(const NativeApi *))(UINTN)executable.entry)(&firmware_api);
    return 0;
}
void bios_main(void) {
    *(volatile UINT32 *)0x09000030UL=0;
    *(volatile UINT32 *)0x09000024UL=13; *(volatile UINT32 *)0x09000028UL=1;
    *(volatile UINT32 *)0x0900002cUL=0x70; *(volatile UINT32 *)0x09000030UL=0x301;
    native_uart_text("\r\nTinyGPT BIOS: native firmware, no EDK II.\r\n");
    disk_init();
    firmware_api=(NativeApi){NATIVE_API_MAGIC,NATIVE_API_VERSION,0x50000000UL,disk_sectors(),
        disk_read,disk_write_sector,disk_flush,disk_writable,0,640,480,2,board_keyboard_read};
    firmware_api.framebuffer=board_framebuffer();
    board_keyboard_init();
    if (!native_initialize(&firmware_api)) for (;;) __asm__ volatile("wfe");
    gTimerHz=timer_frequency(); gStartTicks=timer_count(); settings_defaults(); gCwd=FS_ROOT; gPreviousCwd=FS_ROOT;
    int recovery=boot_screen(&native_image);
    if (!recovery && !native_boot_system()) print("Native system missing or invalid. Use authenticated repair to install it.\n");
    for (;;) {
        pre_os_environment();
        if (!native_boot_system()) print("Native system missing or invalid. Use authenticated repair to install it.\n");
    }
}
#else
static int native_install_system(void) { return 0; }
static const char native_abi_marker[] __attribute__((used,section(".native_abi")))="TinyGPTNativeABI=3\n";
void NativeMain(const NativeApi *api) {
    if (!native_initialize(api)) for (;;) __asm__ volatile("wfe");
    EfiMain(&native_image,&native_table);
}
#endif
