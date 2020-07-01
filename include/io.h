#pragma once

#include <inttypes.h>

static inline void out32(uint16_t port, uint32_t val)
{
    __asm__ volatile("out %%eax, %%dx" ::"a"(val), "d"(port):);
}

static inline uint32_t in32(uint16_t port)
{
    uint32_t res;
    __asm__ volatile("in %%dx, %%eax" :"=a"(res) :"d"(port):);
    return res;
}

static inline uint64_t rdmsr(uint32_t reg)
{
    uint64_t res;
    __asm__ volatile("rdmsr" :"=a"(res) :"c"(reg) :);
    return res;
}
