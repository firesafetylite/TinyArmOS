#ifndef TINYGPT_NATIVE_API_H
#define TINYGPT_NATIVE_API_H
#include <stdint.h>
#include <stddef.h>
#define NATIVE_API_MAGIC 0x3256504147505454ULL
#define NATIVE_API_VERSION 3U
/* Owned by TinyGPT firmware; no EDK II code or UEFI implementation is linked. */
typedef struct {
    uint64_t magic, version, ram_end, sector_count;
    int (*read)(uint64_t, uint8_t *);
    int (*write)(uint64_t, const uint8_t *);
    int (*flush)(void);
    int (*writable)(void);
    uint32_t *framebuffer;
    uint32_t width, height;
    uint64_t boot_partition;
    int (*keyboard)(uint16_t *, uint16_t *);
} NativeApi;
#endif
