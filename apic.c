#include <inttypes.h>

#include "io.h"
#include "logging.h"

#define IA32_APIC_BASE 0x0000001B

void init_apic(void)
{
    uint64_t apic_base = rdmsr(IA32_APIC_BASE);
    LOG_DEBUG("apic base = 0x%llx", apic_base);

    memset(0xFFFF0000, 1, 0);
}
