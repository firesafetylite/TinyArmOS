/* GPT partition management for the pre-OS environment. */
#define GPT_SECTOR_SIZE 512U
#define GPT_ENTRY_SIZE 128U
#define GPT_ENTRY_LIMIT 128U
#define GPT_ALIGNMENT_SECTORS 2048ULL
#define FAT_MIN_SECTORS 8192ULL

typedef struct EFI_BLOCK_IO_PROTOCOL EFI_BLOCK_IO_PROTOCOL;
typedef struct {
    UINT32 MediaId;
    UINT8 RemovableMedia;
    UINT8 MediaPresent;
    UINT8 LogicalPartition;
    UINT8 ReadOnly;
    UINT8 WriteCaching;
    UINT32 BlockSize;
    UINT32 IoAlign;
    UINT64 LastBlock;
    UINT64 LowestAlignedLba;
    UINT32 LogicalBlocksPerPhysicalBlock;
    UINT32 OptimalTransferLengthGranularity;
} EFI_BLOCK_IO_MEDIA;

struct EFI_BLOCK_IO_PROTOCOL {
    UINT64 Revision;
    EFI_BLOCK_IO_MEDIA *Media;
    EFI_STATUS (EFIAPI *Reset)(EFI_BLOCK_IO_PROTOCOL *, UINT8);
    EFI_STATUS (EFIAPI *ReadBlocks)(EFI_BLOCK_IO_PROTOCOL *, UINT32, UINT64, UINTN, void *);
    EFI_STATUS (EFIAPI *WriteBlocks)(EFI_BLOCK_IO_PROTOCOL *, UINT32, UINT64, UINTN, const void *);
    EFI_STATUS (EFIAPI *FlushBlocks)(EFI_BLOCK_IO_PROTOCOL *);
};

typedef struct {
    EFI_BLOCK_IO_PROTOCOL *io;
    UINT8 primary[GPT_SECTOR_SIZE];
    UINT8 backup[GPT_SECTOR_SIZE];
    UINT8 originalPrimary[GPT_SECTOR_SIZE];
    UINT8 originalBackup[GPT_SECTOR_SIZE];
    UINT8 *entries;
    UINT8 *originalEntries;
    UINT8 *verifyEntries;
    UINTN entryBytes;
    UINT32 entryCount;
    UINT32 entrySize;
    UINT64 primaryEntriesLba;
    UINT64 backupEntriesLba;
} PARTITION_DISK;

static EFI_GUID gBlockIoProtocolGuid = {0x964e5b21, 0x6459, 0x11d2, {0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
/* UTM's stock UEFI only publishes FAT volumes typed as ESP through SimpleFS. */
static EFI_GUID gFatPartitionGuid = {0xc12a7328, 0xf81f, 0x11d2, {0xba,0x4b,0x00,0xa0,0xc9,0x3e,0xc9,0x3b}};
static EFI_GUID gTinyGPTDiskGuid = {0x117a71a5, 0xe844, 0x4e20, {0xa4,0x75,0x74,0x69,0x6e,0x79,0x6f,0x73}};
static EFI_GUID gRecoveryPartitionGuid = {0xe056959d, 0x53d4, 0x4c21, {0x9f,0xf4,0x72,0x65,0x63,0x6f,0x76,0x31}};
static UINTN gPartitionError;

static int partition_bytes_equal(const UINT8 *left, const UINT8 *right, UINTN length) {
    UINTN index;
    for (index = 0; index < length; index++) if (left[index] != right[index]) return 0;
    return 1;
}

static int partition_entry_empty(const UINT8 *entry) {
    UINTN index;
    for (index = 0; index < 16U; index++) if (entry[index]) return 0;
    return 1;
}

static UINT32 partition_read_u32(const UINT8 *data) {
    return (UINT32)data[0] | ((UINT32)data[1] << 8) |
        ((UINT32)data[2] << 16) | ((UINT32)data[3] << 24);
}

static UINT64 partition_read_u64(const UINT8 *data) {
    return (UINT64)partition_read_u32(data) |
        ((UINT64)partition_read_u32(data + 4) << 32);
}

static void partition_write_u16(UINT8 *data, UINT16 value) {
    data[0] = (UINT8)value;
    data[1] = (UINT8)(value >> 8);
}

static void partition_write_u32(UINT8 *data, UINT32 value) {
    data[0] = (UINT8)value;
    data[1] = (UINT8)(value >> 8);
    data[2] = (UINT8)(value >> 16);
    data[3] = (UINT8)(value >> 24);
}

static void partition_write_u64(UINT8 *data, UINT64 value) {
    partition_write_u32(data, (UINT32)value);
    partition_write_u32(data + 4, (UINT32)(value >> 32));
}

static UINT32 partition_crc32(const UINT8 *data, UINTN length) {
    UINT32 crc = 0xffffffffU;
    UINTN index;
    for (index = 0; index < length; index++) {
        UINT32 value = crc ^ data[index];
        UINTN bit;
        for (bit = 0; bit < 8U; bit++)
            value = (value >> 1) ^ (0xedb88320U & (UINT32)(0U - (value & 1U)));
        crc = value;
    }
    return crc ^ 0xffffffffU;
}

static int partition_header_valid(const UINT8 *header, UINT64 expectedLba) {
    UINT32 size;
    UINT32 stored;
    UINT8 copy[GPT_SECTOR_SIZE];
    if (!partition_bytes_equal(header, (const UINT8 *)"EFI PART", 8U)) return 0;
    size = partition_read_u32(header + 12U);
    if (size < 92U || size > GPT_SECTOR_SIZE || partition_read_u64(header + 24U) != expectedLba) return 0;
    memory_copy(copy, header, GPT_SECTOR_SIZE);
    stored = partition_read_u32(copy + 16U);
    partition_write_u32(copy + 16U, 0U);
    return stored == partition_crc32(copy, size);
}

static EFI_STATUS partition_read_blocks(EFI_BLOCK_IO_PROTOCOL *io, UINT64 lba, UINTN bytes, void *buffer) {
    return io->ReadBlocks(io, io->Media->MediaId, lba, bytes, buffer);
}

static EFI_STATUS partition_write_blocks(EFI_BLOCK_IO_PROTOCOL *io, UINT64 lba, UINTN bytes, const void *buffer) {
    return io->WriteBlocks(io, io->Media->MediaId, lba, bytes, buffer);
}

static void partition_disk_close(PARTITION_DISK *disk) {
    if (disk->entries) gST->BootServices->FreePool(disk->entries);
    if (disk->originalEntries) gST->BootServices->FreePool(disk->originalEntries);
    if (disk->verifyEntries) gST->BootServices->FreePool(disk->verifyEntries);
    memory_zero(disk, sizeof(*disk));
}

static int partition_disk_open(PARTITION_DISK *disk) {
    EFI_HANDLE *handles = (EFI_HANDLE *)0;
    UINTN count = 0;
    UINTN index;
    EFI_STATUS status;
    memory_zero(disk, sizeof(*disk));
    status = gST->BootServices->LocateHandleBuffer(2, &gBlockIoProtocolGuid, (void *)0,
        &count, &handles);
    if (status != EFI_SUCCESS || !handles) { gPartitionError = 20U; return 0; }
    for (index = 0; index < count; index++) {
        EFI_BLOCK_IO_PROTOCOL *io = (EFI_BLOCK_IO_PROTOCOL *)0;
        if (gST->BootServices->HandleProtocol(handles[index], &gBlockIoProtocolGuid,
                (void **)&io) != EFI_SUCCESS || !io || !io->Media ||
            !io->Media->MediaPresent || io->Media->LogicalPartition || io->Media->ReadOnly ||
            io->Media->BlockSize != GPT_SECTOR_SIZE || io->Media->IoAlign > 8U ||
            io->Media->LastBlock < 4095U) continue;
        if (partition_read_blocks(io, 1U, GPT_SECTOR_SIZE, disk->primary) != EFI_SUCCESS ||
            !partition_header_valid(disk->primary, 1U) ||
            !partition_bytes_equal(disk->primary + 56U, (const UINT8 *)&gTinyGPTDiskGuid, 16U)) continue;
        if (partition_read_blocks(io, io->Media->LastBlock, GPT_SECTOR_SIZE, disk->backup) != EFI_SUCCESS ||
            !partition_header_valid(disk->backup, io->Media->LastBlock) ||
            !partition_bytes_equal(disk->backup + 56U, (const UINT8 *)&gTinyGPTDiskGuid, 16U)) continue;
        disk->io = io;
        break;
    }
    gST->BootServices->FreePool(handles);
    if (!disk->io) { gPartitionError = 21U; return 0; }
    disk->entryCount = partition_read_u32(disk->primary + 80U);
    disk->entrySize = partition_read_u32(disk->primary + 84U);
    disk->primaryEntriesLba = partition_read_u64(disk->primary + 72U);
    disk->backupEntriesLba = partition_read_u64(disk->backup + 72U);
    if (!disk->entryCount || disk->entryCount > GPT_ENTRY_LIMIT ||
        disk->entrySize != GPT_ENTRY_SIZE ||
        partition_read_u32(disk->backup + 80U) != disk->entryCount ||
        partition_read_u32(disk->backup + 84U) != disk->entrySize ||
        partition_read_u64(disk->primary + 32U) != disk->io->Media->LastBlock ||
        partition_read_u64(disk->backup + 32U) != 1U ||
        partition_read_u64(disk->primary + 40U) != partition_read_u64(disk->backup + 40U) ||
        partition_read_u64(disk->primary + 48U) != partition_read_u64(disk->backup + 48U)) {
        gPartitionError = 22U; return 0;
    }
    disk->entryBytes = (UINTN)disk->entryCount * disk->entrySize;
    if (disk->entryBytes % GPT_SECTOR_SIZE ||
        disk->primaryEntriesLba < 2U || disk->backupEntriesLba < 2U ||
        disk->primaryEntriesLba + disk->entryBytes / GPT_SECTOR_SIZE >
            partition_read_u64(disk->primary + 40U) ||
        partition_read_u64(disk->primary + 48U) >= disk->backupEntriesLba ||
        disk->primaryEntriesLba + disk->entryBytes / GPT_SECTOR_SIZE > disk->io->Media->LastBlock ||
        disk->backupEntriesLba + disk->entryBytes / GPT_SECTOR_SIZE > disk->io->Media->LastBlock ||
        gST->BootServices->AllocatePool(2, disk->entryBytes, (void **)&disk->entries) != EFI_SUCCESS ||
        gST->BootServices->AllocatePool(2, disk->entryBytes, (void **)&disk->originalEntries) != EFI_SUCCESS ||
        gST->BootServices->AllocatePool(2, disk->entryBytes, (void **)&disk->verifyEntries) != EFI_SUCCESS) {
        partition_disk_close(disk); gPartitionError = 23U; return 0;
    }
    if (partition_read_blocks(disk->io, disk->primaryEntriesLba, disk->entryBytes,
            disk->entries) != EFI_SUCCESS ||
        partition_read_blocks(disk->io, disk->backupEntriesLba, disk->entryBytes,
            disk->originalEntries) != EFI_SUCCESS) {
        partition_disk_close(disk); gPartitionError = 24U; return 0;
    }
    if (partition_read_u32(disk->primary + 88U) != partition_crc32(disk->entries, disk->entryBytes) ||
        partition_read_u32(disk->backup + 88U) != partition_crc32(disk->originalEntries, disk->entryBytes) ||
        !partition_bytes_equal(disk->entries, disk->originalEntries, disk->entryBytes)) {
        partition_disk_close(disk); gPartitionError = 25U; return 0;
    }
    if (!partition_bytes_equal(disk->entries + 16U, (const UINT8 *)&gRecoveryPartitionGuid, 16U)) {
        partition_disk_close(disk); gPartitionError = 26U; return 0;
    }
    memory_copy(disk->originalPrimary, disk->primary, GPT_SECTOR_SIZE);
    memory_copy(disk->originalBackup, disk->backup, GPT_SECTOR_SIZE);
    return 1;
}

static void partition_refresh_header_crc(UINT8 *header) {
    UINT32 size = partition_read_u32(header + 12U);
    partition_write_u32(header + 16U, 0U);
    partition_write_u32(header + 16U, partition_crc32(header, size));
}

static int partition_copy_matches(PARTITION_DISK *disk, UINT64 entriesLba,
        UINT64 headerLba, const UINT8 *entries, const UINT8 *header) {
    UINT8 verifyHeader[GPT_SECTOR_SIZE];
    return partition_read_blocks(disk->io, entriesLba, disk->entryBytes,
               disk->verifyEntries) == EFI_SUCCESS &&
        partition_read_blocks(disk->io, headerLba, GPT_SECTOR_SIZE, verifyHeader) == EFI_SUCCESS &&
        partition_bytes_equal(disk->verifyEntries, entries, disk->entryBytes) &&
        partition_bytes_equal(verifyHeader, header, GPT_SECTOR_SIZE);
}

static int partition_restore_backup(PARTITION_DISK *disk) {
    if (partition_write_blocks(disk->io, disk->backupEntriesLba, disk->entryBytes,
            disk->originalEntries) != EFI_SUCCESS ||
        partition_write_blocks(disk->io, disk->io->Media->LastBlock, GPT_SECTOR_SIZE,
            disk->originalBackup) != EFI_SUCCESS ||
        disk->io->FlushBlocks(disk->io) != EFI_SUCCESS) return 0;
    return partition_copy_matches(disk, disk->backupEntriesLba, disk->io->Media->LastBlock,
        disk->originalEntries, disk->originalBackup);
}

static int partition_disk_commit(PARTITION_DISK *disk) {
    UINT32 entriesCrc = partition_crc32(disk->entries, disk->entryBytes);
    partition_write_u32(disk->primary + 88U, entriesCrc);
    partition_write_u32(disk->backup + 88U, entriesCrc);
    partition_refresh_header_crc(disk->primary);
    partition_refresh_header_crc(disk->backup);

    /* Commit and verify the backup mirror before touching the primary mirror. */
    if (partition_write_blocks(disk->io, disk->backupEntriesLba, disk->entryBytes,
            disk->entries) != EFI_SUCCESS ||
        partition_write_blocks(disk->io, disk->io->Media->LastBlock, GPT_SECTOR_SIZE,
            disk->backup) != EFI_SUCCESS || disk->io->FlushBlocks(disk->io) != EFI_SUCCESS ||
        !partition_copy_matches(disk, disk->backupEntriesLba, disk->io->Media->LastBlock,
            disk->entries, disk->backup)) {
        gPartitionError = partition_restore_backup(disk) ? 30U : 31U;
        return 0;
    }

    /* A failure from this point is indeterminate: at least one new mirror is durable. */
    if (partition_write_blocks(disk->io, disk->primaryEntriesLba, disk->entryBytes,
            disk->entries) != EFI_SUCCESS ||
        partition_write_blocks(disk->io, 1U, GPT_SECTOR_SIZE, disk->primary) != EFI_SUCCESS ||
        disk->io->FlushBlocks(disk->io) != EFI_SUCCESS ||
        !partition_copy_matches(disk, disk->primaryEntriesLba, 1U,
            disk->entries, disk->primary)) {
        gPartitionError = 31U;
        return 0;
    }
    return 1;
}

static int partition_label_valid(const char *input, char output[12]) {
    UINTN length = 0;
    UINTN index;
    memory_zero(output, 12U);
    if (!input || !input[0]) return 0;
    while (input[length]) length++;
    if (length > 11U) return 0;
    for (index = 0; index < length; index++) {
        char ch = input[index];
        if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-'))
            return 0;
        output[index] = ch;
    }
    output[length] = 0;
    return 1;
}

static int partition_label_unique(const char *label, UINTN exceptPartition) {
    UINTN index;
    for (index = 0; index < PARTITION_MAX; index++)
        if (index + 1U != exceptPartition && gPartitionNames[index][0] &&
            streq(gPartitionNames[index], label)) return 0;
    return 1;
}

static int partition_format_fat16(EFI_BLOCK_IO_PROTOCOL *io, UINT64 start, UINT64 sectors,
        const char label[12], UINT32 serial) {
    UINT8 sector[GPT_SECTOR_SIZE];
    UINT32 sectorsPerCluster = 1U;
    UINT32 rootEntries = 512U;
    UINT32 rootSectors = (rootEntries * 32U + GPT_SECTOR_SIZE - 1U) / GPT_SECTOR_SIZE;
    UINT32 fatSectors;
    UINT32 clusters;
    UINT32 previous;
    UINT64 lba;
    UINTN index;
    if (sectors < FAT_MIN_SECTORS || sectors > 0xffffffffULL) return 0;
    for (;;) {
        fatSectors = 1U;
        do {
            previous = fatSectors;
            clusters = (UINT32)((sectors - 1U - rootSectors - 2U * fatSectors) / sectorsPerCluster);
            fatSectors = (clusters + 2U) * 2U;
            fatSectors = (fatSectors + GPT_SECTOR_SIZE - 1U) / GPT_SECTOR_SIZE;
        } while (fatSectors != previous);
        clusters = (UINT32)((sectors - 1U - rootSectors - 2U * fatSectors) / sectorsPerCluster);
        if (clusters < 65525U) break;
        sectorsPerCluster <<= 1;
        if (sectorsPerCluster > 128U) return 0;
    }
    if (clusters < 4085U) return 0;
    memory_zero(sector, sizeof(sector));
    sector[0] = 0xeb; sector[1] = 0x3c; sector[2] = 0x90;
    memory_copy(sector + 3U, "TINYGPT ", 8U);
    partition_write_u16(sector + 11U, GPT_SECTOR_SIZE);
    sector[13] = (UINT8)sectorsPerCluster;
    partition_write_u16(sector + 14U, 1U);
    sector[16] = 2U;
    partition_write_u16(sector + 17U, (UINT16)rootEntries);
    if (sectors <= 65535U) partition_write_u16(sector + 19U, (UINT16)sectors);
    else partition_write_u32(sector + 32U, (UINT32)sectors);
    sector[21] = 0xf8;
    partition_write_u16(sector + 22U, (UINT16)fatSectors);
    partition_write_u16(sector + 24U, 63U);
    partition_write_u16(sector + 26U, 255U);
    partition_write_u32(sector + 28U, (UINT32)start);
    sector[36] = 0x80; sector[38] = 0x29;
    partition_write_u32(sector + 39U, serial);
    for (index = 0; index < 11U; index++) sector[43U + index] = label[index] ? (UINT8)label[index] : ' ';
    memory_copy(sector + 54U, "FAT16   ", 8U);
    memory_copy(sector + 62U, "TinyGPT data partition\r\n", 24U);
    sector[510] = 0x55; sector[511] = 0xaa;
    if (partition_write_blocks(io, start, GPT_SECTOR_SIZE, sector) != EFI_SUCCESS) return 0;
    memory_zero(sector, sizeof(sector));
    for (lba = 1U; lba < 1U + (UINT64)fatSectors * 2U + rootSectors; lba++)
        if (partition_write_blocks(io, start + lba, GPT_SECTOR_SIZE, sector) != EFI_SUCCESS) return 0;
    sector[0] = 0xf8; sector[1] = 0xff; sector[2] = 0xff; sector[3] = 0xff;
    if (partition_write_blocks(io, start + 1U, GPT_SECTOR_SIZE, sector) != EFI_SUCCESS ||
        partition_write_blocks(io, start + 1U + fatSectors, GPT_SECTOR_SIZE, sector) != EFI_SUCCESS)
        return 0;
    memory_zero(sector, sizeof(sector));
    for (index = 0; index < 11U; index++) sector[index] = label[index] ? (UINT8)label[index] : ' ';
    sector[11] = 0x08;
    memory_copy(sector + 32U, "TINYGPT NEW", 11U);
    sector[32U + 11U] = 0x20;
    lba = start + 1U + (UINT64)fatSectors * 2U;
    if (partition_write_blocks(io, lba, GPT_SECTOR_SIZE, sector) != EFI_SUCCESS) return 0;
    return io->FlushBlocks(io) == EFI_SUCCESS;
}

static void partition_set_gpt_name(UINT8 *entry, const char *label) {
    UINTN index;
    memory_zero(entry + 56U, 72U);
    for (index = 0; label[index] && index < 35U; index++)
        partition_write_u16(entry + 56U + index * 2U, (UINT16)(UINT8)label[index]);
}

static int partition_align_lba(UINT64 value, UINT64 *alignedOut) {
    if (value > ~0ULL - (GPT_ALIGNMENT_SECTORS - 1ULL)) return 0;
    *alignedOut = (value + GPT_ALIGNMENT_SECTORS - 1ULL) & ~(GPT_ALIGNMENT_SECTORS - 1ULL);
    return 1;
}

static int partition_table_extents_valid(PARTITION_DISK *disk) {
    UINT64 firstUsable = partition_read_u64(disk->primary + 40U);
    UINT64 lastUsable = partition_read_u64(disk->primary + 48U);
    UINTN left;
    if (firstUsable > lastUsable) return 0;
    for (left = 0; left < disk->entryCount; left++) {
        UINT8 *leftEntry = disk->entries + left * disk->entrySize;
        UINT64 leftStart;
        UINT64 leftEnd;
        UINTN right;
        if (partition_entry_empty(leftEntry)) continue;
        leftStart = partition_read_u64(leftEntry + 32U);
        leftEnd = partition_read_u64(leftEntry + 40U);
        if (leftStart < firstUsable || leftEnd > lastUsable || leftStart > leftEnd) return 0;
        for (right = left + 1U; right < disk->entryCount; right++) {
            UINT8 *rightEntry = disk->entries + right * disk->entrySize;
            UINT64 rightStart;
            UINT64 rightEnd;
            if (partition_entry_empty(rightEntry)) continue;
            rightStart = partition_read_u64(rightEntry + 32U);
            rightEnd = partition_read_u64(rightEntry + 40U);
            if (rightStart < firstUsable || rightEnd > lastUsable || rightStart > rightEnd ||
                !(leftEnd < rightStart || rightEnd < leftStart)) return 0;
        }
    }
    return 1;
}

static int partition_first_fit(PARTITION_DISK *disk, UINT64 sectors, UINT64 *startOut) {
    UINT64 cursor;
    UINT64 lastUsable = partition_read_u64(disk->primary + 48U);
    if (!sectors || !partition_table_extents_valid(disk) ||
        !partition_align_lba(partition_read_u64(disk->primary + 40U), &cursor)) return 0;
    for (;;) {
        UINT64 nextStart = ~0ULL;
        UINT64 nextEnd = 0;
        UINTN index;
        for (index = 0; index < disk->entryCount; index++) {
            UINT8 *entry = disk->entries + index * disk->entrySize;
            UINT64 start;
            UINT64 end;
            if (partition_entry_empty(entry)) continue;
            start = partition_read_u64(entry + 32U);
            end = partition_read_u64(entry + 40U);
            if (end >= cursor && start < nextStart) {
                nextStart = start;
                nextEnd = end;
            }
        }
        if (nextStart == ~0ULL) {
            if (cursor <= lastUsable && sectors - 1U <= lastUsable - cursor) {
                *startOut = cursor;
                return 1;
            }
            return 0;
        }
        if (cursor < nextStart && sectors <= nextStart - cursor) {
            *startOut = cursor;
            return 1;
        }
        if (nextEnd == ~0ULL || !partition_align_lba(nextEnd + 1U, &cursor) || cursor > lastUsable)
            return 0;
    }
}

static int partition_gpt_name_matches(const UINT8 *entry, const char *label) {
    UINTN index;
    for (index = 0; index < 36U; index++) {
        UINT16 value = (UINT16)entry[56U + index * 2U] |
            ((UINT16)entry[57U + index * 2U] << 8);
        UINT8 expected = label[index] ? (UINT8)label[index] : 0U;
        if (value != expected) return 0;
        if (!expected) {
            for (index++; index < 36U; index++)
                if (entry[56U + index * 2U] || entry[57U + index * 2U]) return 0;
            return 1;
        }
    }
    return 0;
}

static int partition_fat_label_matches(PARTITION_DISK *disk, UINT64 start,
        UINT64 extentSectors, const char *label) {
    UINT8 sector[GPT_SECTOR_SIZE];
    UINT32 bytesPerSector;
    UINT32 sectorsPerCluster;
    UINT32 reserved;
    UINT32 fats;
    UINT32 rootEntries;
    UINT32 fatSectors;
    UINT32 totalSectors;
    UINT64 rootSectors;
    UINT64 metadataSectors;
    UINTN labelOffset;
    UINTN index;
    if (partition_read_blocks(disk->io, start, GPT_SECTOR_SIZE, sector) != EFI_SUCCESS ||
        sector[510] != 0x55U || sector[511] != 0xaaU) return 0;
    bytesPerSector = (UINT32)sector[11] | ((UINT32)sector[12] << 8);
    sectorsPerCluster = sector[13];
    reserved = (UINT32)sector[14] | ((UINT32)sector[15] << 8);
    fats = sector[16];
    rootEntries = (UINT32)sector[17] | ((UINT32)sector[18] << 8);
    totalSectors = (UINT32)sector[19] | ((UINT32)sector[20] << 8);
    if (!totalSectors) totalSectors = partition_read_u32(sector + 32U);
    fatSectors = (UINT32)sector[22] | ((UINT32)sector[23] << 8);
    if (bytesPerSector != GPT_SECTOR_SIZE || !sectorsPerCluster ||
        (sectorsPerCluster & (sectorsPerCluster - 1U)) != 0U || sectorsPerCluster > 128U ||
        !reserved || !fats || !totalSectors || (UINT64)totalSectors > extentSectors) return 0;
    if (rootEntries) {
        if (!fatSectors) return 0;
        labelOffset = 43U;
    } else {
        UINT32 rootCluster;
        fatSectors = partition_read_u32(sector + 36U);
        rootCluster = partition_read_u32(sector + 44U);
        if (!fatSectors || rootCluster < 2U) return 0;
        labelOffset = 71U;
    }
    rootSectors = ((UINT64)rootEntries * 32U + GPT_SECTOR_SIZE - 1U) / GPT_SECTOR_SIZE;
    metadataSectors = (UINT64)reserved + (UINT64)fats * fatSectors + rootSectors;
    if (metadataSectors >= totalSectors) return 0;
    if (!rootEntries) {
        UINT64 rootCluster = partition_read_u32(sector + 44U);
        UINT64 rootOffset = metadataSectors + (rootCluster - 2U) * sectorsPerCluster;
        if (rootOffset >= totalSectors || sectorsPerCluster > totalSectors - rootOffset) return 0;
    }
    for (index = 0; index < 11U; index++) {
        UINT8 expected = label[index] ? (UINT8)label[index] : (UINT8)' ';
        if (sector[labelOffset + index] != expected) return 0;
    }
    return 1;
}

static int partition_entry_is_managed_fat(PARTITION_DISK *disk, UINTN partition,
        UINT64 *startOut, UINT64 *endOut) {
    UINT8 *entry;
    UINT64 start;
    UINT64 end;
    char normalized[12];
    if (partition < 2U || partition > PARTITION_MAX || partition > disk->entryCount ||
        !gPartitionNames[partition - 1U][0] ||
        !partition_label_valid(gPartitionNames[partition - 1U], normalized) ||
        !streq(normalized, gPartitionNames[partition - 1U]) ||
        !partition_table_extents_valid(disk)) return 0;
    entry = disk->entries + (partition - 1U) * disk->entrySize;
    if (partition_entry_empty(entry) ||
        !partition_bytes_equal(entry, (const UINT8 *)&gFatPartitionGuid, 16U) ||
        partition_bytes_equal(entry + 16U, (const UINT8 *)&gRecoveryPartitionGuid, 16U) ||
        !partition_gpt_name_matches(entry, normalized)) return 0;
    start = partition_read_u64(entry + 32U);
    end = partition_read_u64(entry + 40U);
    if (!partition_fat_label_matches(disk, start, end - start + 1U, normalized)) return 0;
    if (startOut) *startOut = start;
    if (endOut) *endOut = end;
    return 1;
}

static int partition_delete_validate(UINTN partition, UINT64 *startOut, UINT64 *endOut) {
    PARTITION_DISK disk;
    int valid;
    gPartitionError = 0;
    if (partition < 2U || partition > PARTITION_MAX) { gPartitionError = 40U; return 0; }
    if (gPartitionRebootRequired) { gPartitionError = 6U; return 0; }
    if (!partition_disk_open(&disk)) return 0;
    valid = partition_entry_is_managed_fat(&disk, partition, startOut, endOut);
    partition_disk_close(&disk);
    if (!valid) gPartitionError = 41U;
    return valid;
}

static UINTN partition_delete_fallback(UINTN removed) {
    UINTN partition;
    if (removed != 2U && gPartitionNames[1][0] && gPartitionRoots[1]) return 2U;
    for (partition = 2U; partition <= PARTITION_MAX; partition++)
        if (partition != removed && gPartitionNames[partition - 1U][0] &&
            gPartitionRoots[partition - 1U]) return partition;
    return 1U;
}

/* Return 2 when GPT deletion committed but recovery-registry cleanup failed. */
static int partition_delete(UINTN partition) {
    PARTITION_DISK disk;
    EFI_FILE_PROTOCOL *removedRoot;
    UINTN oldDefault;
    UINTN fallback;
    int defaultChanged;
    UINT8 *entry;
    if (!partition_delete_validate(partition, (UINT64 *)0, (UINT64 *)0)) return 0;
    if (!partition_disk_open(&disk)) return 0;
    if (!partition_entry_is_managed_fat(&disk, partition, (UINT64 *)0, (UINT64 *)0)) {
        partition_disk_close(&disk); gPartitionError = 41U; return 0;
    }
    removedRoot = gPartitionRoots[partition - 1U];
    if (removedRoot == gBootVolumeRoot) {
        partition_disk_close(&disk); gPartitionError = 41U; return 0;
    }
    oldDefault = boot_order_default_partition();
    fallback = partition_delete_fallback(partition);
    defaultChanged = oldDefault == partition;
    if (defaultChanged && !boot_order_save(fallback)) {
        partition_disk_close(&disk); gPartitionError = 42U; return 0;
    }

    if (gActivePartition == partition) {
        if (fallback >= 2U) {
            (void)storage_activate_partition(fallback);
            (void)storage_mount_latest();
        } else {
            gVolumeRoot = (EFI_FILE_PROTOCOL *)0;
            gActivePartition = 1U;
            gDedicatedStorage = 0U;
            gCwd = FS_ROOT;
            gPreviousCwd = FS_ROOT;
            gGeneration = 0;
            gTransactionCorrupt = 0;
            memory_zero(gNodes, sizeof(gNodes));
        }
    }
    if (removedRoot && removedRoot->Close(removedRoot) != EFI_SUCCESS) {
        partition_disk_close(&disk);
        gPartitionError = defaultChanged && !boot_order_save(oldDefault) ? 45U : 43U;
        return 0;
    }
    gPartitionRoots[partition - 1U] = (EFI_FILE_PROTOCOL *)0;

    entry = disk.entries + (partition - 1U) * disk.entrySize;
    memory_zero(entry, GPT_ENTRY_SIZE);
    if (!partition_disk_commit(&disk)) {
        UINTN commitError = gPartitionError;
        partition_disk_close(&disk);
        /* The GPT may be restored, but the target root was already closed. */
        gPartitionRebootRequired = 1U;
        if (commitError != 31U && defaultChanged && !boot_order_save(oldDefault))
            commitError = 46U;
        gPartitionError = commitError;
        return 0;
    }
    partition_disk_close(&disk);
    gPartitionRebootRequired = 1U;
    memory_zero(gPartitionNames[partition - 1U], sizeof(gPartitionNames[partition - 1U]));
    if (!partition_registry_save()) {
        /* The in-memory view stays deleted; PARTS.CFG may require recovery. */
        gPartitionError = 44U;
        return 2;
    }
    return 1;
}

static int partition_add(UINT32 mebibytes, const char *requestedLabel, UINTN *partitionOut) {
    PARTITION_DISK disk;
    UINT8 *entry = (UINT8 *)0;
    char label[12];
    UINTN index;
    UINTN slot = PARTITION_MAX;
    UINT64 start = 0;
    UINT64 sectors = (UINT64)mebibytes * 2048ULL;
    EFI_GUID unique;
    gPartitionError = 0;
    if (gPartitionRebootRequired) { gPartitionError = 6U; return 0; }
    if (mebibytes < 4U || !partition_label_valid(requestedLabel, label) ||
        !partition_label_unique(label, 0U)) { gPartitionError = 1U; return 0; }
    if (!partition_disk_open(&disk)) return 0;
    for (index = 1U; index < disk.entryCount && index < PARTITION_MAX; index++) {
        UINT8 *candidate = disk.entries + index * disk.entrySize;
        if (partition_entry_empty(candidate) && slot == PARTITION_MAX) slot = index;
    }
    if (slot == PARTITION_MAX || !partition_first_fit(&disk, sectors, &start)) {
        partition_disk_close(&disk);
        gPartitionError = 2U;
        return 0;
    }
    if (!partition_format_fat16(disk.io, start, sectors, label,
            (UINT32)(timer_count() ^ start ^ sectors))) {
        partition_disk_close(&disk);
        gPartitionError = 3U;
        return 0;
    }
    entry = disk.entries + slot * disk.entrySize;
    memory_zero(entry, disk.entrySize);
    memory_copy(entry, &gFatPartitionGuid, 16U);
    unique.Data1 = 0x54475054U ^ (UINT32)timer_count();
    unique.Data2 = (UINT16)(start ^ (start >> 16));
    unique.Data3 = (UINT16)(((sectors ^ (sectors >> 16)) & 0x0fffU) | 0x4000U);
    memory_zero(unique.Data4, sizeof(unique.Data4));
    unique.Data4[0] = 0x80U;
    unique.Data4[1] = (UINT8)(slot + 1U);
    partition_write_u64(unique.Data4, timer_count() ^ (start << 7));
    unique.Data4[0] = (UINT8)((unique.Data4[0] & 0x3fU) | 0x80U);
    memory_copy(entry + 16U, &unique, 16U);
    partition_write_u64(entry + 32U, start);
    partition_write_u64(entry + 40U, start + sectors - 1U);
    partition_set_gpt_name(entry, label);
    if (!partition_disk_commit(&disk)) {
        partition_disk_close(&disk);
        if (gPartitionError != 30U && gPartitionError != 31U) gPartitionError = 4U;
        if (gPartitionError == 31U) gPartitionRebootRequired = 1U;
        return 0;
    }
    partition_disk_close(&disk);
    gPartitionRebootRequired = 1U;
    string_copy(gPartitionNames[slot], label, sizeof(gPartitionNames[slot]));
    if (!partition_registry_save()) { gPartitionError = 5U; return 0; }
    if (partitionOut) *partitionOut = slot + 1U;
    return 1;
}

static const char *partition_error_text(void) {
    if (gPartitionError == 1U) return "use 4-63 MiB and a unique 1-11 character name";
    if (gPartitionError == 2U) return "no aligned free extent or GPT slot can hold that size";
    if (gPartitionError == 3U) return "the new FAT filesystem could not be written";
    if (gPartitionError == 4U) return "the mirrored GPT transaction could not be committed";
    if (gPartitionError == 5U) return "partition created, but its recovery registry entry could not be saved; reboot and do not reuse the slot";
    if (gPartitionError == 6U) return "reboot is required before further partition changes";
    if (gPartitionError == 30U) return "the original GPT was restored, but the target handle is closed; reboot required before retrying";
    if (gPartitionError == 31U) return "indeterminate GPT state; stop partition operations and reboot into recovery immediately";
    if (gPartitionError == 40U) return "partition 1 is protected recovery storage; use a number from 2 through 16";
    if (gPartitionError == 41U) return "target is absent, unregistered, malformed, or not a TinyGPT-managed FAT partition";
    if (gPartitionError == 42U) return "the default boot fallback could not be saved; nothing was deleted";
    if (gPartitionError == 43U) return "the mounted filesystem handle could not be closed; nothing was deleted";
    if (gPartitionError == 44U) return "partition deleted, but registry cleanup failed; do not reuse the slot and reboot into recovery";
    if (gPartitionError == 45U) return "nothing was deleted, but the prior default boot selection could not be restored";
    if (gPartitionError == 46U) return "the original GPT was restored, but the target handle is closed and the prior default could not be restored; reboot required";
    return "the TinyGPT boot disk is unavailable, read-only, or has an unsupported GPT layout";
}

static int partition_update_fat_label(PARTITION_DISK *disk, UINT64 start, const char label[12]) {
    UINT8 sector[GPT_SECTOR_SIZE];
    UINT32 reserved;
    UINT32 fats;
    UINT32 fatSectors;
    UINT32 rootEntries;
    UINT32 sectorsPerCluster;
    UINT32 rootCluster = 0;
    UINT32 backupBoot = 0;
    UINTN labelOffset;
    UINT64 rootLba;
    UINTN index;
    if (partition_read_blocks(disk->io, start, GPT_SECTOR_SIZE, sector) != EFI_SUCCESS ||
        sector[510] != 0x55 || sector[511] != 0xaa) return 0;
    reserved = (UINT32)sector[14] | ((UINT32)sector[15] << 8);
    fats = sector[16];
    sectorsPerCluster = sector[13];
    rootEntries = (UINT32)sector[17] | ((UINT32)sector[18] << 8);
    fatSectors = (UINT32)sector[22] | ((UINT32)sector[23] << 8);
    if (!reserved || !fats || !sectorsPerCluster) return 0;
    if (rootEntries && fatSectors) {
        labelOffset = 43U;
        rootLba = start + reserved + (UINT64)fats * fatSectors;
    } else {
        fatSectors = partition_read_u32(sector + 36U);
        rootCluster = partition_read_u32(sector + 44U);
        backupBoot = (UINT32)sector[50] | ((UINT32)sector[51] << 8);
        if (!fatSectors || rootCluster < 2U) return 0;
        labelOffset = 71U;
        rootLba = start + reserved + (UINT64)fats * fatSectors +
            (UINT64)(rootCluster - 2U) * sectorsPerCluster;
    }
    for (index = 0; index < 11U; index++) sector[labelOffset + index] = label[index] ? (UINT8)label[index] : ' ';
    if (partition_write_blocks(disk->io, start, GPT_SECTOR_SIZE, sector) != EFI_SUCCESS) return 0;
    if (backupBoot && backupBoot < reserved &&
        partition_write_blocks(disk->io, start + backupBoot, GPT_SECTOR_SIZE, sector) != EFI_SUCCESS) return 0;
    if (partition_read_blocks(disk->io, rootLba, GPT_SECTOR_SIZE, sector) != EFI_SUCCESS) return 0;
    for (index = 0; index < GPT_SECTOR_SIZE; index += 32U) {
        if (sector[index] == 0U || sector[index] == 0xe5U || sector[index + 11U] == 0x08U) {
            UINTN character;
            memory_zero(sector + index, 32U);
            for (character = 0; character < 11U; character++)
                sector[index + character] = label[character] ? (UINT8)label[character] : ' ';
            sector[index + 11U] = 0x08U;
            return partition_write_blocks(disk->io, rootLba, GPT_SECTOR_SIZE, sector) == EFI_SUCCESS &&
                disk->io->FlushBlocks(disk->io) == EFI_SUCCESS;
        }
    }
    return 0;
}

static int partition_rename(UINTN partition, const char *requestedLabel) {
    PARTITION_DISK disk;
    UINT8 *entry;
    UINT64 start;
    char label[12];
    if (gPartitionRebootRequired || partition < 2U || partition > PARTITION_MAX ||
        !partition_label_valid(requestedLabel, label) ||
        !partition_label_unique(label, partition) || !partition_disk_open(&disk)) return 0;
    if (partition > disk.entryCount) {
        partition_disk_close(&disk);
        return 0;
    }
    entry = disk.entries + (partition - 1U) * disk.entrySize;
    if (partition_entry_empty(entry)) {
        partition_disk_close(&disk);
        return 0;
    }
    start = partition_read_u64(entry + 32U);
    partition_set_gpt_name(entry, label);
    if (!partition_disk_commit(&disk)) {
        if (gPartitionError == 31U) gPartitionRebootRequired = 1U;
        partition_disk_close(&disk);
        return 0;
    }
    gPartitionRebootRequired = 1U;
    if (!partition_update_fat_label(&disk, start, label)) {
        partition_disk_close(&disk);
        return 0;
    }
    partition_disk_close(&disk);
    string_copy(gPartitionNames[partition - 1U], label, sizeof(gPartitionNames[partition - 1U]));
    return partition_registry_save();
}
