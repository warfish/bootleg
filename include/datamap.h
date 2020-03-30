/**
 * Static layout of data segment, where we store our RW data
 */

#pragma once

/**
 * D-seg
 */

/** see include/dataseg.h */
#define DATASEG_BASE 0x000D0000ul
#define DATASEG_SIZE 0x1000ul

/** heap arena lookup table pointer */
#define HEAP_LOOKUP_PTR_ADDR (DATASEG_BASE + DATASEG_SIZE)

/**
 * C-seg
 */

#define HEAP_BASE 0x000C0000ul
#define HEAP_SIZE (64ul << 10)
