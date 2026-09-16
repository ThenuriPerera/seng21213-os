#ifndef RAMDISK_H
#define RAMDISK_H

#include "../include/types.h"

#define RAMDISK_SIZE   (128 * 1024)   /* 1 MB */
#define BLOCK_SIZE     4096
#define TOTAL_BLOCKS   (RAMDISK_SIZE / BLOCK_SIZE)

void ramdisk_init(void);
void ramdisk_read(uint32_t block_num, void *buf);
void ramdisk_write(uint32_t block_num, const void *buf);

#endif /* RAMDISK_H */
