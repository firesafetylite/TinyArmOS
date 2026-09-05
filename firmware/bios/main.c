/* Standalone QEMU virt firmware bring-up. The existing EFI OS is NOT supported yet. */
#include "bios.h"
void *memcpy(void *destination, const void *source, size_t bytes) {
    uint8_t *out=destination; const uint8_t *in=source;
    while (bytes--) *out++=*in++;
    return destination;
}
void *memset(void *destination, int value, size_t bytes) {
    uint8_t *out=destination;
    while (bytes--) *out++=(uint8_t)value;
    return destination;
}
int equal(const char *left, const char *right) {
    while (*left && *left==*right) { left++; right++; }
    return *left==*right;
}
uint64_t ticks(void) { uint64_t n; __asm__ volatile("isb; mrs %0, cntpct_el0" : "=r"(n)); return n; }
uint64_t frequency(void) { uint64_t n; __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(n)); return n; }
uint64_t exception_level(void) { uint64_t n; __asm__ volatile("mrs %0, CurrentEL" : "=r"(n)); return n>>2; }
void putc_bios(char ch) {
    if (ch=='\n') putc_bios('\r');
    while (mmio_read(BIOS_UART+0x18)&(1U<<5)) __asm__ volatile("yield");
    mmio_write(BIOS_UART, (uint8_t)ch);
}
void puts_bios(const char *text) { while (*text) putc_bios(*text++); }
void number(uint64_t n) {
    char digits[21]; unsigned used=0;
    do { digits[used++]=(char)('0'+n%10); n/=10; } while (n);
    while (used) putc_bios(digits[--used]);
}
void hex(uint64_t n) {
    puts_bios("0x");
    for (int bit=60; bit>=0; bit-=4) putc_bios("0123456789abcdef"[(n>>bit)&15]);
}
void bios_exception(uint64_t level, uint64_t syndrome, uint64_t address) {
    puts_bios("\nFirmware exception: EL"); number(level>>2);
    puts_bios(" ESR="); hex(syndrome); puts_bios(" PC="); hex(address);
    puts_bios("\nHalted; disk writes are not implemented.\n");
    for (;;) __asm__ volatile("wfe");
}
static void power(int reboot) {
    register uint64_t x0 __asm__("x0")=reboot ? 0x84000009UL : 0x84000008UL;
    __asm__ volatile("hvc #0" : "+r"(x0) :: "x1", "x2", "x3", "memory");
    puts_bios("Power request returned; use the emulator's power controls.\n");
}
static int line_read(char *text, size_t capacity) {
    size_t used=0; int overflow=0;
    for (;;) {
        while (mmio_read(BIOS_UART+0x18)&(1U<<4)) __asm__ volatile("yield");
        char ch=(char)(mmio_read(BIOS_UART)&255U);
        if (ch==27) { text[0]=0; puts_bios("\n"); return 2; }
        if (ch=='\r' || ch=='\n') { text[used]=0; puts_bios("\n"); return !overflow; }
        if (ch==8 || ch==127) { if (used) { used--; puts_bios("\b \b"); } }
        else if (ch>=32 && ch<=126) {
            if (used+1<capacity) { text[used++]=ch; putc_bios(ch); }
            else overflow=1;
        }
    }
}
static char *token(char **rest) {
    char *start=*rest;
    while (*start==' ') start++;
    char *end=start;
    while (*end && *end!=' ') end++;
    if (*end) *end++=0;
    *rest=end;
    return start;
}
static void help(void) {
    puts_bios("\n=== TinyGPT BIOS - native firmware bring-up ===\n"
              "No EDK II, UEFI services, or disk-resident recovery application.\n"
              "Experimental: serial console, one VirtIO-MMIO disk, read-only FAT.\n"
              "  help / Esc            this recovery menu\n"
              "  info                  firmware and hardware information\n"
              "  partitions            validate and list GPT partitions\n"
              "  ls N [PATH]           list a FAT directory (8.3 names)\n"
              "  cat N PATH            read a text file (maximum 8 MiB)\n"
              "  boot N PATH           load a native AArch64 ELF64 payload\n"
              "  reboot / shutdown     emulator power controls\n"
              "The current TinyGPT EFI system cannot boot here yet.\n"
              "Repair, disk writes, account security, graphics and PCI are not implemented.\n");
}
static void info(void) {
    puts_bios("TinyGPT standalone firmware v0 (not UEFI)\nCPU: AArch64 EL"); number(exception_level());
    puts_bios("\nConsole: PL011 UART\nDisk sectors: "); number(disk_sectors());
    puts_bios("\nTimer Hz: "); number(frequency()); puts_bios("\n");
}
static int scan(void) {
    if (!disk_sectors()) { puts_bios("No supported disk. Attach exactly one modern VirtIO-MMIO block device.\n"); return 0; }
    if (!partitions_scan()) { puts_bios("Invalid or unreadable primary GPT; refusing filesystem access.\n"); return 0; }
    return 1;
}
static Partition *partition_find(const char *text) {
    uint32_t number_value=0;
    if (!*text) return 0;
    while (*text) {
        if (*text<'0' || *text>'9' || number_value>128) return 0;
        number_value=number_value*10U+(unsigned)(*text++-'0');
    }
    for (uint32_t i=0; i<partition_count; i++) if (partitions[i].number==number_value) return &partitions[i];
    return 0;
}
static void boot_native(const uint8_t *image, size_t bytes) {
    Executable executable;
    if (!executable_parse(image, bytes, &executable)) {
        puts_bios("Boot refused: requires bounded native AArch64 ELF64, not an EFI/PE image.\n"); return;
    }
    /* No relocation, MMU mappings, or UEFI table. Entry receives a versioned,
       read-only description in x0; this is a development ABI, not a stable OS ABI. */
    static const uint64_t boot_info[]={0x534f494254504754ULL, 1, BIOS_UART, 0x40000000UL, 128U*1024U*1024U};
    for (uint32_t i=0; i<executable.count; i++) {
        Segment *segment=&executable.segments[i];
        memcpy((void *)(uintptr_t)segment->destination, image+segment->offset, (size_t)segment->files);
        memset((void *)(uintptr_t)(segment->destination+segment->files), 0, (size_t)(segment->memory-segment->files));
    }
    __asm__ volatile("dsb sy; ic iallu; dsb sy; isb" ::: "memory");
    puts_bios("Starting native payload at "); hex(executable.entry); puts_bios("\n");
    void (*entry)(const uint64_t *)=(void (*)(const uint64_t *))(uintptr_t)executable.entry;
    entry(boot_info);
    puts_bios("Native payload returned to firmware.\n");
}
static void command(char *line) {
    char *rest=line, *verb=token(&rest);
    if (!*verb) return;
    if (equal(verb, "help")) { help(); return; }
    if (equal(verb, "info")) { info(); return; }
    if (equal(verb, "shutdown")) { power(0); return; }
    if (equal(verb, "reboot")) { power(1); return; }
    if (equal(verb, "partitions")) {
        if (!scan()) return;
        for (uint32_t i=0; i<partition_count; i++) {
            Partition *p=&partitions[i]; number(p->number); puts_bios("  "); puts_bios(p->name);
            puts_bios("  LBA "); number(p->first); puts_bios("  sectors "); number(p->sectors); puts_bios("\n");
        }
        return;
    }
    if (!equal(verb, "ls") && !equal(verb, "cat") && !equal(verb, "boot")) {
        puts_bios("Unknown command. Type help; Esc opens the firmware menu.\n"); return;
    }
    if (!scan()) return;
    char *number_text=token(&rest), *path=token(&rest);
    if (*token(&rest) || (!*path && !equal(verb, "ls"))) { puts_bios("Expected N PATH.\n"); return; }
    Partition *partition=partition_find(number_text);
    Fat fat; File file;
    if (!partition || !fat_mount(partition, &fat)) { puts_bios("Partition unavailable or unsupported FAT layout.\n"); return; }
    if (!fat_find(&fat, path, &file)) { puts_bios("Path unavailable; use short (8.3) names, not . or ...\n"); return; }
    if (equal(verb, "ls")) {
        if (!fat_list(&fat, &file)) puts_bios("Directory is unreadable, invalid, or exceeds the scan limit.\n");
        return;
    }
    uint8_t *data=(uint8_t *)BIOS_STAGE;
    if (!fat_read(&fat, &file, data, BIOS_FILE_LIMIT)) { puts_bios("File unreadable or exceeds the 8 MiB limit.\n"); return; }
    if (equal(verb, "boot")) boot_native(data, file.size);
    else {
        for (uint32_t i=0; i<file.size; i++) {
            uint8_t ch=data[i];
            putc_bios((ch>=32 && ch<=126) || ch=='\n' || ch=='\r' || ch=='\t' ? (char)ch : '.');
        }
        puts_bios("\n");
    }
}
void bios_main(void) {
    mmio_write(BIOS_UART+0x30, 0);
    mmio_write(BIOS_UART+0x24, 13); mmio_write(BIOS_UART+0x28, 1);
    mmio_write(BIOS_UART+0x2c, 0x70); mmio_write(BIOS_UART+0x30, 0x301);
    puts_bios("\nTinyGPT native reset reached.\n");
    disk_init();
    help();
    char line[192];
    for (;;) {
        puts_bios("bios> ");
        int result=line_read(line, sizeof(line));
        if (result==2) help();
        else if (!result) puts_bios("Input too long; command rejected.\n");
        else command(line);
    }
}
