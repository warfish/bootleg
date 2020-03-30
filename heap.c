#include <inttypes.h>
#include <assert.h>
#include <string.h>

#include "heap.h"
#include "dataseg.h"
#include "logging.h"

/**
 * Alloc arena, stored in dataseg, becase we don't ever free them.
 */
struct alloc_arena {
    uintptr_t blocks;
    uint32_t nblocks;
    uint8_t order;

    /* Set bit means block is free */
    uint8_t bitmap[/* nblocks >> 8 + 1 for remainder */];

    /* blocks */
} __attribute__((packed));

static inline uint32_t bsf(uint32_t val)
{
    uint32_t res;
    __asm__ volatile ("bsf %1, %0" :"=r"(res) :"rm"(val) :);
    return res;
}

static inline uint32_t bsr(uint32_t val)
{
    uint32_t res;
    __asm__ volatile ("bsr %1, %0" :"=r"(res) :"rm"(val) :);
    return res;
}

static inline size_t arena_bitmap_size(size_t nblocks)
{
    return (nblocks >> 3) + ((nblocks & 0x7) != 0);
}

static inline void* arena_blocks_ptr(struct alloc_arena* arena)
{
    return (void*) arena->blocks;
}

static void* arena_alloc_block(struct alloc_arena* arena, uint32_t block)
{
    assert(block < arena->nblocks);

    LOG_DEBUG("arena %u: allocate block %u\n", arena->order, block);

    arena->bitmap[block >> 3] &= ~((uint8_t)1 << (block & 0x7));
    return arena_blocks_ptr(arena) + (block << arena->order);
}

static void arena_free_block(struct alloc_arena* arena, uint32_t block)
{
    assert(block < arena->nblocks);

    LOG_DEBUG("arena %u: free block %u\n", arena->order, block);

    arena->bitmap[block >> 3] |= (uint8_t)1 << (block & 0x7);
}

static void* arena_alloc(struct alloc_arena* arena, size_t size)
{
    assert(size <= (1ul << arena->order));

    uint32_t block = 0;

    /* Number of 32-bit bitmap chunks we can traverse */
    uint32_t nchunks = arena->nblocks / 32;
    uint32_t nrem = arena->nblocks - nchunks * 32;
    uint32_t* pchunk = (uint32_t*)arena->bitmap;

    while (nchunks-- > 0) {
        if (*pchunk == 0) {
            block += 32;
            pchunk++;
            continue;
        }

        return arena_alloc_block(arena, block + bsf(*pchunk));
    }

    if (nrem) {
        /* Setting this to 0 also marks any overflown bytes as "occupied blocks" */
        uint32_t chunk = 0;
        memcpy(&chunk, pchunk, arena_bitmap_size(nrem));
        if (chunk != 0) {
            return arena_alloc_block(arena, block + bsf(chunk));
        }
    }

    return NULL;
}

static void arena_free(struct alloc_arena* arena, void* ptr)
{
    uint32_t block = (ptr - arena_blocks_ptr(arena)) >> arena->order;
    if (block < arena->nblocks) {
        arena_free_block(arena, block);
    }
}

static struct alloc_arena* init_arena(uintptr_t base, size_t size, uint8_t order)
{
    size_t nblocks = size >> order;
    if (nblocks == 0) {
        return NULL;
    }

    size_t bitmap_size = arena_bitmap_size(nblocks);
    assert(bitmap_size < size);

    /* Arena headers (including bitmap) are sitting in dataseg */
    struct alloc_arena* arena = dataseg_alloc(sizeof(*arena) + bitmap_size);
    arena->blocks = base;
    arena->nblocks = nblocks;
    arena->order = order;

    memset(arena->bitmap, 0xFF, bitmap_size);

    return arena;
}

/**
 * Heap defines arenas for given orders of allocations
 * and holds an arena lookup table in dataseg
 */

#include "datamap.h"

/** We start at 2^6 order and move to 2^10 */
#define HEAP_ORDER_BASE 6
#define HEAP_ORDER_MAX  10

#if !defined(HEAP_LOOKUP_PTR_ADDR)
#   error HEAP_LOOKUP_PTR_ADDR should be defined
#endif

#if !defined(HEAP_BASE)
#   error HEAP_BASE should be defined
#endif

#if !defined(HEAP_SIZE)
#   error HEAP_SIZE should be defined
#endif

/** Type of a pointer to arena lookup table */
typedef struct alloc_arena* (*heap_lookup_ptr)[HEAP_ORDER_MAX - HEAP_ORDER_BASE + 1];

/** Macro that expands to fixed address of a heap lookup pointer in data segment */
#define HEAP_LOOKUP_PTR (*(heap_lookup_ptr*)HEAP_LOOKUP_PTR_ADDR)
#define HEAP_LOOKUP_TABLE (*HEAP_LOOKUP_PTR)

struct heap_header {
    uint32_t order;
};

static inline void dump_arena(struct alloc_arena* arena)
{
    LOG_DEBUG("arena at %x: order %hhx, nblocks %u, bitmap size %u\n",
        arena, arena->order, arena->nblocks, arena_bitmap_size(arena->nblocks));
}

void init_heap(void)
{
    uintptr_t base = HEAP_BASE;
    size_t avail = HEAP_SIZE;

    HEAP_LOOKUP_PTR = dataseg_alloc(sizeof(HEAP_LOOKUP_TABLE));
    assert(HEAP_LOOKUP_PTR);

    /* Half the space is cosumed by 64-byte allocations (which are most common for us) */
    avail >>= 1;
    HEAP_LOOKUP_TABLE[0] = init_arena(base, avail, HEAP_ORDER_BASE);

    /* Remaining space is separated equally between all other orders up to 1K */
    for (unsigned i = HEAP_ORDER_BASE + 1; i <= HEAP_ORDER_MAX; ++i) {
        HEAP_LOOKUP_TABLE[i - HEAP_ORDER_BASE] =
            init_arena(base, avail / (HEAP_ORDER_MAX - HEAP_ORDER_BASE), i);
    }
}

void* heap_alloc(size_t size)
{
    /* Just to be on the safe side, check that our HEAP_ORDER_MAX is withing reasonable limits
     * so we don't need to worry about overflows so much */
    _Static_assert(HEAP_ORDER_MAX < 31, "HEAP_ORDER_MAX is way too large");

    /* We don't support allocations larger than our max base */
    if (size >= (1ul << HEAP_ORDER_MAX)) {
        return NULL;
    }

    size += sizeof(struct heap_header);
    LOG_DEBUG("heap_alloc: full size %u\n", size);

    /* Find next log2 order for allocation size */
    uint32_t order = bsr((size - 1) << 1);
    if (order > HEAP_ORDER_MAX) {
        return NULL;
    } else if (order < HEAP_ORDER_BASE) {
        order = HEAP_ORDER_BASE;
    }

    struct alloc_arena* arena = (*HEAP_LOOKUP_PTR)[order - HEAP_ORDER_BASE];
    dump_arena(arena);
    assert(arena->order == order);

    void* ptr = arena_alloc(arena, size); 
    if (!ptr) {
        return NULL;
    }

    struct heap_header* header = ptr;
    header->order = order;

    ptr += sizeof(*header);
    assert(((uintptr_t)ptr & (HEAP_PTR_ALIGNMENT - 1)) == 0);
    return ptr;
}

void heap_free(void* ptr)
{
    if (!ptr) {
        return;
    }

    struct heap_header* header = ptr - sizeof(*header);
    assert(header->order >= HEAP_ORDER_BASE && header->order <= HEAP_ORDER_MAX);

    struct alloc_arena* arena = (*HEAP_LOOKUP_PTR)[header->order - HEAP_ORDER_BASE];
    dump_arena(arena);
    assert(arena->order == header->order);

    arena_free(arena, header);
}
