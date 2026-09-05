/* TinyGPT-owned protocol-shaped adapter. The ABI names ease migration; this is
 * not EDK II and does not advertise general-purpose UEFI compatibility. */
#include "../../third_party/FatFs/ff.h"
#define disk_read fatfs_disk_read
#define disk_write fatfs_disk_write
#include "../../third_party/FatFs/diskio.h"
#undef disk_read
#undef disk_write

static const NativeApi *native_api;
static EFI_SYSTEM_TABLE native_table;
static EFI_BOOT_SERVICES_PREFIX native_boot;
static EFI_RUNTIME_SERVICES native_runtime;
static EFI_SIMPLE_TEXT_INPUT_PROTOCOL native_input;
static EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL native_output;
static EFI_SIMPLE_TEXT_OUTPUT_MODE native_mode;
static EFI_LOADED_IMAGE_PROTOCOL native_image;
static EFI_BLOCK_IO_PROTOCOL native_block;
static EFI_BLOCK_IO_MEDIA native_media;
typedef struct {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL protocol;
    FATFS fs;
    UINT64 first, sectors;
    int mounted;
    char prefix[8];
} NativeVolume;
static NativeVolume native_volumes[16];
typedef struct {
    EFI_FILE_PROTOCOL protocol;
    NativeVolume *volume;
    FIL file;
    DIR directory;
    UINT64 position;
    UINT64 mode;
    int used, is_directory;
    char path[300];
} NativeFile;
static NativeFile native_files[64];
typedef struct NativePool { UINTN size; struct NativePool *next; UINT64 free, reserved; } NativePool;
static NativePool *native_pool;
static UINTN native_heap_end;

static int native_guid(const EFI_GUID *a, const EFI_GUID *b) {
    return a && b && memory_equal(a, b, 16);
}
static EFI_STATUS native_result(FRESULT result) {
    if (result==FR_OK) return EFI_SUCCESS;
    if (result==FR_NO_FILE || result==FR_NO_PATH) return EFI_NOT_FOUND;
    if (result==FR_WRITE_PROTECTED || result==FR_DENIED || result==FR_LOCKED) return EFI_ACCESS_DENIED;
    if (result==FR_TOO_MANY_OPEN_FILES || result==FR_NOT_ENOUGH_CORE) return EFI_OUT_OF_RESOURCES;
    return EFI_DEVICE_ERROR;
}
static EFI_STATUS native_allocate(UINT32 type, UINTN bytes, void **out) {
    (void)type;
    if (!out || bytes>native_heap_end-(UINTN)native_pool-64) return EFI_OUT_OF_RESOURCES;
    bytes=(bytes+15U)&~15ULL; if (!bytes) bytes=16;
    for (NativePool *block=native_pool; block; block=block->next) if (block->free && block->size>=bytes) {
        if (block->size>bytes+sizeof(NativePool)+16) {
            NativePool *next=(NativePool *)((UINT8 *)(block+1)+bytes);
            next->size=block->size-bytes-sizeof(NativePool); next->next=block->next; next->free=1;
            block->next=next; block->size=bytes;
        }
        block->free=0; *out=block+1; return EFI_SUCCESS;
    }
    *out=0; return EFI_OUT_OF_RESOURCES;
}
static EFI_STATUS native_free(void *pointer) {
    if (!pointer) return EFI_SUCCESS;
    int found=0;
    for (NativePool *block=native_pool; block; block=block->next)
        if (pointer==(void *)(block+1) && !block->free) { block->free=1; found=1; break; }
    if (!found) return EFI_ACCESS_DENIED;
    for (NativePool *block=native_pool; block && block->next;) {
        if (block->free && block->next->free) {
            block->size+=sizeof(NativePool)+block->next->size; block->next=block->next->next;
        } else block=block->next;
    }
    return EFI_SUCCESS;
}
#include "graphics.inc"

static void native_uart_char(char ch) {
    volatile UINT32 *flags=(volatile UINT32 *)0x09000018UL;
    while (*flags&(1U<<5)) __asm__ volatile("yield");
    *(volatile UINT32 *)0x09000000UL=(UINT8)ch;
}
static void native_uart_text(const char *text) { while (*text) native_uart_char(*text++); }
static void native_uart_number(UINTN number) {
    char digits[24]; UINTN used=0;
    do { digits[used++]=(char)('0'+number%10); number/=10; } while (number);
    while (used) native_uart_char(digits[--used]);
}
static EFI_STATUS native_text(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, CHAR16 *text) {
    (void)self;
    while (*text) {
        CHAR16 ch=*text++;
        native_uart_char(ch<128 ? (char)ch : '?');
        native_console_character(ch);
    }
    return EFI_SUCCESS;
}
static EFI_STATUS native_query(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, UINTN mode, UINTN *columns, UINTN *rows) {
    (void)self; if (mode) return EFI_UNSUPPORTED; *columns=80; *rows=25; return EFI_SUCCESS;
}
static EFI_STATUS native_set_mode(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, UINTN mode) { (void)self; return mode ? EFI_UNSUPPORTED : EFI_SUCCESS; }
static EFI_STATUS native_attribute(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, UINTN attribute) {
    static const UINT8 colors[]={0,4,2,6,1,5,3,7}; (void)self;
    native_mode.Attribute=(INT32)attribute;
    native_uart_text("\033["); native_uart_number((attribute&8 ? 90 : 30)+colors[attribute&7]);
    native_uart_char(';'); native_uart_number(40+colors[(attribute>>4)&7]); native_uart_char('m'); return EFI_SUCCESS;
}
static EFI_STATUS native_clear(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self) {
    (void)self; native_uart_text("\033[2J\033[H"); native_mode.CursorColumn=0; native_mode.CursorRow=0;
    native_graphics_clear(); return EFI_SUCCESS;
}
static EFI_STATUS native_cursor(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, UINTN x, UINTN y) {
    (void)self; if (x>=80 || y>=25) return EFI_UNSUPPORTED;
    native_uart_text("\033["); native_uart_number(y+1); native_uart_char(';'); native_uart_number(x+1); native_uart_char('H');
    native_mode.CursorColumn=(INT32)x; native_mode.CursorRow=(INT32)y; return EFI_SUCCESS;
}
static EFI_STATUS native_cursor_enable(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, UINT8 enabled) {
    (void)self; native_mode.CursorVisible=enabled; native_uart_text(enabled ? "\033[?25h" : "\033[?25l"); return EFI_SUCCESS;
}
static int native_uart_get(void) {
    if (*(volatile UINT32 *)0x09000018UL&(1U<<4)) return -1;
    return *(volatile UINT32 *)0x09000000UL&255U;
}
static EFI_STATUS native_key(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *self, EFI_INPUT_KEY *key) {
    (void)self;
    if (native_api->keyboard && native_api->keyboard(&key->ScanCode,&key->UnicodeChar)) return EFI_SUCCESS;
    int ch=native_uart_get(); if (ch<0) return EFI_NOT_READY;
    key->ScanCode=0; key->UnicodeChar=(CHAR16)ch;
    if (ch!=27) return EFI_SUCCESS;
    UINT64 start=timer_count(), wait=timer_frequency()/40U;
    do { ch=native_uart_get(); } while (ch<0 && timer_count()-start<wait);
    key->UnicodeChar=0; key->ScanCode=23;
    if (ch=='[' || ch=='O') {
        do { ch=native_uart_get(); } while (ch<0 && timer_count()-start<wait);
        if (ch=='A') key->ScanCode=1; else if (ch=='B') key->ScanCode=2;
        else if (ch=='C') key->ScanCode=3; else if (ch=='D') key->ScanCode=4;
        else if (ch=='H') key->ScanCode=5; else if (ch=='F') key->ScanCode=6;
        else if (ch=='Q') key->ScanCode=12;
        else if (ch=='3' || ch=='5' || ch=='6') {
            key->ScanCode=ch=='3' ? 8 : (ch=='5' ? 9 : 10);
            do { ch=native_uart_get(); } while (ch<0 && timer_count()-start<wait);
        }
    }
    return EFI_SUCCESS;
}
static EFI_STATUS native_stall(UINTN micros) {
    UINT64 start=timer_count(), delta=timer_frequency()/1000000U*micros;
    while (timer_count()-start<delta) __asm__ volatile("yield"); return EFI_SUCCESS;
}
static EFI_STATUS native_watchdog(UINTN a, UINT64 b, UINTN c, CHAR16 *d) { (void)a;(void)b;(void)c;(void)d;return EFI_SUCCESS; }
static EFI_STATUS native_connect(EFI_HANDLE a, EFI_HANDLE *b, void *c, UINT8 d) { (void)a;(void)b;(void)c;(void)d;return EFI_SUCCESS; }
static void native_reset(EFI_RESET_TYPE type, EFI_STATUS status, UINTN bytes, void *data) {
    (void)status;(void)bytes;(void)data; native_api->flush();
    register UINT64 call __asm__("x0")=type==EfiResetShutdown ? 0x84000008ULL : 0x84000009ULL;
    __asm__ volatile("hvc #0" : "+r"(call) :: "x1","x2","x3","memory");
    for (;;) __asm__ volatile("wfe");
}
DSTATUS disk_initialize(BYTE drive) { return drive<16 && native_volumes[drive].sectors ? (native_api->writable() ? 0 : STA_PROTECT) : STA_NOINIT; }
DSTATUS disk_status(BYTE drive) { return disk_initialize(drive); }
DRESULT fatfs_disk_read(BYTE drive, BYTE *buffer, LBA_t sector, UINT count) {
    if (drive>=16 || !count || sector>=native_volumes[drive].sectors || count>native_volumes[drive].sectors-sector) return RES_PARERR;
    for (UINT i=0;i<count;i++) if (!native_api->read(native_volumes[drive].first+sector+i, buffer+(UINTN)i*512)) return RES_ERROR;
    return RES_OK;
}
DRESULT fatfs_disk_write(BYTE drive, const BYTE *buffer, LBA_t sector, UINT count) {
    if (!native_api->writable()) return RES_WRPRT;
    if (drive>=16 || !count || sector>=native_volumes[drive].sectors || count>native_volumes[drive].sectors-sector) return RES_PARERR;
    for (UINT i=0;i<count;i++) if (!native_api->write(native_volumes[drive].first+sector+i, buffer+(UINTN)i*512)) return RES_ERROR;
    return RES_OK;
}
DRESULT disk_ioctl(BYTE drive, BYTE operation, void *buffer) {
    if (drive>=16 || !native_volumes[drive].sectors) return RES_PARERR;
    if (operation==CTRL_SYNC) return native_api->flush() ? RES_OK : RES_ERROR;
    if (operation==GET_SECTOR_COUNT) { *(LBA_t *)buffer=(LBA_t)native_volumes[drive].sectors; return RES_OK; }
    if (operation==GET_BLOCK_SIZE) { *(DWORD *)buffer=1; return RES_OK; }
    return RES_PARERR;
}
static int native_path(NativeFile *parent, const CHAR16 *name, char path[300], int rename) {
    UINTN used=0;
    if (*name=='\\' || *name=='/') {
        string_copy(path, parent->volume->prefix, 300); used=string_length(path);
    } else {
        string_copy(path, parent->path, 300); used=string_length(path);
        if (rename || !parent->is_directory) while (used && path[used-1]!='/') used--;
        else if (used && path[used-1]!='/') path[used++]='/';
    }
    while (*name) {
        CHAR16 ch=*name++;
        if (ch>126 || ch<32 || ch==':' || used+1>=300) return 0;
        path[used++]=ch=='\\' ? '/' : (char)ch;
    }
    path[used]=0; return 1;
}
static const char *native_basename(const char *path) {
    const char *name=path;
    while (*path) if (*path++=='/') name=path;
    return name;
}
static EFI_STATUS native_file_info(NativeFile *file, FILINFO *entry, UINTN *bytes, void *buffer) {
    const char *name=entry ? entry->fname : native_basename(file->path);
    UINTN chars=string_length(name), need=80+(chars+1)*2;
    if (!buffer || *bytes<need) { *bytes=need; return EFI_BUFFER_TOO_SMALL; }
    EFI_FILE_INFO *info=buffer; memory_zero(info, need); info->Size=need;
    info->FileSize=entry ? entry->fsize : (file->is_directory ? 0 : f_size(&file->file));
    info->PhysicalSize=(info->FileSize+511)&~511ULL;
    info->Attribute=entry ? entry->fattrib : (file->is_directory ? EFI_FILE_DIRECTORY : 0);
    for (UINTN i=0;i<=chars;i++) info->FileName[i]=(CHAR16)(UINT8)name[i];
    *bytes=need; return EFI_SUCCESS;
}
static EFI_STATUS native_close(EFI_FILE_PROTOCOL *protocol) {
    NativeFile *file=(NativeFile *)protocol; if (!file->used) return EFI_ACCESS_DENIED;
    FRESULT result=file->is_directory ? f_closedir(&file->directory) : f_close(&file->file);
    file->used=0; return native_result(result);
}
static EFI_STATUS native_delete(EFI_FILE_PROTOCOL *protocol) {
    NativeFile *file=(NativeFile *)protocol;
    if (!(file->mode&EFI_FILE_MODE_WRITE) || !*native_basename(file->path)) { native_close(protocol); return EFI_ACCESS_DENIED; }
    EFI_STATUS status=native_close(protocol);
    if (status!=EFI_SUCCESS) return status;
    return native_result(f_unlink(file->path));
}
static EFI_STATUS native_read(EFI_FILE_PROTOCOL *protocol, UINTN *bytes, void *buffer) {
    NativeFile *file=(NativeFile *)protocol; if (!file->used || *bytes>0xffffffffU) return EFI_ACCESS_DENIED;
    if (file->is_directory) {
        DIR saved=file->directory; FILINFO info;
        FRESULT result=f_readdir(&file->directory,&info);
        if (result!=FR_OK) return native_result(result);
        if (!info.fname[0]) { *bytes=0; return EFI_SUCCESS; }
        EFI_STATUS status=native_file_info(file,&info,bytes,buffer);
        if (status==EFI_BUFFER_TOO_SMALL) file->directory=saved;
        return status;
    }
    UINT amount=0; FRESULT result=f_read(&file->file,buffer,(UINT)*bytes,&amount);
    *bytes=amount; return native_result(result);
}
static EFI_STATUS native_write(EFI_FILE_PROTOCOL *protocol, UINTN *bytes, void *buffer) {
    NativeFile *file=(NativeFile *)protocol;
    if (!file->used || file->is_directory || !(file->mode&EFI_FILE_MODE_WRITE) || *bytes>0xffffffffU) return EFI_ACCESS_DENIED;
    UINT amount=0; FRESULT result=f_write(&file->file,buffer,(UINT)*bytes,&amount);
    *bytes=amount; return native_result(result);
}
static EFI_STATUS native_position_get(EFI_FILE_PROTOCOL *protocol, UINT64 *position) {
    NativeFile *file=(NativeFile *)protocol; if (file->is_directory) return EFI_UNSUPPORTED;
    *position=f_tell(&file->file); return EFI_SUCCESS;
}
static EFI_STATUS native_position_set(EFI_FILE_PROTOCOL *protocol, UINT64 position) {
    NativeFile *file=(NativeFile *)protocol;
    if (file->is_directory) return position ? EFI_UNSUPPORTED : native_result(f_readdir(&file->directory,0));
    if (position==~0ULL) position=f_size(&file->file);
    if (position>0xffffffffU) return EFI_UNSUPPORTED;
    return native_result(f_lseek(&file->file,(FSIZE_t)position));
}
static EFI_STATUS native_info_get(EFI_FILE_PROTOCOL *protocol, EFI_GUID *guid, UINTN *bytes, void *buffer) {
    NativeFile *file=(NativeFile *)protocol;
    if (native_guid(guid,&gFileInfoGuid)) return native_file_info(file,0,bytes,buffer);
    if (!native_guid(guid,&gFileSystemInfoGuid)) return EFI_UNSUPPORTED;
    char label[12]; DWORD serial, free_clusters; FATFS *fs=0;
    FRESULT result=f_getlabel(file->volume->prefix,label,&serial);
    if (result!=FR_OK) return native_result(result);
    UINTN chars=string_length(label), need=36+(chars+1)*2;
    if (!buffer || *bytes<need) { *bytes=need; return EFI_BUFFER_TOO_SMALL; }
    result=f_getfree(file->volume->prefix,&free_clusters,&fs);
    if (result!=FR_OK) return native_result(result);
    EFI_FILE_SYSTEM_INFO *info=buffer; memory_zero(info,need); info->Size=need;
    info->ReadOnly=(UINT8)!native_api->writable(); info->VolumeSize=file->volume->sectors*512;
    info->FreeSpace=(UINT64)free_clusters*fs->csize*512; info->BlockSize=512;
    for (UINTN i=0;i<=chars;i++) info->VolumeLabel[i]=(CHAR16)(UINT8)label[i];
    *bytes=need; return EFI_SUCCESS;
}
static EFI_STATUS native_flush(EFI_FILE_PROTOCOL *protocol) {
    NativeFile *file=(NativeFile *)protocol;
    FRESULT result=file->is_directory ? FR_OK : f_sync(&file->file);
    return result==FR_OK && native_api->flush() ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}
static EFI_STATUS native_info_set(EFI_FILE_PROTOCOL *protocol, EFI_GUID *guid, UINTN bytes, void *buffer) {
    NativeFile *file=(NativeFile *)protocol; EFI_FILE_INFO *info=buffer;
    if (!native_guid(guid,&gFileInfoGuid) || !(file->mode&EFI_FILE_MODE_WRITE) || bytes<82 || info->Size>bytes || info->Size<82) return EFI_ACCESS_DENIED;
    UINTN name_chars=(info->Size-80)/2, i=0;
    while (i<name_chars && info->FileName[i]) i++;
    if (i==name_chars) return EFI_ACCESS_DENIED;
    char destination[300];
    if (!native_path(file,info->FileName,destination,1)) return EFI_ACCESS_DENIED;
    if (!streq(file->path,destination)) {
        UINT64 position=file->is_directory ? 0 : f_tell(&file->file);
        FRESULT result=file->is_directory ? f_closedir(&file->directory) : f_close(&file->file);
        if (result!=FR_OK) return native_result(result);
        result=f_rename(file->path,destination);
        if (result==FR_OK) string_copy(file->path,destination,sizeof(file->path));
        FRESULT reopened=file->is_directory ? f_opendir(&file->directory,file->path) : f_open(&file->file,file->path,FA_READ|FA_WRITE);
        if (reopened!=FR_OK) return native_result(reopened);
        if (!file->is_directory && f_lseek(&file->file,(FSIZE_t)position)!=FR_OK) return EFI_DEVICE_ERROR;
        if (result!=FR_OK) return native_result(result);
    }
    if (!file->is_directory && info->FileSize!=f_size(&file->file)) {
        if (info->FileSize>0xffffffffU) return EFI_OUT_OF_RESOURCES;
        UINT64 position=f_tell(&file->file);
        if (info->FileSize>f_size(&file->file)) return EFI_UNSUPPORTED;
        if (f_lseek(&file->file,(FSIZE_t)info->FileSize)!=FR_OK || f_truncate(&file->file)!=FR_OK ||
            f_lseek(&file->file,(FSIZE_t)(position<info->FileSize ? position : info->FileSize))!=FR_OK) return EFI_DEVICE_ERROR;
    }
    return native_result(f_chmod(file->path,(BYTE)info->Attribute,AM_RDO|AM_HID|AM_SYS|AM_ARC));
}
static EFI_STATUS native_open(EFI_FILE_PROTOCOL *protocol, EFI_FILE_PROTOCOL **out, CHAR16 *name, UINT64 mode, UINT64 attributes) {
    NativeFile *parent=(NativeFile *)protocol; *out=0;
    if (!parent->is_directory) return EFI_ACCESS_DENIED;
    NativeFile *file=0;
    for (UINTN i=0;i<64;i++) if (!native_files[i].used) { file=&native_files[i]; break; }
    if (!file) return EFI_OUT_OF_RESOURCES;
    memory_zero(file,sizeof(*file)); file->volume=parent->volume; file->mode=mode;
    if (!native_path(parent,name,file->path,0)) return EFI_ACCESS_DENIED;
    FILINFO info; FRESULT status;
    if (!*native_basename(file->path)) { memory_zero(&info,sizeof(info)); info.fattrib=AM_DIR; status=FR_OK; }
    else status=f_stat(file->path,&info);
    if ((status==FR_NO_FILE || status==FR_NO_PATH) && (mode&EFI_FILE_MODE_CREATE)) {
        if (!(mode&EFI_FILE_MODE_WRITE)) return EFI_ACCESS_DENIED;
        if (attributes&EFI_FILE_DIRECTORY) {
            status=f_mkdir(file->path); if (status!=FR_OK) return native_result(status);
            memory_zero(&info,sizeof(info)); info.fattrib=AM_DIR;
        } else { memory_zero(&info,sizeof(info)); status=FR_OK; }
    }
    if (status!=FR_OK) return native_result(status);
    file->is_directory=(info.fattrib&AM_DIR)!=0;
    if (file->is_directory) status=f_opendir(&file->directory,file->path);
    else status=f_open(&file->file,file->path,FA_READ|((mode&EFI_FILE_MODE_WRITE)?FA_WRITE:0)|((mode&EFI_FILE_MODE_CREATE)?FA_OPEN_ALWAYS:0));
    if (status!=FR_OK) return native_result(status);
    file->protocol=(EFI_FILE_PROTOCOL){0x10000,native_open,native_close,native_delete,native_read,native_write,
        native_position_get,native_position_set,native_info_get,native_info_set,native_flush};
    file->used=1; *out=&file->protocol; return EFI_SUCCESS;
}
static EFI_STATUS native_volume_open(void *protocol, EFI_FILE_PROTOCOL **root) {
    NativeVolume *volume=protocol; NativeFile parent; memory_zero(&parent,sizeof(parent));
    parent.volume=volume; parent.is_directory=1; string_copy(parent.path,volume->prefix,sizeof(parent.path));
    static CHAR16 path[]={'\\',0};
    return native_open(&parent.protocol,root,path,EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE,EFI_FILE_DIRECTORY);
}
static EFI_STATUS native_blocks_read(EFI_BLOCK_IO_PROTOCOL *self, UINT32 id, UINT64 lba, UINTN bytes, void *buffer) {
    (void)self;(void)id;
    if (bytes%512 || lba>=native_api->sector_count || bytes/512>native_api->sector_count-lba) return EFI_ACCESS_DENIED;
    for (UINTN i=0;i<bytes/512;i++) if (!native_api->read(lba+i,(UINT8 *)buffer+i*512)) return EFI_DEVICE_ERROR;
    return EFI_SUCCESS;
}
static EFI_STATUS native_blocks_write(EFI_BLOCK_IO_PROTOCOL *self, UINT32 id, UINT64 lba, UINTN bytes, const void *buffer) {
    (void)self;(void)id;
    if (!native_api->writable() || bytes%512 || lba>=native_api->sector_count || bytes/512>native_api->sector_count-lba) return EFI_ACCESS_DENIED;
    for (UINTN i=0;i<bytes/512;i++) if (!native_api->write(lba+i,(const UINT8 *)buffer+i*512)) return EFI_DEVICE_ERROR;
    return EFI_SUCCESS;
}
static EFI_STATUS native_blocks_flush(EFI_BLOCK_IO_PROTOCOL *self) { (void)self; return native_api->flush() ? EFI_SUCCESS : EFI_DEVICE_ERROR; }
static EFI_STATUS native_handle(EFI_HANDLE handle, EFI_GUID *guid, void **out) {
    static EFI_GUID loaded={0x5b1b31a1,0x9562,0x11d2,{0x8e,0x3f,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
    static EFI_GUID simple={0x964e5b22,0x6459,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
    *out=0;
    if (handle==&native_image && native_guid(guid,&loaded)) *out=&native_image;
    else if (handle==&native_block && native_guid(guid,&gBlockIoProtocolGuid)) *out=&native_block;
    else if (native_guid(guid,&simple)) for (UINTN i=0;i<16;i++)
        if (handle==&native_volumes[i] && native_volumes[i].mounted) *out=&native_volumes[i].protocol;
    return *out ? EFI_SUCCESS : EFI_NOT_FOUND;
}
static EFI_STATUS native_locate_handles(UINT32 type, EFI_GUID *guid, void *registration, UINTN *count, EFI_HANDLE **handles) {
    (void)registration; *count=0; *handles=0;
    static EFI_GUID simple={0x964e5b22,0x6459,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
    int blocks=native_guid(guid,&gBlockIoProtocolGuid);
    if (type && !blocks && !native_guid(guid,&simple)) return EFI_NOT_FOUND;
    if (native_allocate(2,17*sizeof(EFI_HANDLE),(void **)handles)!=EFI_SUCCESS) return EFI_OUT_OF_RESOURCES;
    if (blocks) { (*handles)[(*count)++]=&native_block; return EFI_SUCCESS; }
    for (UINTN i=0;i<16;i++) if (native_volumes[i].mounted) (*handles)[(*count)++]=&native_volumes[i];
    return EFI_SUCCESS;
}
static EFI_STATUS native_locate_protocol(EFI_GUID *guid, void *registration, void **out) {
    static EFI_GUID graphics={0x9042a9de,0x23dc,0x4a38,{0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}};
    (void)registration; *out=0;
    if (native_api->framebuffer && native_guid(guid,&graphics)) *out=&native_gop;
    return *out ? EFI_SUCCESS : EFI_NOT_FOUND;
}
static int native_initialize(const NativeApi *api) {
    if (!api || api->magic!=NATIVE_API_MAGIC || api->version!=NATIVE_API_VERSION || api->ram_end<0x50000000UL) return 0;
    if (api->framebuffer && (api->width!=640 || api->height!=480)) return 0;
    native_api=api;
#ifdef TINYGPT_FIRMWARE
    native_pool=(NativePool *)0x40900000UL; native_heap_end=0x41000000UL;
#else
    native_pool=(NativePool *)0x45800000UL; native_heap_end=(UINTN)api->ram_end;
#endif
    native_pool->size=native_heap_end-(UINTN)(native_pool+1); native_pool->next=0; native_pool->free=1;
    native_mode=(EFI_SIMPLE_TEXT_OUTPUT_MODE){1,0,7,0,0,1};
    native_graphics_initialize();
    native_output.OutputString=native_text; native_output.QueryMode=native_query;
    native_output.SetMode=native_set_mode; native_output.SetAttribute=native_attribute;
    native_output.ClearScreen=native_clear; native_output.SetCursorPosition=native_cursor;
    native_output.EnableCursor=native_cursor_enable; native_output.Mode=&native_mode;
    native_input.ReadKeyStroke=native_key;
    native_boot.AllocatePool=native_allocate; native_boot.FreePool=native_free;
    native_boot.HandleProtocol=native_handle; native_boot.LocateHandleBuffer=native_locate_handles;
    native_boot.LocateProtocol=native_locate_protocol; native_boot.ConnectController=native_connect;
    native_boot.Stall=native_stall; native_boot.SetWatchdogTimer=native_watchdog;
    native_runtime.ResetSystem=native_reset;
    static CHAR16 vendor[]={'T','i','n','y','G','P','T',' ','B','I','O','S',0};
    native_table.FirmwareVendor=vendor; native_table.ConIn=&native_input; native_table.ConOut=&native_output;
    native_table.StdErr=&native_output; native_table.RuntimeServices=&native_runtime; native_table.BootServices=&native_boot;
    native_media.MediaId=1; native_media.MediaPresent=(UINT8)(api->sector_count!=0); native_media.ReadOnly=(UINT8)!api->writable();
    native_media.BlockSize=512; native_media.IoAlign=1; native_media.LastBlock=api->sector_count ? api->sector_count-1 : 0;
    native_block.Revision=0x10000; native_block.Media=&native_media; native_block.ReadBlocks=native_blocks_read;
    native_block.WriteBlocks=native_blocks_write; native_block.FlushBlocks=native_blocks_flush;
    /* Reuse the application's strict mirrored GPT validation before exposing FAT volumes. */
    gST=&native_table;
    PARTITION_DISK disk;
    if (partition_disk_open(&disk)) {
        for (UINTN i=0;i<16 && i<disk.entryCount;i++) {
            UINT8 *entry=disk.entries+i*disk.entrySize;
            if (partition_entry_empty(entry)) continue;
            NativeVolume *volume=&native_volumes[i];
            volume->first=partition_read_u64(entry+32); volume->sectors=partition_read_u64(entry+40)-volume->first+1;
            if (volume->sectors>0xffffffffU) { volume->sectors=0; continue; }
            volume->prefix[0]='p'; UINTN n=1;
            if (i>=10) volume->prefix[n++]='1';
            volume->prefix[n++]=(char)('0'+i%10); volume->prefix[n++]=':'; volume->prefix[n]=0;
            volume->protocol.Revision=0x10000; volume->protocol.OpenVolume=native_volume_open;
            volume->mounted=f_mount(&volume->fs,volume->prefix,1)==FR_OK;
        }
        partition_disk_close(&disk);
    }
    native_image.SystemTable=&native_table; native_image.DeviceHandle=&native_volumes[0];
    return 1;
}
