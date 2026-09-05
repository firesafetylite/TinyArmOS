/* QEMU virt peripherals owned by TinyGPT: RAMFB via fw_cfg and VirtIO keyboard. */
#include "../bios/bios.h"
#include "key_repeat.h"
static uint16_t be16(uint16_t n) { return (uint16_t)((n>>8)|(n<<8)); }
static uint32_t be32(uint32_t n) { return __builtin_bswap32(n); }
static uint64_t be64(uint64_t n) { return __builtin_bswap64(n); }
static uint8_t config_byte(void) { return *(volatile uint8_t *)0x09020000UL; }
static void config_select(uint16_t selector) { *(volatile uint16_t *)0x09020008UL=be16(selector); }
uint32_t *board_framebuffer(void) {
    config_select(0);
    if (config_byte()!='Q' || config_byte()!='E' || config_byte()!='M' || config_byte()!='U') return 0;
    config_select(0x19);
    uint32_t count=0; for (unsigned i=0;i<4;i++) count=(count<<8)|config_byte();
    if (count>256) return 0;
    uint16_t selector=0;
    for (uint32_t i=0;i<count;i++) {
        uint8_t entry[64]; for (unsigned j=0;j<64;j++) entry[j]=config_byte();
        const char *name="etc/ramfb"; unsigned j=0;
        while (name[j] && entry[8+j]==(uint8_t)name[j]) j++;
        if (!name[j] && !entry[8+j]) selector=(uint16_t)((entry[4]<<8)|entry[5]);
    }
    if (!selector) return 0;
    static uint8_t config[28] __attribute__((aligned(16)));
    static volatile struct { uint32_t control,length; uint64_t address; } dma __attribute__((aligned(16)));
    uint64_t address=be64(0x40400000UL); memcpy(config,&address,8);
    const uint32_t fields[]={be32(0x34325258),0,be32(640),be32(480),be32(640*4)};
    memcpy(config+8,fields,sizeof(fields));
    dma.control=be32(((uint32_t)selector<<16)|8U|16U); dma.length=be32(sizeof(config)); dma.address=be64((uintptr_t)config);
    barrier(); *(volatile uint64_t *)0x09020010UL=be64((uintptr_t)&dma);
    uint64_t start=ticks();
    while (dma.control) { if (be32(dma.control)&1U || ticks()-start>frequency()) return 0; }
    barrier();
    uint32_t *pixels=(uint32_t *)0x40400000UL;
    for (unsigned i=0;i<640*480;i++) pixels[i]=0;
    return pixels;
}
typedef struct { uint64_t address; uint32_t length; uint16_t flags,next; } InputDescriptor;
typedef struct { uint16_t flags,index,ring[64],event; } InputAvailable;
typedef struct { uint32_t id,length; } InputUsedElement;
typedef struct { uint16_t flags,index; InputUsedElement ring[64]; uint16_t event; } InputUsed;
typedef struct { uint16_t type,code; uint32_t value; } InputEvent;
static InputDescriptor input_descriptors[64] __attribute__((aligned(4096)));
static volatile InputAvailable input_available __attribute__((aligned(4096)));
static volatile InputUsed input_used __attribute__((aligned(4096)));
static volatile InputEvent input_events[64] __attribute__((aligned(4096)));
static uintptr_t input_device;
static uint16_t input_seen;
static uint8_t shift_left,shift_right,control_left,control_right,caps;
static NativeKeyRepeat input_repeat;
static void input_address(uint32_t offset, uintptr_t address) {
    mmio_write(input_device+offset,(uint32_t)address); mmio_write(input_device+offset+4,(uint32_t)(address>>32));
}
int board_keyboard_init(void) {
    input_device=0; input_seen=0;
    input_repeat=(NativeKeyRepeat){0};
    shift_left=shift_right=control_left=control_right=caps=0;
    for (unsigned i=0;i<32;i++) {
        uintptr_t base=0x0a000000UL+i*0x200UL;
        if (mmio_read(base)==0x74726976U && mmio_read(base+4)==2 && mmio_read(base+8)==18) { input_device=base; break; }
    }
    if (!input_device) return 0;
    mmio_write(input_device+0x70,0); mmio_write(input_device+0x70,3);
    mmio_write(input_device+0x14,1);
    if (!(mmio_read(input_device+0x10)&1)) return 0;
    mmio_write(input_device+0x24,1); mmio_write(input_device+0x20,1);
    mmio_write(input_device+0x24,0); mmio_write(input_device+0x20,0);
    mmio_write(input_device+0x70,11);
    if (!(mmio_read(input_device+0x70)&8)) return 0;
    mmio_write(input_device+0x30,0);
    if (mmio_read(input_device+0x34)<64) return 0;
    memset((void *)&input_available,0,sizeof(input_available)); memset((void *)&input_used,0,sizeof(input_used));
    input_available.flags=1;
    for (unsigned i=0;i<64;i++) {
        input_descriptors[i]=(InputDescriptor){(uintptr_t)&input_events[i],sizeof(InputEvent),2,0}; input_available.ring[i]=(uint16_t)i;
    }
    input_available.index=64;
    mmio_write(input_device+0x38,64); input_address(0x80,(uintptr_t)input_descriptors);
    input_address(0x90,(uintptr_t)&input_available); input_address(0xa0,(uintptr_t)&input_used);
    barrier(); mmio_write(input_device+0x44,1); mmio_write(input_device+0x70,15); mmio_write(input_device+0x50,0);
    return 1;
}
static int input_translate(uint16_t code, uint32_t value, uint16_t *scan, uint16_t *character) {
    if (value>2) return 0;
    uint16_t arrow=code==103 ? 1 : code==108 ? 2 : code==106 ? 3 : code==105 ? 4 : 0;
    if (arrow) {
        native_repeat_event(&input_repeat,arrow,value,ticks(),frequency());
        *scan=arrow; *character=0;
        return value!=0;
    }
    if (code==42) { shift_left=value!=0; return 0; } if (code==54) { shift_right=value!=0; return 0; }
    if (code==29) { control_left=value!=0; return 0; } if (code==97) { control_right=value!=0; return 0; }
    if (!value) return 0;
    if (code==58) { if (value==1) caps=!caps; return 0; }
    *scan=0; *character=0; int shifted=shift_left||shift_right;
    if (code==1) *scan=23;
    else if (code==102) *scan=5; else if (code==107) *scan=6;
    else if (code==111) *scan=8; else if (code==104) *scan=9; else if (code==109) *scan=10;
    else if (code>=59 && code<=68) *scan=11+code-59;
    else if (code==87 || code==88) *scan=code-66;
    else if (code==14) *character=8; else if (code==15) *character=9;
    else if (code==28) *character=13; else if (code==57) *character=' ';
    else if (code>=2 && code<=13) *character=(shifted ? "!@#$%^&*()_+" : "1234567890-=")[code-2];
    else if (code>=16 && code<=25) *character="qwertyuiop"[code-16];
    else if (code>=30 && code<=38) *character="asdfghjkl"[code-30];
    else if (code>=44 && code<=50) *character="zxcvbnm"[code-44];
    else if (code==26) *character=shifted ? '{' : '['; else if (code==27) *character=shifted ? '}' : ']';
    else if (code==39) *character=shifted ? ':' : ';'; else if (code==40) *character=shifted ? '"' : '\'';
    else if (code==41) *character=shifted ? '~' : '`'; else if (code==43) *character=shifted ? '|' : '\\';
    else if (code==51) *character=shifted ? '<' : ','; else if (code==52) *character=shifted ? '>' : '.';
    else if (code==53) *character=shifted ? '?' : '/';
    if (*character>='a' && *character<='z') {
        if (control_left||control_right) *character=*character-'a'+1;
        else if (shifted!=caps) *character-=32;
    }
    return *scan || *character;
}
int board_keyboard_read(uint16_t *scan, uint16_t *character) {
    if (!input_device) return 0;
    unsigned budget=64;
    while (input_used.index!=input_seen && budget--) {
        barrier();
        uint32_t id=input_used.ring[input_seen%64].id;
        if (id>=64 || input_used.ring[input_seen%64].length!=8) { input_device=0; return 0; }
        InputEvent event={input_events[id].type,input_events[id].code,input_events[id].value};
        input_seen++; input_available.ring[input_available.index%64]=(uint16_t)id;
        barrier(); input_available.index++; barrier(); mmio_write(input_device+0x50,0);
        if (event.type==1 && input_translate(event.code,event.value,scan,character)) return 1;
    }
    uint32_t interrupts=mmio_read(input_device+0x60); if (interrupts) mmio_write(input_device+0x64,interrupts);
    /* Drain queued releases before synthesizing another scroll event. */
    if (input_used.index!=input_seen) return 0;
    return native_repeat_poll(&input_repeat,ticks(),frequency(),scan,character);
}
