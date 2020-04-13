OUTPUT_ARCH(i386:x86-64)
OUTPUT_FORMAT(elf32-x86-64)

MEMORY {
    /* LD will look for regions in order of definition.
     * Put FSEG on top to try and keep a 64k binary. */
    FSEG_HIGH(rwx) : ORIGIN = 0xffff0000, LENGTH = 64k
    ESEG_HIGH(rwx) : ORIGIN = 0xfffe0000, LENGTH = 64k
}

SECTIONS {
    /* put fixed expection table at the base of E-segment */
    .excp_tbl ORIGIN(ESEG_HIGH) : {
        KEEP(*(.excp_tbl));
    }

    .text : { *(.text) *(.text.*) *(.code) }
    .data : { *(.data) }
    .bss  : { *(.bss) }
    .rodata : { *(.rodata) *(.rodata.*) }

    /* 16-bit segment need to explictly be in an f-segment
     * so that resetvector can near-jump to it */
    .rodata16 : {
        *(.rodata16)
    } > FSEG_HIGH
    .code16 : {
        *(.code16)
    } > FSEG_HIGH
    .code32 : {
        *(.code32)
    } > FSEG_HIGH
    .code64 : {
        *(.code64)
    } > FSEG_HIGH

    /* pad resetvector up to 16 bytes */
    .resetvector16 ORIGIN(FSEG_HIGH) + LENGTH(FSEG_HIGH) - 16 : {
        KEEP(*(.resetvector16));
        . = 16;
    } =0xCC

    /DISCARD/ : { *(.comment) }
}

NOCROSSREFS_TO(.text .code16 .code32);
