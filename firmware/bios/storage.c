/* Bounded, read-only GPT and FAT16/FAT32 inspection. Short (8.3) names only. */
#include "bios.h"
Partition partitions[BIOS_MAX_PARTITIONS];
uint32_t partition_count;
static uint8_t entries[128*128];
static uint32_t crc32(const uint8_t *data, size_t bytes) {
    uint32_t crc=~0U;
    while (bytes--) {
        crc^=*data++;
        for (unsigned bit=0; bit<8; bit++) crc=(crc>>1)^((0U-(crc&1U))&0xedb88320U);
    }
    return ~crc;
}
int partitions_scan(void) {
    uint8_t header[512];
    partition_count=0;
    if (disk_sectors()<68 || !disk_read(1, header)) return 0;
    const char *signature="EFI PART";
    for (unsigned i=0; i<8; i++) if (header[i]!=(uint8_t)signature[i]) return 0;
    uint32_t size=le32(header+12), stored=le32(header+16);
    if (le32(header+8)!=0x10000 || size<92 || size>512 || le32(header+20) ||
        le64(header+24)!=1 || le64(header+32)!=disk_sectors()-1) return 0;
    memset(header+16, 0, 4);
    if (crc32(header, size)!=stored) return 0;
    uint64_t first=le64(header+40), last=le64(header+48), table=le64(header+72);
    uint32_t count=le32(header+80), stride=le32(header+84);
    if (!count || count>128 || stride!=128 || first<34 || first>last || last>=disk_sectors()-33 ||
        table<2 || table>=first || (count*128U+511U)/512U>first-table) return 0;
    for (uint32_t i=0; i<(count*128U+511U)/512U; i++)
        if (!disk_read(table+i, entries+i*512U)) return 0;
    if (crc32(entries, count*128U)!=le32(header+88)) return 0;
    for (uint32_t i=0; i<count; i++) {
        uint8_t *entry=entries+i*128U;
        unsigned present=0;
        for (unsigned j=0; j<16; j++) present|=entry[j];
        if (!present) continue;
        uint64_t start=le64(entry+32), end=le64(entry+40);
        if (start<first || end<start || end>last) goto bad;
        for (uint32_t j=0; j<partition_count; j++)
            if (start<partitions[j].first+partitions[j].sectors && end>=partitions[j].first) goto bad;
        Partition *p=&partitions[partition_count++];
        p->number=i+1; p->first=start; p->sectors=end-start+1;
        memset(p->name, 0, sizeof(p->name));
        for (unsigned j=0; j<36; j++) {
            uint16_t ch=le16(entry+56+j*2U);
            if (!ch) break;
            p->name[j]=(ch>=32 && ch<=126) ? (char)ch : '?';
        }
    }
    return 1;
bad:
    partition_count=0;
    return 0;
}
static int volume_read(Fat *fat, uint32_t sector, uint8_t out[512]) {
    return sector<fat->partition.sectors && disk_read(fat->partition.first+sector, out);
}
int fat_mount(const Partition *partition, Fat *fat) {
    uint8_t boot[512];
    memset(fat, 0, sizeof(*fat));
    if (!disk_read(partition->first, boot) || le16(boot+510)!=0xaa55 || le16(boot+11)!=512) return 0;
    fat->partition=*partition;
    fat->sectors_per_cluster=boot[13]; fat->reserved=le16(boot+14); fat->fats=boot[16];
    fat->root_entries=le16(boot+17);
    uint32_t total=le16(boot+19); if (!total) total=le32(boot+32);
    fat->fat_sectors=le16(boot+22); if (!fat->fat_sectors) fat->fat_sectors=le32(boot+36);
    fat->root_sectors=(fat->root_entries*32U+511U)/512U;
    if (!fat->sectors_per_cluster || fat->sectors_per_cluster>128 ||
        (fat->sectors_per_cluster&(fat->sectors_per_cluster-1U)) || !fat->reserved ||
        !fat->fats || fat->fats>2 || !fat->fat_sectors || !total || total>partition->sectors) return 0;
    uint64_t data=(uint64_t)fat->reserved+(uint64_t)fat->fats*fat->fat_sectors+fat->root_sectors;
    if (data>=total) return 0;
    fat->data_sector=(uint32_t)data;
    fat->clusters=(total-fat->data_sector)/fat->sectors_per_cluster;
    if (fat->clusters<4085 || fat->clusters>=0x0ffffff5U) return 0; /* FAT12 is not supported. */
    fat->bits=fat->clusters<65525 ? 16 : 32;
    if ((uint64_t)(fat->clusters+2)*(fat->bits/8U)>(uint64_t)fat->fat_sectors*512U) return 0;
    if (fat->bits==16) {
        if (!fat->root_entries || !le16(boot+22)) return 0;
    } else {
        if (fat->root_entries || le16(boot+22) || le16(boot+42) || (le16(boot+40)&0x80)) return 0;
        fat->root_cluster=le32(boot+44);
        if (fat->root_cluster<2 || fat->root_cluster>=fat->clusters+2) return 0;
    }
    fat->partition.sectors=total;
    return 1;
}
static int cluster_valid(Fat *fat, uint32_t cluster) { return cluster>=2 && cluster<fat->clusters+2; }
static int next_cluster(Fat *fat, uint32_t cluster, uint32_t *next) {
    uint8_t sector[512];
    if (!cluster_valid(fat, cluster)) return 0;
    uint32_t offset=cluster*(fat->bits/8U);
    if (!volume_read(fat, fat->reserved+offset/512U, sector)) return 0;
    uint32_t value=fat->bits==16 ? le16(sector+offset%512U) : le32(sector+offset%512U)&0x0fffffffU;
    if (value>=(fat->bits==16 ? 0xfff8U : 0x0ffffff8U)) { *next=0; return 1; }
    if (!cluster_valid(fat, value)) return 0;
    *next=value;
    return 1;
}
static void file_decode(Fat *fat, const uint8_t *entry, File *file) {
    memcpy(file->name, entry, 11);
    file->attributes=entry[11];
    file->cluster=le16(entry+26);
    if (fat->bits==32) file->cluster|=(uint32_t)le16(entry+20)<<16;
    file->size=le32(entry+28);
}
static void print_file(const File *file) {
    for (unsigned i=0; i<8 && file->name[i]!=' '; i++)
        putc_bios(file->name[i]>=32 && file->name[i]<=126 ? (char)file->name[i] : '?');
    if (file->name[8]!=' ') {
        putc_bios('.');
        for (unsigned i=8; i<11 && file->name[i]!=' '; i++)
            putc_bios(file->name[i]>=32 && file->name[i]<=126 ? (char)file->name[i] : '?');
    }
    if (file->attributes&16) puts_bios("/");
    else { puts_bios("  "); number(file->size); puts_bios(" bytes"); }
    puts_bios("\n");
}
/* Return 1 for a matching file, 2 for completed listing, or 0 for missing/error. */
static int directory(Fat *fat, uint32_t cluster, const uint8_t *name, File *found) {
    uint8_t sector[512];
    int fixed=fat->bits==16 && !cluster;
    uint32_t walked=0;
    do {
        if (++walked>256 || (!fixed && !cluster_valid(fat, cluster))) return 0;
        uint32_t first=fixed ? fat->reserved+fat->fats*fat->fat_sectors :
            fat->data_sector+(cluster-2U)*fat->sectors_per_cluster;
        uint32_t sectors=fixed ? fat->root_sectors : fat->sectors_per_cluster;
        for (uint32_t i=0; i<sectors; i++) {
            if (!volume_read(fat, first+i, sector)) return 0;
            for (unsigned j=0; j<512; j+=32) {
                const uint8_t *e=sector+j;
                if (!e[0]) return name ? 0 : 2;
                if (e[0]==0xe5 || e[0]=='.' || (e[11]&8)) continue;
                File file; file_decode(fat, e, &file);
                if (!name) print_file(&file);
                else {
                    unsigned different=0;
                    for (unsigned k=0; k<11; k++) different|=e[k]^name[k];
                    if (!different) { *found=file; return 1; }
                }
            }
        }
        if (fixed) break;
        if (!next_cluster(fat, cluster, &cluster)) return 0;
    } while (cluster);
    return name ? 0 : 2;
}
int fat_find(Fat *fat, const char *path, File *file) {
    uint32_t cluster=fat->root_cluster;
    memset(file, 0, sizeof(*file)); file->cluster=cluster; file->attributes=16;
    unsigned depth=0;
    if (*path=='/') path++;
    if (!*path) return 1;
    while (*path) {
        uint8_t name[11]; memset(name, ' ', sizeof(name));
        unsigned base=0, extension=0, dot=0;
        if (++depth>16) return 0;
        while (*path && *path!='/') {
            char ch=*path++;
            if (ch=='.') { if (dot++ || !base) return 0; continue; }
            if (ch>='a' && ch<='z') ch-=32;
            if (!((ch>='A' && ch<='Z') || (ch>='0' && ch<='9') || ch=='_' || ch=='-' || ch=='~')) return 0;
            if (!dot) { if (base==8) return 0; name[base++]=(uint8_t)ch; }
            else { if (extension==3) return 0; name[8+extension++]=(uint8_t)ch; }
        }
        if (!base || (dot && !extension) || !directory(fat, cluster, name, file)) return 0;
        if (!*path) return 1;
        if (!(file->attributes&16) || !cluster_valid(fat, file->cluster)) return 0;
        cluster=file->cluster;
        path++;
    }
    return 1;
}
int fat_list(Fat *fat, const File *directory_file) {
    return (directory_file->attributes&16) && directory(fat, directory_file->cluster, 0, 0)==2;
}
int fat_read(Fat *fat, const File *file, uint8_t *data, uint32_t capacity) {
    if ((file->attributes&16) || file->size>capacity) return 0;
    uint32_t left=file->size, cluster=file->cluster;
    uint8_t sector[512];
    while (left) {
        if (!cluster_valid(fat, cluster)) return 0;
        uint32_t first=fat->data_sector+(cluster-2)*fat->sectors_per_cluster;
        for (uint32_t i=0; i<fat->sectors_per_cluster && left; i++) {
            if (!volume_read(fat, first+i, sector)) return 0;
            uint32_t amount=left>512 ? 512 : left;
            memcpy(data, sector, amount); data+=amount; left-=amount;
        }
        if (left && !next_cluster(fat, cluster, &cluster)) return 0;
    }
    return 1;
}
