; =============================================================================
; SENG21213-OS :: Paging Enable
; File   : boot/paging.asm
; Purpose: Loads CR3 with the page directory physical address and sets the
;          PG bit in CR0 to turn on paging. Called from C as:
;              void enable_paging(uint32_t page_dir_phys);
; =============================================================================
[BITS 32]
[GLOBAL enable_paging]

enable_paging:
    mov eax, [esp+4]        ; arg1: page_dir_phys
    mov cr3, eax             ; load page directory base

    mov eax, cr0
    or  eax, 0x80000000      ; set PG bit (bit 31)
    mov cr0, eax

    ret
