;
; bootleg reset vector entry
;

%include "asm/logging.inc"

; Segment layout
%define FSEG_BASE 0xffff0000
%define ESEG_BASE 0xfffe0000

; GDT selectors
%define SEL_CODE32  0x08
%define SEL_CODE64  0x18
%define SEL_DATA    0x10

; Stack page base
%define STACK_BASE  0x1000

; Base address for exception handler trampolines
%define EXCP_TABLE_BASE ESEG_BASE

; Work around ELF 32-bit relocations for 16-bit code to make sure R_X86_64_16 will fit
; This is because our linker script currently uses high memory addresses
%define FSEGREL16(var) var - 0xffff0000

; Base address for PML4 page tables
%define PAGE_SIZE   0x1000
%define PML4_BASE   0x100000
%define PDPE_BASE   PML4_BASE + PAGE_SIZE
%define PDE_BASE    PDPE_BASE + PAGE_SIZE
%define PTE_BASE    PDE_BASE + PAGE_SIZE * 4

struc gdt_desc
    .lim_low:   resw 1
    .base_low:  resw 1
    .base_hl:   resb 1
    .access:    resb 1
    .gran:      resb 1
    .base_hh:   resb 1
endstruc

struc idt_desc32
    .off_low:   resw 1
    .seg_sel:   resw 1
    .reserved:  resb 1
    .type:      resb 1
    .off_high:  resw 1
endstruc

struc idt_desc64
    .off32_low:     resw 1
    .seg_sel:       resw 1
    .ist:           resb 1
    .type:          resb 1
    .off32_high:    resw 1
    .off64_high:    resd 1
    .reserved:      resd 1
endstruc

; make_idt_desc32 offset32, type
%macro make_idt_desc32 2
    istruc idt_desc32
        at idt_desc32.off_low,    dw %1 & 0x0000FFFF
        at idt_desc32.seg_sel,    dw SEL_CODE32
        at idt_desc32.reserved,   db 0
        at idt_desc32.type,       db 0b10001000 | %2
        at idt_desc32.off_high,   dw %1 >> 16
    iend
%endmacro

; make_idt_desc64 offset64, type
%macro make_idt_desc64 2
    istruc idt_desc64
        at idt_desc64.off32_low,  dw %1 & 0x0000FFFF
        at idt_desc64.seg_sel,    dw SEL_CODE64
        at idt_desc64.ist,        db 0
        at idt_desc64.type,       db 0b10000000 | %2
        at idt_desc64.off32_high, dw %1 >> 16
        at idt_desc64.off64_high, dd %1 >> 32
        at idt_desc64.reserved,   dd 0
    iend
%endmacro

%macro make_idt_trap32 1
    make_idt_desc32 %1, 0b1111
%endmacro

%macro make_idt_trap64 1
    make_idt_desc64 %1, 0b1111
%endmacro

%macro make_idt_intr32 1
    make_idt_desc32 %1, 0b1110
%endmacro

%macro make_idt_intr64 1
    make_idt_desc64 %1, 0b1110
%endmacro

struc desc_table_ptr32
    .limit:     resw 1
    .base:      resd 1
endstruc

struc desc_table_ptr64
    .limit:     resw 1
    .base:      resq 1
endstruc

; ------------------------------------------------------------------------------
; Read-only data
section .rodata16

gdt_start:
    ; 0x00
    dq 0
    ; 0x08: CS for 32-bit protected mode
    istruc gdt_desc
        at gdt_desc.lim_low,    dw 0xFFFF
        at gdt_desc.base_low,   dw 0
        at gdt_desc.base_hl,    db 0
        at gdt_desc.access,     db 0b10011111
        at gdt_desc.gran,       db 0b11001111
        at gdt_desc.base_hh,    db 0
    iend
    ; 0x10: DS/ES/FG/GS/SS for all modes
    istruc gdt_desc
        at gdt_desc.lim_low,    dw 0xFFFF
        at gdt_desc.base_low,   dw 0
        at gdt_desc.base_hl,    db 0
        at gdt_desc.access,     db 0b10010011
        at gdt_desc.gran,       db 0b11001111
        at gdt_desc.base_hh,    db 0
    iend
    ; 0x18: CS in 64-bit mode
    istruc gdt_desc
        at gdt_desc.lim_low,    dw 0xFFFF
        at gdt_desc.base_low,   dw 0
        at gdt_desc.base_hl,    db 0
        at gdt_desc.access,     db 0b10011111
        at gdt_desc.gran,       db 0b10101111
        at gdt_desc.base_hh,    db 0
    iend
gdt_end:

gdt32:
    istruc desc_table_ptr32
        at desc_table_ptr32.limit,    dw (gdt_end - gdt_start - 1)
        at desc_table_ptr32.base,     dd gdt_start
    iend

gdt64:
    istruc desc_table_ptr64
        at desc_table_ptr64.limit,    dw (gdt_end - gdt_start - 1)
        at desc_table_ptr64.base,     dq gdt_start
    iend

; Generate 32-bit idt trap descriptors
; Each descriptor will point to 8-byte aligned trampoline starting at address 0
; Trampoline jump table is generated in a code section
idt32_start:
    %assign i 0
    %rep 32 
        make_idt_trap32 EXCP_TABLE_BASE + (i << 3)
    %assign i i + 1
    %endrep    
idt32_end:

idt32:
    istruc desc_table_ptr32
        at desc_table_ptr32.limit,    dw (idt32_end - idt32_start - 1)
        at desc_table_ptr32.base,     dd idt32_start
    iend

; Generate 64-bit idt trap descriptors
; Each descriptor points to the same trampoline as for 32-bit case
idt64_start:
    %assign i 0
    %rep 32
        make_idt_trap64 EXCP_TABLE_BASE + (i << 3)
    %assign i i + 1
    %endrep
idt64_end:

idt64:
    istruc desc_table_ptr64
        at desc_table_ptr64.limit,    dw (idt64_end - idt64_start - 1)
        at desc_table_ptr64.base,     dq idt64_start
    iend

bad_bist_str: db 'Bad BIST on startup: ', 0


; ------------------------------------------------------------------------------
; Exception handling

; Exception jump table to jump to relocatable entries
; Located at fixed address (E-seg base)
; IDT points to jmp entries in descriptors (which cannot hold relocatable entries)
; Both 32 and 64-bit IDTs use the same trampolines below
section .excp_tbl exec
use32

%macro isr_trampoline 1
    align 8

    ; Encode jump as near 32-bit relative, which will work in both 32 and 64 bit modes
    jmp     dword isr%1
%endmacro

%assign i 0
%rep 32 
    isr_trampoline i
%assign i i + 1
%endrep

; Relocatable exception handlers
section .code32 exec
use32

; C generic entry for exceptions
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
isr31: ud2


; ------------------------------------------------------------------------------
; Reset vector entry point and PM init
; * Maskable interrupts disabled
; * NMIs enabled.
;   NMIs happen either if watchdog delivers interrupts as NMI through APIC (and we didn't program it yet),
;   or if hardware is failing (and then we're fucked anyway).
section .code16 exec
use16

%define lgdtl o32 lgdt
%define lidtl o32 lidt

init16:
    ; Check BIST status
    test    eax, eax
    jz      .enter_pm
    mov     ebx, eax
    mov     si, FSEGREL16(bad_bist_str)
    _puts
    _putdw  ebx
    _putline
    hlt
    
.enter_pm:
    ; Point data segment to low bios mapping
    mov     ax, 0xf000
    mov     ds, ax

    ; enable A20
    in      al, 0x92
    or      al, 2
    out     0x92, al

    ; Load gdt and switch to pm
    lgdtl   [FSEGREL16(gdt32)]
    lidtl   [FSEGREL16(idt32)]
    mov     eax, 0x40000001 
    mov     cr0, eax

    ; Switch to PM code segment
    jmp     SEL_CODE32:dword .now_in_protected_mode

.now_in_protected_mode:
use32
    mov     ax, SEL_DATA
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    ; Set stack to low 4k memory
    mov     esp, STACK_BASE

    ;
    ; Prepare for LME
    ;

    ; CR4.PAE = 1, CR4.PGE = 1
    mov     eax, 0x00000020
    mov     cr4, eax
    
    ;
    ; Setup 4GB identity mapped page tables
    ;
    
    ; Setup PML4E table page (with 1 entry)
    mov     ebx, PML4_BASE

    xor     eax, eax
    mov     ecx, PAGE_SIZE
    mov     edi, PML4_BASE
    rep     stosb

    mov     eax, PDPE_BASE | 0b00000011
    mov     [ebx], eax
    mov     [ebx + 4], dword 0
    add     ebx, PAGE_SIZE

    ; Setup PDPE table page (with 4 entries)
    xor     eax, eax
    mov     edi, PDPE_BASE
    mov     ecx, PAGE_SIZE
    rep     stosb

    mov     ebx, PDPE_BASE
    mov     eax, PDE_BASE | 0b00000001
    mov     [ebx], eax
    mov     [ebx + 4], dword 0
    add     eax, PAGE_SIZE
    mov     [ebx + 8], eax
    mov     [ebx + 12], dword 0
    add     eax, PAGE_SIZE
    mov     [ebx + 16], eax
    mov     [ebx + 20], dword 0
    add     eax, PAGE_SIZE
    mov     [ebx + 24], eax
    mov     [ebx + 28], dword 0

    ; Setup 4 PDE table pages
    mov     ebx, PDE_BASE
    mov     eax, PTE_BASE | 0b00000011
    mov     ecx, 512 * 4
.pde_setup_loop:
    mov     [ebx], eax
    mov     [ebx + 4], dword 0
    add     eax, PAGE_SIZE
    add     ebx, 8
    dec     ecx
    cmp     ecx, 0
    jne     .pde_setup_loop

    ; Setup 8MB worth of PTEs
    mov     ebx, PTE_BASE
    xor     eax, eax
    or      eax, 0b00000011
    mov     ecx, (1 << 20)
.pte_setup_loop:
    mov     [ebx], eax
    mov     [ebx + 4], dword 0
    add     eax, PAGE_SIZE
    add     ebx, 8
    dec     ecx
    cmp     ecx, 0
    jne     .pte_setup_loop
    
    ; Point CR3 to PML4
    mov     eax, PML4_BASE
    mov     cr3, eax

    ; Enable long mode
    mov     ecx, 0xC0000080
    rdmsr
    or      eax, 0x100
    wrmsr

    ; Activate paging, which will transition us to compatibility mode
    mov     eax, cr0
    or      eax, 0x80000000
    mov     cr0, eax

.now_in_compatibility_mode:
    ; Jump to 64-bit code segment to transition
    jmp SEL_CODE64:dword .now_in_64bit_mode

section .code64 exec
use64
extern _start

.now_in_64bit_mode:
    ; Initialize 64bit GDTR, IDTR and RSP
    ; Note: lidt/lgdt a32 address override prefix is used so that addresses will _not_ be sign-extended
    mov     rsp, qword STACK_BASE
    lidt    [a32 idt64]
    lgdt    [a32 gdt64]

    ; Validate that we're in long mode now (EFER.LMA == 1)
    mov     ecx, 0xC0000080
    rdmsr
    and     eax, (1 << 10)
    jz      abort

    ; Call C entry point
    call    _start

    ; TODO: report exit status to fw_cfg?
    hlt

global abort
abort: ud2

section .resetvector16 exec
use16
    jmp     init16
    hlt
