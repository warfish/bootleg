;
; bootleg reset vector entry
;

%include "asm/logging.inc"

struc gdt_desc
    .lim_low:   resw 1
    .base_low:  resw 1
    .base_hl:   resb 1
    .access:    resb 1
    .gran:      resb 1
    .base_hh:   resb 1
endstruc

struc idt_desc
    .off_low:   resw 1
    .seg_sel:   resw 1
    .reserved:  resb 1
    .access:    resb 1
    .off_high:  resw 1
endstruc

; make_idt_desc offset32, type
%macro make_idt_desc 2
    istruc idt_desc
        at idt_desc.off_low,    dw %1 & 0x0000FFFF
        at idt_desc.seg_sel,    dw SEL_CODE32
        at idt_desc.reserved,   db 0
        at idt_desc.access,     db 0b10001000 | %2
        at idt_desc.off_high,   dw %1 >> 16
    iend
%endmacro

%macro make_idt_trap 1
    make_idt_desc %1, 0b111
%endmacro

%macro make_idt_intr 1
    make_idt_desc %1, 0b110
%endmacro

struc desc_table_ptr
    .limit:     resw 1
    .base:      resd 1
endstruc

%define SEL_CODE32  0x08
%define SEL_DATA32  0x10
%define SEL_STACK32 0x18

; ------------------------------------------------------------------------------
; Read-only data
section .rodata16

gdt_start:
    ; 0x00
    dq 0
    ; 0x08: CS
    istruc gdt_desc
        at gdt_desc.lim_low,    dw 0xFFFF
        at gdt_desc.base_low,   dw 0
        at gdt_desc.base_hl,    db 0
        at gdt_desc.access,     db 0b10011011
        at gdt_desc.gran,       db 0b11001111
        at gdt_desc.base_hh,    db 0
    iend
    ; 0x10: DS/ES/FG/GS
    istruc gdt_desc
        at gdt_desc.lim_low,    dw 0xFFFF
        at gdt_desc.base_low,   dw 0
        at gdt_desc.base_hl,    db 0
        at gdt_desc.access,     db 0b10010011
        at gdt_desc.gran,       db 0b11001111
        at gdt_desc.base_hh,    db 0
    iend
    ; 0x18: SS
    istruc gdt_desc
        at gdt_desc.lim_low,    dw 0xFFFF
        at gdt_desc.base_low,   dw 0
        at gdt_desc.base_hl,    db 0
        at gdt_desc.access,     db 0b10010011
        at gdt_desc.gran,       db 0b11001111
        at gdt_desc.base_hh,    db 0
    iend
gdt_end:

gdt:
    istruc desc_table_ptr
        at desc_table_ptr.limit,    dw (gdt_end - gdt_start - 1)
        at desc_table_ptr.base,     dd gdt_start
    iend

; Generate 32 idt trap descriptors
; Each descriptor will point to 8-byte aligned trampoline starting at address 0
; Trampoline jump table is generated in a code section
idt_start:
    %define excp_base 0xfffe0000
    %assign i 0
    %rep 32 
        make_idt_trap excp_base + (i << 3)
    %assign i i + 1
    %endrep    
idt_end:

idt:
    istruc desc_table_ptr
        at desc_table_ptr.limit,    dw (idt_end - idt_start - 1)
        at desc_table_ptr.base,     dd idt_start
    iend

bad_bist_str: db 'Bad BIST on startup: ', 0


; ------------------------------------------------------------------------------
; Exception handling

; Exception jump table to jump to relocatable entries
; Located at fixed address (E-seg base)
; IDT points to jmp entries in descriptors (which cannot hold relocatable entries)
section .excp_tbl exec
use32

%macro isr_trampoline 1
    align 8
    jmp     dword isr%1
%endmacro

%assign i 0
%rep 32 
    isr_trampoline i
%assign i i + 1
%endrep    

; Relocatable exception handlers
section .code
use32

extern exception_handler

%macro excp_entry 1
    pushad
    push    esp ; Exception frame pointer
    push    dword %1
    call    exception_handler
    add     esp, 8
    popad
    add     esp, 4
    iret
%endmacro

%macro excp_entry_no_error 1
    ; Push fake error code to align with normal frame pointer
    push    dword 0
    excp_entry %1
%endmacro

isr0: excp_entry_no_error 0
isr1: excp_entry_no_error 1
isr2: excp_entry_no_error 2
isr3: excp_entry_no_error 3
isr4: excp_entry_no_error 4
isr5: excp_entry_no_error 5
isr6: excp_entry_no_error 6
isr7: excp_entry_no_error 7
isr8: excp_entry 8
isr9: excp_entry_no_error 9
isr10: excp_entry 10
isr11: excp_entry 11
isr12: excp_entry 12
isr13: excp_entry 13
isr14: excp_entry 14
isr16: excp_entry_no_error 16
isr17: excp_entry 17
isr18: excp_entry_no_error 18
isr19: excp_entry_no_error 19
isr20: excp_entry_no_error 20
isr21: excp_entry 21
; 15, 22-31: reserved
isr15:
isr22:
isr23:
isr24:
isr25:
isr26:
isr27:
isr28:
isr29:
isr30:
isr31:

global abort
abort: ud2


; ------------------------------------------------------------------------------
; Reset vector entry point and PM init
; * Maskable interrupts disabled
; * NMIs enabled.
;   NMIs happen either if watchdog delivers interrupts as NMI through APIC (and we didn't program it yet),
;   or if hardware is failing (and then we're fucked anyway).
section .code16 exec
use16

extern _start
extern _code32flat_start

init16:

%define lgdtl o32 lgdt
%define lidtl o32 lidt

    ; Check BIST status
    test    eax, eax
    jnz     .bad_bist
    
    ; Point data segment to low bios mapping
    mov     ax, 0xf000
    mov     ds, ax

    ; enable A20
    in      al, 0x92
    or      al, 2
    out     0x92, al

    ; Load gdt and switch to pm
    lgdtl   [gdt]
    lidtl   [idt]
    mov     eax, 0x40000001 
    mov     cr0, eax

    ; Switch to PM code segment
    jmp     0x08:dword .enter_pm

.bad_bist:
    mov     ebx, eax
    mov     si, bad_bist_str
    _puts
    _putdw  ebx
    _putline
    hlt

.enter_pm:
use32
    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ax, 0x18
    mov     ss, ax

    ; Set stack to low 4k memory
    mov     esp, 0x1000

    ; Call C entry point
    ; TODO: report exit status to fw_cfg?
    call    _start
    hlt

section .resetvector16 exec
use16
    jmp     init16
    hlt
