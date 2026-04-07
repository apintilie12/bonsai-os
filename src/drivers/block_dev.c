#include "drivers/block_dev.h"

static block_dev_t *registered_dev;

void blkdev_register(block_dev_t *dev)
{
    registered_dev = dev;
}

int blkdev_read(unsigned int lba, void *buf)
{
    if (!registered_dev)
        return -1;
    return registered_dev->read_block(lba, buf);
}
