; =============================================================================
; SENG21213-OS :: Kernel Entry Point
; File   : kernel/kernel_entry.asm
; Purpose: Bridges the bootloader (NASM) to the C kernel. Sets up calling
;          conventions then calls kernel_main().
; =============================================================================

[BITS 32]
[EXTERN kernel_main]   ; Defined in kernel.c
[EXTERN __bss_start]   ; Defined in linker.ld
[EXTERN __bss_end]     ; Defined in linker.ld
[GLOBAL _start]

_start:
    ; Zero the .bss section before anything reads/writes uninitialised globals
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    ; The bootloader already set up segments and a stack at 0x90000.
    ; We just call the C kernel main function.
    call kernel_main

    ; If kernel_main ever returns, halt the CPU permanently.
    cli
.halt:
    hlt
    jmp .halt
