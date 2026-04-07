#ifndef _BLOCK_DEV_H
#define _BLOCK_DEV_H

typedef struct {
    int (*read_block)(unsigned int lba, void *buf);
} block_dev_t;

void blkdev_register(block_dev_t *dev);
int  blkdev_read(unsigned int lba, void *buf);

#endif // _BLOCK_DEV_H
