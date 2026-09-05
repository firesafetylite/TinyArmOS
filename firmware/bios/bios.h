/* TinyGPT standalone firmware: no UEFI or EDK II dependencies. */
#ifndef TINY_BIOS_H
#define TINY_BIOS_H
#include <stdint.h>
#include <stddef.h>
#define BIOS_UART 0x09000000UL
#define BIOS_MAX_PARTITIONS 128U
#define BIOS_FILE_LIMIT (8U * 1024U * 1024U)
#define BIOS_LOAD_MIN 0x41000000UL
#define BIOS_LOAD_MAX 0x44000000UL
#define BIOS_STAGE 0x45000000UL

typedef struct { uint64_t first, sectors; uint32_t number; char name[37]; } Partition;
typedef struct {
    Partition partition;
    uint32_t reserved, fat_sectors, root_sectors, data_sector, clusters, root_cluster;
    uint16_t root_entries;
    uint8_t sectors_per_cluster, fats, bits;
} Fat;
typedef struct { uint8_t name[11], attributes; uint32_t cluster, size; } File;
typedef struct { uint64_t destination, offset, files, memory; uint32_t flags; } Segment;
typedef struct { uint64_t entry; uint32_t count; Segment segments[16]; } Executable;

static inline uint16_t le16(const void *p) { const uint8_t *b=p; return b[0] | ((uint16_t)b[1]<<8); }
static inline uint32_t le32(const void *p) { const uint8_t *b=p; return le16(b) | ((uint32_t)le16(b+2)<<16); }
static inline uint64_t le64(const void *p) { const uint8_t *b=p; return le32(b) | ((uint64_t)le32(b+4)<<32); }
static inline uint32_t mmio_read(uintptr_t p) { return *(volatile uint32_t *)p; }
static inline void mmio_write(uintptr_t p, uint32_t n) { *(volatile uint32_t *)p=n; }
static inline void barrier(void) { __asm__ volatile("dsb sy" ::: "memory"); }
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
int equal(const char *, const char *);
void puts_bios(const char *);
void putc_bios(char);
void number(uint64_t);
void hex(uint64_t);
uint64_t ticks(void);
uint64_t frequency(void);
uint64_t exception_level(void);
int disk_init(void);
uint64_t disk_sectors(void);
int disk_read(uint64_t, uint8_t[512]);
int partitions_scan(void);
extern Partition partitions[BIOS_MAX_PARTITIONS];
extern uint32_t partition_count;
int fat_mount(const Partition *, Fat *);
int fat_find(Fat *, const char *, File *);
int fat_list(Fat *, const File *);
int fat_read(Fat *, const File *, uint8_t *, uint32_t);
int executable_parse(const uint8_t *, size_t, Executable *);
#endif
