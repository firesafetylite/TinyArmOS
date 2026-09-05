/* VirtIO 1.0 MMIO transport. Writes require an explicit native-stack build. */
#include "bios.h"
typedef struct { uint64_t address; uint32_t length; uint16_t flags, next; } Descriptor;
typedef struct { uint16_t flags, index, ring[8], event; } Available;
typedef struct { uint32_t id, length; } UsedElement;
typedef struct { uint16_t flags, index; UsedElement ring[8]; uint16_t event; } Used;
static Descriptor descriptors[8] __attribute__((aligned(4096)));
static volatile Available available __attribute__((aligned(4096)));
static volatile Used used __attribute__((aligned(4096)));
static struct { uint32_t type, reserved; uint64_t sector; } request __attribute__((aligned(16)));
static uint8_t block[512] __attribute__((aligned(16)));
static volatile uint8_t completion;
static uintptr_t device;
static uint64_t capacity;
static uint16_t completed;
static int writable, flush_supported;
static void address_write(uint32_t offset, uintptr_t address) {
    mmio_write(device+offset, (uint32_t)address);
    mmio_write(device+offset+4, (uint32_t)(address>>32));
}
static int fail(void) {
    if (device) mmio_write(device+0x70, 128);
    capacity=0;
    return 0;
}
uint64_t disk_sectors(void) { return capacity; }
int disk_init(void) {
    uint32_t found=0;
    capacity=0; device=0; writable=0; flush_supported=0;
    for (uint32_t i=0; i<32; i++) {
        uintptr_t base=0x0a000000UL + i*0x200UL;
        if (mmio_read(base)==0x74726976U && mmio_read(base+8)==2U) {
            device=base; found++;
        }
    }
    if (found!=1) { device=0; return 0; }
    if (mmio_read(device+4)!=2) return fail();
    mmio_write(device+0x70, 0);
    if (mmio_read(device+0x70)!=0) return fail();
    mmio_write(device+0x70, 1|2);
    mmio_write(device+0x14, 1);
    if (!(mmio_read(device+0x10)&1U)) return fail(); /* VIRTIO_F_VERSION_1 */
    mmio_write(device+0x24, 1); mmio_write(device+0x20, 1);
    mmio_write(device+0x14, 0);
    uint32_t features=mmio_read(device+0x10);
    uint32_t accepted=features&(1U<<5);
#ifdef TINYGPT_WRITABLE
    accepted|=features&(1U<<9);
    flush_supported=(accepted&(1U<<9))!=0;
    writable=!(accepted&(1U<<5)) && flush_supported;
#endif
    mmio_write(device+0x24, 0); mmio_write(device+0x20, accepted);
    mmio_write(device+0x70, 1|2|8);
    if (!(mmio_read(device+0x70)&8U)) return fail();
    mmio_write(device+0x30, 0);
    if (mmio_read(device+0x34)<8 || mmio_read(device+0x44)) return fail();
    memset(descriptors, 0, sizeof(descriptors));
    memset((void *)&available, 0, sizeof(available));
    memset((void *)&used, 0, sizeof(used));
    completed=0;
    available.flags=1; /* no interrupts: synchronous polling only */
    mmio_write(device+0x38, 8);
    address_write(0x80, (uintptr_t)descriptors);
    address_write(0x90, (uintptr_t)&available);
    address_write(0xa0, (uintptr_t)&used);
    barrier();
    mmio_write(device+0x44, 1);
    mmio_write(device+0x70, 1|2|8|4);
    for (uint32_t attempt=0; attempt<4; attempt++) {
        uint32_t generation=mmio_read(device+0xfc);
        uint64_t count=mmio_read(device+0x100);
        count|=(uint64_t)mmio_read(device+0x104)<<32;
        if (generation==mmio_read(device+0xfc)) { capacity=count; break; }
    }
    return capacity!=0;
}
static int transfer(uint32_t type, uint64_t sector, uint8_t data[512]) {
    if (!capacity || sector>=capacity || !frequency()) return 0;
    request.type=type; request.reserved=0; request.sector=sector;
    if (type==1) memcpy(block, data, 512);
    completion=0xff;
    descriptors[0]=(Descriptor){(uintptr_t)&request, sizeof(request), 1, 1};
    descriptors[1]=(Descriptor){(uintptr_t)block, sizeof(block), type==0 ? 3 : 1, 2};
    descriptors[2]=(Descriptor){(uintptr_t)&completion, 1, 2, 0};
    if (type==4) descriptors[0].next=2;
    available.ring[available.index%8]=0;
    barrier(); available.index++; barrier();
    mmio_write(device+0x50, 0);
    uint64_t start=ticks();
    while (used.index==completed) {
        if (ticks()-start>frequency()) return fail();
        __asm__ volatile("yield");
    }
    barrier();
    if (used.index!=(uint16_t)(completed+1U) || used.ring[completed%8].id!=0 ||
        used.ring[completed%8].length>513 || completion!=0) return fail();
    completed++;
    if (type==0) memcpy(data, block, 512);
    uint32_t interrupts=mmio_read(device+0x60);
    if (interrupts) mmio_write(device+0x64, interrupts);
    return 1;
}
int disk_read(uint64_t sector, uint8_t out[512]) { return transfer(0, sector, out); }
int disk_write_sector(uint64_t sector, const uint8_t data[512]) {
    return writable && transfer(1, sector, (uint8_t *)data);
}
int disk_flush(void) { return !writable || (flush_supported && transfer(4, 0, 0)); }
int disk_writable(void) { return writable; }
