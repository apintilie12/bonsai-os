#include "fs/fat32.h"
#include "drivers/block_dev.h"
#include "lib/string.h"
#include "lib/log.h"
#include "lib/printf.h"

// Read a little-endian 16-bit value from a byte buffer.
static inline unsigned int le16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

// Read a little-endian 32-bit value from a byte buffer.
static inline unsigned int le32(const unsigned char *p)
{
    return (unsigned int)p[0]
         | ((unsigned int)p[1] <<  8)
         | ((unsigned int)p[2] << 16)
         | ((unsigned int)p[3] << 24);
}

// First LBA of a data cluster (clusters are numbered from 2).
static inline unsigned int cluster_to_lba(const fat32_vol_t *vol, unsigned int cluster)
{
    return vol->data_start_lba + (cluster - 2) * vol->sectors_per_cluster;
}


static void normalise_83(const char *in, char out[11])
{
    int i;
    for (i = 0; i < 11; i++)
        out[i] = ' ';

    /* "." and ".." are stored literally in directory entries — handle them
       before the general 8.3 logic, which would otherwise misparse them. */
    if (in[0] == '.' && in[1] == '\0') {
        out[0] = '.';
        return;
    }
    if (in[0] == '.' && in[1] == '.' && in[2] == '\0') {
        out[0] = '.'; out[1] = '.';
        return;
    }

    /* Find the last dot to split base and extension. */
    const char *dot = (const char *)0;
    for (const char *p = in; *p; p++)
        if (*p == '.')
            dot = p;

    int base_len = dot ? (int)(dot - in) : (int)strlen(in);
    if (base_len > 8) base_len = 8;

    for (i = 0; i < base_len; i++)
        out[i] = toupper(in[i]);

    if (dot) {
        const char *ext = dot + 1;
        for (i = 0; i < 3 && ext[i]; i++)
            out[8 + i] = toupper(ext[i]);
    }
}


static unsigned char fat_cache[512];
static unsigned int  fat_cache_lba = (unsigned int)-1;

static int fat_cache_read(unsigned int lba)
{
    if (lba == fat_cache_lba)
        return 0;
    if (blkdev_read(lba, fat_cache) < 0)
        return -1;
    fat_cache_lba = lba;
    return 0;
}

// Follow one step in the FAT32 cluster chain.
// Returns the next cluster number, 0x0FFFFFFF for end-of-chain, or 0 on error.
static unsigned int fat_next_cluster(const fat32_vol_t *vol, unsigned int cluster)
{
    unsigned int byte_offset = cluster * 4;
    unsigned int lba         = vol->fat_start_lba + byte_offset / 512;
    unsigned int idx         = byte_offset % 512;

    if (fat_cache_read(lba) < 0)
        return 0;

    return le32(fat_cache + idx) & 0x0FFFFFFFu;
}

int fat32_mount(fat32_vol_t *vol)
{
    unsigned char buf[512];

    // Read MBR (LBA 0)
    if (blkdev_read(0, buf) < 0) {
        LOG_DEBUG("FAT32: failed to read MBR\r\n");
        return -1;
    }

    // Partition entry 0 starts at offset 0x1BE (16 bytes each)
    unsigned char *pe = buf + 0x1BE;
    unsigned char type = pe[4];
    if (type != 0x0B && type != 0x0C) {
        LOG_DEBUG("FAT32: partition 0 type 0x%x is not FAT32\r\n", type);
        return -1;
    }
    vol->first_lba = le32(pe + 8);

    // Read Volume Boot Record / BPB
    if (blkdev_read(vol->first_lba, buf) < 0) {
        LOG_DEBUG("FAT32: failed to read VBR at LBA %u\r\n", vol->first_lba);
        return -1;
    }

    vol->bytes_per_sector    = le16(buf + 11);
    vol->sectors_per_cluster = buf[13];
    unsigned int reserved    = le16(buf + 14);
    unsigned int num_fats    = buf[16];
    vol->fat_size_32         = le32(buf + 36);
    vol->root_cluster        = le32(buf + 44);

    if (vol->bytes_per_sector != 512) {
        LOG_DEBUG("FAT32: unsupported sector size %u\r\n", vol->bytes_per_sector);
        return -1;
    }
    if (num_fats < 1) {
        LOG_DEBUG("FAT32: invalid num_fats %u\r\n", num_fats);
        return -1;
    }

    vol->fat_start_lba  = vol->first_lba + reserved;
    vol->data_start_lba = vol->fat_start_lba + num_fats * vol->fat_size_32;

    LOG_DEBUG("FAT32: mounted — first_lba=%u fat_start=%u data_start=%u spc=%u root_clus=%u\r\n",
              vol->first_lba, vol->fat_start_lba, vol->data_start_lba,
              vol->sectors_per_cluster, vol->root_cluster);
    return 0;
}

int fat32_open(fat32_vol_t *vol, unsigned int cwd_cluster, const char *path,
               unsigned int *first_cluster, unsigned int *file_size)
{
    char target[11];
    normalise_83(path, target);

    unsigned char buf[512];
    unsigned int cluster = cwd_cluster;

    while (cluster < 0x0FFFFFF8u) {
        unsigned int lba = cluster_to_lba(vol, cluster);

        for (unsigned int s = 0; s < vol->sectors_per_cluster; s++) {
            if (blkdev_read(lba + s, buf) < 0) {
                LOG_DEBUG("FAT32: read error in dir cluster %u\r\n", cluster);
                return -1;
            }

            // Each directory sector holds 512/32 = 16 entries
            for (int e = 0; e < 512 / 32; e++) {
                unsigned char *entry = buf + e * 32;

                if (entry[0] == 0x00)   // end of directory
                    return -1;
                if (entry[0] == 0xE5)   // deleted entry
                    continue;
                if (entry[11] == 0x0F)  // Long File Name entry
                    continue;

                if (strncmp((const char *)entry, target, 11) == 0) {
                    unsigned int clus_hi = le16(entry + 20);
                    unsigned int clus_lo = le16(entry + 26);
                    *first_cluster = (clus_hi << 16) | clus_lo;
                    *file_size     = le32(entry + 28);
                    return 0;
                }
            }
        }

        cluster = fat_next_cluster(vol, cluster);
    }

    LOG_DEBUG("FAT32: file not found: %s\r\n", path);
    return -1;
}

int fat32_read(fat32_vol_t *vol, unsigned int first_cluster,
               void *buf, unsigned int max_bytes)
{
    static unsigned char sector_buf[512];
    unsigned char *dst     = (unsigned char *)buf;
    unsigned int remaining = max_bytes;
    unsigned int cluster   = first_cluster;

    while (cluster < 0x0FFFFFF8u && remaining > 0) {
        unsigned int lba = cluster_to_lba(vol, cluster);

        for (unsigned int s = 0; s < vol->sectors_per_cluster && remaining > 0; s++) {
            if (blkdev_read(lba + s, sector_buf) < 0) {
                LOG_DEBUG("FAT32: read error at LBA %u\r\n", lba + s);
                return -1;
            }

            unsigned int to_copy = remaining < 512 ? remaining : 512;
            for (unsigned int i = 0; i < to_copy; i++)
                dst[i] = sector_buf[i];

            dst       += to_copy;
            remaining -= to_copy;
        }

        cluster = fat_next_cluster(vol, cluster);
    }

    return (int)(max_bytes - remaining);
}

/* Convert a raw 11-byte 8.3 directory name to a NUL-terminated display string.
   "HELLO   ELF" -> "HELLO.ELF", "MYDIR      " -> "MYDIR" */
static void format_83(const unsigned char name[11], char out[13])
{
    int i, n = 0;
    for (i = 0; i < 8 && name[i] != ' '; i++)
        out[n++] = name[i];
    if (name[8] != ' ') {
        out[n++] = '.';
        for (i = 8; i < 11 && name[i] != ' '; i++)
            out[n++] = name[i];
    }
    out[n] = '\0';
}

/* Print name left-justified in a field of width cols, space-padded on the right. */
static void ls_pad(const char *s, int cols)
{
    int n = 0;
    while (s[n]) { printf("%c", s[n++]); }
    while (n++ < cols) printf(" ");
}

void fat32_ls(fat32_vol_t *vol, unsigned int cwd_cluster)
{
    unsigned char buf[512];
    unsigned int cluster = cwd_cluster;
    int count = 0;

    while (cluster < 0x0FFFFFF8u) {
        unsigned int lba = cluster_to_lba(vol, cluster);

        for (unsigned int s = 0; s < vol->sectors_per_cluster; s++) {
            if (blkdev_read(lba + s, buf) < 0) {
                printf("ls: read error\r\n");
                return;
            }

            for (int e = 0; e < 512 / 32; e++) {
                unsigned char *entry = buf + e * 32;

                if (entry[0] == 0x00)   // end of directory
                    return;
                if (entry[0] == 0xE5)   // deleted
                    continue;
                if (entry[11] == 0x0F)  // LFN
                    continue;

                char name[13];
                format_83(entry, name);

                unsigned char attr = entry[11];
                int is_dir = (attr & 0x10) != 0;
                unsigned int size = le32(entry + 28);

                printf("  ");
                ls_pad(name, 14);
                if (is_dir)
                    printf("<DIR>\r\n");
                else
                    printf("%u\r\n", size);

                count++;
            }
        }

        cluster = fat_next_cluster(vol, cluster);
    }

    if (count == 0)
        printf("  (empty)\r\n");
}

int fat32_cd(fat32_vol_t *vol, unsigned int cwd_cluster, const char *name,
             unsigned int *out_cluster)
{
    if (name[0] == '/' && name[1] == '\0') {
        *out_cluster = vol->root_cluster;
        return 0;
    }

    char target[11];
    normalise_83(name, target);

    unsigned char buf[512];
    unsigned int cluster = cwd_cluster;

    while (cluster < 0x0FFFFFF8u) {
        unsigned int lba = cluster_to_lba(vol, cluster);

        for (unsigned int s = 0; s < vol->sectors_per_cluster; s++) {
            if (blkdev_read(lba + s, buf) < 0)
                return -1;

            for (int e = 0; e < 512 / 32; e++) {
                unsigned char *entry = buf + e * 32;

                if (entry[0] == 0x00)
                    return -1;
                if (entry[0] == 0xE5)
                    continue;
                if (entry[11] == 0x0F)
                    continue;
                if (!(entry[11] & 0x10))  // not a directory
                    continue;

                if (strncmp((const char *)entry, target, 11) == 0) {
                    unsigned int clus_hi = le16(entry + 20);
                    unsigned int clus_lo = le16(entry + 26);
                    unsigned int next    = (clus_hi << 16) | clus_lo;
                    /* cluster 0 in the ".." entry of a top-level dir means root */
                    *out_cluster = (next == 0) ? vol->root_cluster : next;
                    return 0;
                }
            }
        }

        cluster = fat_next_cluster(vol, cluster);
    }

    return -1;
}
