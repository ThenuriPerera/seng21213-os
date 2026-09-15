#ifndef PMM_H
#define PMM_H

#include "../include/types.h"

#define FRAME_SIZE 4096

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_ext;
} __attribute__((packed)) e820_entry_t;

void pmm_init(void);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t paddr);
void pmm_get_stats(uint32_t *total_kb, uint32_t *used_kb, uint32_t *free_kb);

#endif
