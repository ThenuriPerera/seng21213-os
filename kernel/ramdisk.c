#include "ramdisk.h"

static uint8_t disk[RAMDISK_SIZE];

void ramdisk_init(void) {
    for (uint32_t i = 0; i < RAMDISK_SIZE; i++) {
        disk[i] = 0;
    }
}

void ramdisk_read(uint32_t block_num, void *buf) {
    uint8_t *dst = (uint8_t *)buf;
    uint8_t *src = disk + (block_num * BLOCK_SIZE);
    for (uint32_t i = 0; i < BLOCK_SIZE; i++) {
        dst[i] = src[i];
    }
}

void ramdisk_write(uint32_t block_num, const void *buf) {
    const uint8_t *src = (const uint8_t *)buf;
    uint8_t *dst = disk + (block_num * BLOCK_SIZE);
    for (uint32_t i = 0; i < BLOCK_SIZE; i++) {
        dst[i] = src[i];
    }
}
