/* Validate every segment before copying anything into the native OS load window. */
#include "bios.h"
static int span(uint64_t offset, uint64_t bytes, size_t size) {
    return offset<=size && bytes<=size-offset;
}
int executable_parse(const uint8_t *image, size_t size, Executable *executable) {
    memset(executable, 0, sizeof(*executable));
    if (size<64 || size>BIOS_FILE_LIMIT || image[0]!=0x7f || image[1]!='E' || image[2]!='L' || image[3]!='F' ||
        image[4]!=2 || image[5]!=1 || image[6]!=1 || le16(image+16)!=2 || le16(image+18)!=183 ||
        le32(image+20)!=1 || le16(image+52)!=64 || le16(image+54)!=56) return 0;
    uint64_t table=le64(image+32), entry=le64(image+24);
    uint16_t count=le16(image+56);
    if (!count || count>32 || !span(table, (uint64_t)count*56U, size) || (entry&3)) return 0;
    int executable_entry=0;
    for (uint16_t i=0; i<count; i++) {
        const uint8_t *p=image+table+i*56U;
        uint32_t type=le32(p);
        if (type==2 || type==3) return 0; /* No dynamic linking or interpreter. */
        if (type!=1) continue;
        Segment segment={le64(p+24), le64(p+8), le64(p+32), le64(p+40), le32(p+4)};
        uint64_t align=le64(p+48);
        if (!segment.memory) { if (segment.files) return 0; continue; }
        if (executable->count==16 || le64(p+16)!=segment.destination ||
            segment.destination<BIOS_LOAD_MIN || segment.destination>=BIOS_LOAD_MAX ||
            segment.memory>BIOS_LOAD_MAX-segment.destination || segment.files>segment.memory ||
            !span(segment.offset, segment.files, size) ||
            (align>1 && ((align&(align-1U)) || (segment.destination%align)!=(segment.offset%align)))) return 0;
        for (uint32_t j=0; j<executable->count; j++) {
            Segment *previous=&executable->segments[j];
            if (segment.destination<previous->destination+previous->memory &&
                previous->destination<segment.destination+segment.memory) return 0;
        }
        executable->segments[executable->count++]=segment;
        if ((segment.flags&1U) && entry>=segment.destination && entry-segment.destination<segment.files)
            executable_entry=1;
    }
    if (!executable_entry) return 0;
    executable->entry=entry;
    return 1;
}
