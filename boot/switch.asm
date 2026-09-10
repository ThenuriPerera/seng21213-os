; =============================================================================
; SENG21213-OS :: Context Switch
; File   : boot/switch.asm
; Purpose: Saves the current process's registers onto its stack, switches to
;          the new process's stack, restores its registers, and returns into
;          it. Called from C as:
;              void context_switch(uint32_t *old_sp, uint32_t new_sp);
; =============================================================================
[BITS 32]
[GLOBAL context_switch]

context_switch:
    pushad                  ; save all registers of the OLD process

    mov eax, [esp+36]       ; arg1: old_sp  (32 bytes pushad + 4 bytes ret addr)
    mov [eax], esp          ; *old_sp = current esp

    mov eax, [esp+40]       ; arg2: new_sp
    mov esp, eax            ; switch stacks

    popad                   ; restore all registers of the NEW process
    sti                     ; always ensure interrupts are enabled after a switch
    ret                     ; jump to whatever return address is on the new stack
