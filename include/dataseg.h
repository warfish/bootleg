/**
 * We don't have an RW .data segment (because we always run from ROM).
 * To store RW data that does not need to be deallocated we use a simple grows-up allocator,
 * which is called data segment. This segment is just that: a reserved memory area.
 * Since there is no deallocation involved, we don't need to worry about fragmentation.
 */

#pragma once

/**
 * Init dataseg.
 * Location is specified by build defines: DATASEG_BASE and DATASEG_SIZE.
 */
void init_dataseg(void);

/**
 * Alloc more memory.
 * Allocations from dataseg are either successfull or an abort if no memory is available.
 */
void* dataseg_alloc(size_t size);

/**
 * Scrub entire datasegment and forget it
 */
void dataseg_scrub(void);
