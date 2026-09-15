#ifndef VMM_H
#define VMM_H

#include "../include/types.h"

#define PAGE_PRESENT 0x1
#define PAGE_WRITE   0x2

void vmm_init(void);
void vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags);

#endif
