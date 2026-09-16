#ifndef FS_H
#define FS_H

#include "../include/types.h"

#define FS_MAGIC          0x53454E47u   /* 'SENG' */
#define FS_MAX_INODES     16
#define FS_MAX_DIRENTS    16
#define FS_DIRECT_PTRS    8
#define FS_MAX_NAME       28

#define FS_BLOCK_SUPER      0
#define FS_BLOCK_DIR        1
#define FS_BLOCK_BITMAP     2
#define FS_BLOCK_INOBITMAP  3
#define FS_BLOCK_INOTABLE   4
#define FS_FIRST_DATA_BLOCK 5

typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t total_inodes;
} superblock_t;

typedef struct {
    uint8_t  used;
    uint32_t size;
    uint32_t direct[FS_DIRECT_PTRS];
} inode_t;

typedef struct {
    char     name[FS_MAX_NAME];
    uint32_t inode;
    uint8_t  used;
} dirent_t;

void fs_init(void);
int  fs_create(const char *name);
int  fs_open(const char *name);
int  fs_read(int fd, void *buf, uint32_t len);
int  fs_write(int fd, const void *buf, uint32_t len);
void fs_close(int fd);
int  fs_unlink(const char *name);
void fs_list(void);
uint32_t fs_debug_magic(void);

#endif /* FS_H */
