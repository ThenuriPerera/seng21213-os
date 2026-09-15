#include "vmm.h"
#include "pmm.h"
#include "vga.h"

#define ENTRIES 1024

__attribute__((aligned(4096))) static uint32_t page_directory[ENTRIES];
__attribute__((aligned(4096))) static uint32_t first_page_table[ENTRIES];

extern void enable_paging(uint32_t page_dir_phys);

void vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_index = virt >> 22;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    if (!(page_directory[pd_index] & PAGE_PRESENT)) {
        uint32_t new_table_phys = pmm_alloc_frame();
        page_directory[pd_index] = new_table_phys | PAGE_PRESENT | PAGE_WRITE;
        uint32_t *table = (uint32_t *)new_table_phys;
        for (int i = 0; i < ENTRIES; i++) table[i] = 0;
    }

    uint32_t *table = (uint32_t *)(page_directory[pd_index] & ~0xFFF);
    table[pt_index] = (phys & ~0xFFF) | flags;
}

void vmm_init(void) {
    for (int i = 0; i < ENTRIES; i++) {
        page_directory[i] = 0;
        first_page_table[i] = (i * FRAME_SIZE) | PAGE_PRESENT | PAGE_WRITE;
    }
    page_directory[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_WRITE;

    /* Identity-map 0x400000 - 0xFFFFFF as well, so anything up to 16MB works */
    for (uint32_t addr = 0x400000; addr < 0x1000000; addr += FRAME_SIZE) {
        vmm_map_page(addr, addr, PAGE_PRESENT | PAGE_WRITE);
    }

    vga_puts_color("  [VMM] Page tables built, enabling paging...\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);

    enable_paging((uint32_t)page_directory);

    vga_puts_color("  [VMM] Paging enabled successfully.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);
}
