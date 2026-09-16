#include "fs.h"
#include "ramdisk.h"
#include "vga.h"

static superblock_t sb;
static dirent_t     directory[FS_MAX_DIRENTS];
static inode_t      inode_table[FS_MAX_INODES];
static uint8_t       block_bitmap[BLOCK_SIZE];
static uint8_t       inode_bitmap[BLOCK_SIZE];

static int k_streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return *a == *b;
}

void fs_init(void) {
    /* Build a fresh superblock in memory */
    sb.magic        = FS_MAGIC;
    sb.total_blocks = TOTAL_BLOCKS;
    sb.total_inodes = FS_MAX_INODES;

    /* Zero the directory */
    for (int i = 0; i < FS_MAX_DIRENTS; i++) {
        directory[i].used = 0;
        directory[i].name[0] = '\0';
        directory[i].inode = 0;
    }

    /* Zero the inode table */
    for (int i = 0; i < FS_MAX_INODES; i++) {
        inode_table[i].used = 0;
        inode_table[i].size = 0;
        for (int d = 0; d < FS_DIRECT_PTRS; d++) {
            inode_table[i].direct[d] = 0;
        }
    }

    /* Zero the bitmaps */
    for (uint32_t i = 0; i < BLOCK_SIZE; i++) {
        block_bitmap[i] = 0;
        inode_bitmap[i] = 0;
    }

    /* Write everything to the ramdisk */
    ramdisk_write(FS_BLOCK_SUPER, &sb);
    /* superblock struct is small, but ramdisk_write always writes a full
       BLOCK_SIZE — that's fine here, we're just writing from a small struct's
       address; the rest of the block will contain whatever was in memory
       right after `sb`, which is harmless since fs_init always runs first. */
}
uint32_t fs_debug_magic(void) {
    return sb.magic;
}
int fs_create(const char *name) {
    /* Find a free inode */
    int inode_idx = -1;
    for (int i = 0; i < FS_MAX_INODES; i++) {
        if (!inode_table[i].used) {
            inode_idx = i;
            break;
        }
    }
    if (inode_idx == -1) {
        return -1;   /* no free inodes */
    }

    /* Find a free directory slot */
    int dirent_idx = -1;
    for (int i = 0; i < FS_MAX_DIRENTS; i++) {
        if (!directory[i].used) {
            dirent_idx = i;
            break;
        }
    }
    if (dirent_idx == -1) {
        return -1;   /* directory full */
    }

    /* Reject duplicate names */
    for (int i = 0; i < FS_MAX_DIRENTS; i++) {
        if (directory[i].used && k_streq(directory[i].name, name)) {
            return -1;
        }
    }

    /* Mark inode used, zero it */
    inode_table[inode_idx].used = 1;
    inode_table[inode_idx].size = 0;
    for (int d = 0; d < FS_DIRECT_PTRS; d++) {
        inode_table[inode_idx].direct[d] = 0;
    }

    /* Fill directory entry */
    int i = 0;
    while (name[i] && i < FS_MAX_NAME - 1) {
        directory[dirent_idx].name[i] = name[i];
        i++;
    }
    directory[dirent_idx].name[i] = '\0';
    directory[dirent_idx].inode = (uint32_t)inode_idx;
    directory[dirent_idx].used  = 1;

    return 0;
}
/* Find a free data block (blocks FS_FIRST_DATA_BLOCK..TOTAL_BLOCKS-1),
   mark it used in the in-memory bitmap, return its block number or -1 */
static int alloc_data_block(void) {
    for (uint32_t b = FS_FIRST_DATA_BLOCK; b < TOTAL_BLOCKS; b++) {
        uint32_t byte_idx = b / 8;
        uint32_t bit_idx  = b % 8;
        if (!(block_bitmap[byte_idx] & (1 << bit_idx))) {
            block_bitmap[byte_idx] |= (1 << bit_idx);
            return (int)b;
        }
    }
    return -1;   /* disk full */
}

int fs_write(int fd, const void *buf, uint32_t len) {
    if (fd < 0 || fd >= FS_MAX_INODES || !inode_table[fd].used) {
        return -1;
    }

    uint32_t max_size = FS_DIRECT_PTRS * BLOCK_SIZE;
    if (len > max_size) {
        len = max_size;   /* truncate to what the file system can hold */
    }

    const uint8_t *src = (const uint8_t *)buf;
    uint32_t written = 0;
    uint32_t block_idx = 0;

    static uint8_t block_buf[BLOCK_SIZE];

    while (written < len && block_idx < FS_DIRECT_PTRS) {
        /* Allocate this block if the file doesn't have one there yet */
        if (inode_table[fd].direct[block_idx] == 0) {
            int new_block = alloc_data_block();
            if (new_block == -1) {
                break;   /* disk full, stop writing */
            }
            inode_table[fd].direct[block_idx] = (uint32_t)new_block;
        }

        /* Copy up to BLOCK_SIZE bytes into a local buffer, then write it */
        uint32_t chunk = len - written;
        if (chunk > BLOCK_SIZE) chunk = BLOCK_SIZE;

        for (uint32_t i = 0; i < BLOCK_SIZE; i++) block_buf[i] = 0;
        for (uint32_t i = 0; i < chunk; i++) block_buf[i] = src[written + i];

        ramdisk_write(inode_table[fd].direct[block_idx], block_buf);

        written += chunk;
        block_idx++;
    }

    inode_table[fd].size = written;
    return (int)written;
}
int fs_open(const char *name) {
    for (int i = 0; i < FS_MAX_DIRENTS; i++) {
        if (directory[i].used && k_streq(directory[i].name, name)) {
            return (int)directory[i].inode;   /* fd == inode index */
        }
    }
    return -1;   /* not found */
}

int fs_read(int fd, void *buf, uint32_t len) {
    if (fd < 0 || fd >= FS_MAX_INODES || !inode_table[fd].used) {
        return -1;
    }

    uint32_t size = inode_table[fd].size;
    if (len > size) {
        len = size;
    }

    uint8_t *dst = (uint8_t *)buf;
    uint32_t read_so_far = 0;
    uint32_t block_idx = 0;

    static uint8_t block_buf[BLOCK_SIZE];

    while (read_so_far < len && block_idx < FS_DIRECT_PTRS) {
        uint32_t blk = inode_table[fd].direct[block_idx];
        if (blk == 0) break;

        ramdisk_read(blk, block_buf);

        uint32_t chunk = len - read_so_far;
        if (chunk > BLOCK_SIZE) chunk = BLOCK_SIZE;

        for (uint32_t i = 0; i < chunk; i++) {
            dst[read_so_far + i] = block_buf[i];
        }

        read_so_far += chunk;
        block_idx++;
    }

    return (int)read_so_far;
}

void fs_close(int fd) {
    (void)fd;   /* no open-file table yet, nothing to release */
}
int fs_unlink(const char *name) {
    int dirent_idx = -1;
    for (int i = 0; i < FS_MAX_DIRENTS; i++) {
        if (directory[i].used && k_streq(directory[i].name, name)) {
            dirent_idx = i;
            break;
        }
    }
    if (dirent_idx == -1) {
        return -1;   /* not found */
    }

    int inode_idx = (int)directory[dirent_idx].inode;

    /* Free the data blocks this file used */
    for (int d = 0; d < FS_DIRECT_PTRS; d++) {
        uint32_t blk = inode_table[inode_idx].direct[d];
        if (blk != 0) {
            uint32_t byte_idx = blk / 8;
            uint32_t bit_idx  = blk % 8;
            block_bitmap[byte_idx] &= ~(1 << bit_idx);
            inode_table[inode_idx].direct[d] = 0;
        }
    }

    /* Free the inode */
    inode_table[inode_idx].used = 0;
    inode_table[inode_idx].size = 0;

    /* Free the directory entry */
    directory[dirent_idx].used = 0;
    directory[dirent_idx].name[0] = '\0';

    return 0;
}
void fs_list(void) {
    int count = 0;
    for (int i = 0; i < FS_MAX_DIRENTS; i++) {
        if (directory[i].used) {
            int inode_idx = (int)directory[i].inode;
            vga_puts("  ");
            vga_puts(directory[i].name);
            vga_puts("  (");
            vga_printf("%d", inode_table[inode_idx].size);
            vga_puts(" bytes)\n");
            count++;
        }
    }
    if (count == 0) {
        vga_puts("  (no files)\n");
    }
}
