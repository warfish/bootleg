%define DEBUGCON_IOPORT 0x402

; Dump newline character(s)
; @clobbers    al, dx
%macro _putline 0
    mov     dx, DEBUGCON_IOPORT
    mov     al, 0x0D
    out     dx, al
    mov     al, 0x0A
    out     dx, al
%endmacro

; Dump ASCII character
; @clobbers     al, dx
%macro _putc 1
    mov     dx, DEBUGCON_IOPORT
    mov     al, %1   
    out     dx, al
%endmacro

; Dump ASCII string
; @cs:si    String pointer
; @clobbers al, si, dx
%macro _puts 0
    mov     dx, DEBUGCON_IOPORT
    %%.iter:
    mov     al, [cs:si]
    test    al, al
    jz      %%.end
    out     dx, al
    inc si
    jmp     %%.iter
    %%.end:
%endmacro

; Convert 1 nibble to a hex character code
; @al           Nibble
; @clobber      al
%macro _nib_to_char 0
    and     al, 0x0f
    add     al, 0x30
    cmp     al, 0x3A
    jb      %%.done
    add     al, 0x7
    %%.done:
%endmacro

; Dump hexadecimal byte
; @al           byte
; @clobber      al, bl, dx
%macro _putb 0
    mov     dx, DEBUGCON_IOPORT
    mov     bl, al
    shr     al, 4
    _nib_to_char
    out     dx, al

    mov     al, bl
    _nib_to_char
    out     dx, al
%endmacro

%macro _putb 1
    mov     al, %1
    _putb
%endmacro

; Dump a 16-bit hex value
; @ax           16-bit value to output
; @clobbers     ax, bl, cl, dx
%macro _putw 0
    mov     cl, 2
    xchg    al, ah
    %%.iter:
    _putb
    shr     ax, 8
    dec     cl
    test    cl, cl
    jnz     %%.iter
%endmacro

%macro _putw 1
    mov     ax, %1
    _putw
%endmacro

; Dump a 32-bit hex value
; @eax          32-bit value to output
; @clobbers     eax, bl, cl, dx
%macro _putdw 0
    mov     cl, 4
    bswap   eax
    %%.iter:
    _putb
    shr     eax, 8
    dec     cl
    test    cl, cl
    jnz     %%.iter
%endmacro

%macro _putdw 1
    mov     eax, %1
    _putdw
%endmacro
