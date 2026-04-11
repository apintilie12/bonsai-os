#ifndef _FAT32_H
#define _FAT32_H

typedef struct {
    unsigned int first_lba;           // LBA of partition's Volume Boot Record
    unsigned int fat_start_lba;       // LBA of first FAT
    unsigned int data_start_lba;      // LBA of first data cluster
    unsigned int sectors_per_cluster;
    unsigned int root_cluster;        // first cluster of root directory (usually 2)
    unsigned int fat_size_32;         // sectors per FAT copy
    unsigned int bytes_per_sector;    // always 512 in practice
} fat32_vol_t;

// Parse MBR + BPB from the block device and fill vol.
// Returns 0 on success, -1 on error.
int fat32_mount(fat32_vol_t *vol);

// Find a file in cwd_cluster by name (e.g. "HELLO.ELF").
// On success fills *first_cluster and *file_size, returns 0. Returns -1 if not found.
int fat32_open(fat32_vol_t *vol, unsigned int cwd_cluster, const char *path,
               unsigned int *first_cluster, unsigned int *file_size);

// Read up to max_bytes from the file starting at first_cluster into buf.
// Returns number of bytes read, or -1 on error.
int fat32_read(fat32_vol_t *vol, unsigned int first_cluster,
               void *buf, unsigned int max_bytes);

// List the contents of cwd_cluster to the console.
void fat32_ls(fat32_vol_t *vol, unsigned int cwd_cluster);

// Change directory: find name inside cwd_cluster and return its cluster in *out_cluster.
// Pass "/" to go to root. Returns 0 on success, -1 if not found or not a directory.
int fat32_cd(fat32_vol_t *vol, unsigned int cwd_cluster, const char *name,
             unsigned int *out_cluster);

#endif // _FAT32_H
