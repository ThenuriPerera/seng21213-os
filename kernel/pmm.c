#include "pmm.h"
#include "vga.h"

#define E820_COUNT_ADDR   0x8000
#define E820_BUFFER_ADDR  0x8004

/* Supports up to 128MB of RAM tracked (32768 frames / 8 = 4096 bytes bitmap).
 * Increase if you plan to test with more than -m 128M. */
#define MAX_FRAMES        32768

static uint8_t frame_bitmap[MAX_FRAMES / 8];
static uint32_t total_frames = 0;
static uint32_t used_frames  = 0;
static uint32_t highest_frame = 0;

static void bitmap_set(uint32_t frame) {
    if (frame >= MAX_FRAMES) return;
    frame_bitmap[frame / 8] |= (1 << (frame % 8));
}

static void bitmap_clear(uint32_t frame) {
    if (frame >= MAX_FRAMES) return;
    frame_bitmap[frame / 8] &= ~(1 << (frame % 8));
}

static int bitmap_test(uint32_t frame) {
    if (frame >= MAX_FRAMES) return 1; /* out of range = treat as used/reserved */
    return frame_bitmap[frame / 8] & (1 << (frame % 8));
}

void pmm_init(void) {
    uint16_t entry_count = *(uint16_t *)E820_COUNT_ADDR;
    e820_entry_t *entries = (e820_entry_t *)E820_BUFFER_ADDR;

    /* Start with everything marked reserved/used */
    for (uint32_t i = 0; i < MAX_FRAMES / 8; i++) frame_bitmap[i] = 0xFF;

    /* Walk the E820 map, free up frames inside usable (type 1) regions */
    for (uint16_t i = 0; i < entry_count; i++) {
        if (entries[i].type != 1) continue;

        uint32_t base = (uint32_t)entries[i].base;
        uint32_t len  = (uint32_t)entries[i].length;
        uint32_t end  = base + len;

        uint32_t first_frame = (base + FRAME_SIZE - 1) / FRAME_SIZE;
        uint32_t last_frame  = end / FRAME_SIZE;

        for (uint32_t f = first_frame; f < last_frame && f < MAX_FRAMES; f++) {
            bitmap_clear(f);
            if (f > highest_frame) highest_frame = f;
        }
    }

    /* Always reserve frame 0 (so 0 can mean "alloc failed") and the first
     * 1MB / kernel load region, regardless of what E820 said was usable. */
    for (uint32_t f = 0; f < 256; f++) bitmap_set(f);

    total_frames = highest_frame + 1;
    if (total_frames > MAX_FRAMES) total_frames = MAX_FRAMES;

    used_frames = 0;
    for (uint32_t f = 0; f < total_frames; f++) {
        if (bitmap_test(f)) used_frames++;
    }

    vga_puts_color("\n  [PMM] Physical memory manager initialised.\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);

    /* --- Temporary self-test: alloc/free 100 frames, confirm no leak --- */
    {
        uint32_t before_used = used_frames;
        uint32_t frames[100];
        for (int i = 0; i < 100; i++) frames[i] = pmm_alloc_frame();
        for (int i = 0; i < 100; i++) pmm_free_frame(frames[i]);
        if (used_frames == before_used) {
            vga_puts_color("  [PMM] Self-test PASSED: alloc/free 100 frames, no leak.\n",
                           VGA_LIGHT_GREEN, VGA_BLACK);
        } else {
            vga_puts_color("  [PMM] Self-test FAILED: frame count leaked.\n",
                           VGA_LIGHT_RED, VGA_BLACK);
        }
    }
}

uint32_t pmm_alloc_frame(void) {
    for (uint32_t f = 0; f < total_frames; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            used_frames++;
            return f * FRAME_SIZE;
        }
    }
    return 0; /* out of memory */
}

void pmm_free_frame(uint32_t paddr) {
    uint32_t frame = paddr / FRAME_SIZE;
    if (frame >= total_frames) return;
    if (bitmap_test(frame)) {
        bitmap_clear(frame);
        used_frames--;
    }
}

void pmm_get_stats(uint32_t *total_kb, uint32_t *used_kb, uint32_t *free_kb) {
    *total_kb = (total_frames * FRAME_SIZE) / 1024;
    *used_kb  = (used_frames * FRAME_SIZE) / 1024;
    *free_kb  = *total_kb - *used_kb;
}
