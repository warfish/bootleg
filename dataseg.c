#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "dataseg.h"
#include "datamap.h"

#if !defined(DATASEG_BASE)
#   error DATASEG_BASE must be defined
#endif

#if !defined(DATASEG_SIZE)
#   error DATASEG_SIZE must be defined
#endif

_Static_assert((DATASEG_BASE & (HEAP_PTR_ALIGNMENT - 1)) == 0, "Bad dataseg alignment");
_Static_assert(DATASEG_SIZE > sizeof(uintptr_t), "Dataseg size too small");

static void* alloc_at(size_t offset, size_t size)
{
    uintptr_t ptr = DATASEG_BASE + offset;

    offset += (size + (HEAP_PTR_ALIGNMENT - 1)) & ~(HEAP_PTR_ALIGNMENT - 1);
    if (offset >= DATASEG_SIZE) {
        abort();
    }

    /* assert that new offset is still properly aligned */
    assert((offset & (HEAP_PTR_ALIGNMENT - 1)) == 0);
    *(uintptr_t*)DATASEG_BASE = offset;

    return (void*)ptr;
}

void init_dataseg(void)
{
    /* Dataseg base contains current offset, make an initial allocation to store it there */
    (void) alloc_at(0, sizeof(uintptr_t));
}


void* dataseg_alloc(size_t size)
{
    /* TODO: add debug tracing */
    return alloc_at(*(uintptr_t*)DATASEG_BASE, size);
}

void dataseg_scrub(void)
{
    bzero((void*)DATASEG_BASE, DATASEG_SIZE);
}
