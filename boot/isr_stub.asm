; =============================================================================
; SENG21213-OS :: IRQ0 (timer) interrupt service routine stub
; File   : boot/isr_stub.asm
; Purpose: Landing point when IRQ0 fires. Saves registers, calls the C-level
;          handler, restores registers, and returns via IRET.
; =============================================================================
[BITS 32]
[GLOBAL irq0_stub]
[EXTERN irq0_handler]   ; Defined in kernel/scheduler.c

irq0_stub:
    pushad                  ; save all general-purpose registers
    call irq0_handler       ; call the C handler (increments tick, may switch)
    popad                   ; restore all general-purpose registers
    iret                    ; special interrupt return (pops CS, EIP, EFLAGS)
