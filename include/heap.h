#pragma once

/**
 * Init heap.
 * Heap base and size is defined in datamap.
 */
void init_heap(void);

/**
 * Allocate stuff from the heap
 */
void* heap_alloc(size_t size);
void heap_free(void* ptr);
