#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>

#include "pci.h"
#include "dataseg.h"
#include "logging.h"
#include "heap.h"
#include "apic.h"

void* memset(void* s, int c, size_t n)
{
    /* We just assume we have ERMSB */
    __asm__ volatile ("rep stosb" : :"D"(s), "c"(n), "a"((uint8_t)c) :"memory");
    return s;
}

void* memcpy(void* dest, const void* src, size_t n)
{
    uint32_t* pdest32 = (uint32_t*)dest;
    const uint32_t* psrc32 = (const uint32_t*)src;

    for (size_t i = 0; i < (n >> 2); ++i) {
        *pdest32++ = *psrc32++;
    }

    for (size_t i = 0; i < (n & 3); ++i) {
        *(uint8_t*)pdest32++ = *(uint8_t*)psrc32++;
    }

    return dest;
}

size_t strlen(const char* s)
{
    size_t len = 0;
    while (*s++ != '\0') {
        ++len;
    }

    return len;
}

void _assert(const char* file, unsigned long line, const char* reason)
{
    LOG_ERROR("Assertion \"%s\" failed at file %s, line %u\n", reason, file, line);
    abort();
}


/**
 * Exception frame record.
 * Generated by assembly code, order of fields is important.
 */
struct exception_frame32 {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t error_code;
    uint32_t eip;
    uint16_t cs;
    uint16_t _pad;
    uint32_t eflags;
};

static uint32_t read_cr0(void)
{
    uint32_t res;
    __asm__ volatile ("mov %%cr0, %%eax":"=a"(res)::);
    return res;
}

static uint32_t read_cr2(void)
{
    uint32_t res;
    __asm__ volatile ("mov %%cr2, %%eax":"=a"(res)::);
    return res;
}

static uint32_t read_cr3(void)
{
    uint32_t res;
    __asm__ volatile ("mov %%cr3, %%eax":"=a"(res)::);
    return res;
}

static uint32_t read_cr4(void)
{
    uint32_t res;
    __asm__ volatile ("mov %%cr4, %%eax":"=a"(res)::);
    return res;
}

void exception_handler(unsigned long num, struct exception_frame32* frame)
{
    LOG_ERROR("Exception 0x%hx\n", num);
    LOG_ERROR("CS: %hx\n", frame->cs);
    LOG_ERROR("EIP: %x\n", frame->eip);
    LOG_ERROR("EFLAGS: %x\n", frame->eflags);
    LOG_ERROR("Error code: %x\n", frame->error_code);
    LOG_ERROR("EAX: 0x%x\n", frame->eax);
    LOG_ERROR("EBX: 0x%x\n", frame->ebx);
    LOG_ERROR("ECX: 0x%x\n", frame->ecx);
    LOG_ERROR("EDX: 0x%x\n", frame->edx);
    LOG_ERROR("EDI: 0x%x\n", frame->edi);
    LOG_ERROR("ESI: 0x%x\n", frame->esi);
    LOG_ERROR("EBP: 0x%x\n", frame->ebp);
    LOG_ERROR("ESP: 0x%x\n", frame->esp);
    LOG_ERROR("CR0: 0x%x\n", read_cr0());
    LOG_ERROR("CR2: 0x%x\n", read_cr2());
    LOG_ERROR("CR3: 0x%x\n", read_cr3());
    LOG_ERROR("CR4: 0x%x\n", read_cr4());
}

static void enable_low_ram(void)
{
    uint32_t i440fx = pci_make_bdf(0, 0, 0);
    uint32_t pam02 = pci_read32(i440fx, 0x58);
    uint32_t pam36 = pci_read32(i440fx, 0x5C);

    pam02 |= 0x33333000; /* PAM0 starts at 0x59, avoid touching 0x58 */
    pam36 |= 0x33333333;

    pci_write32(i440fx, 0x58, pam02);
    pci_write32(i440fx, 0x5C, pam36);

    pam02 = pci_read32(i440fx, 0x58);
    pam36 = pci_read32(i440fx, 0x5C);

#ifdef DEBUG
    for (uint32_t* ptr = 0x000E0000; ptr < 0x000F0000; ++ptr) {
        *ptr = 0xEEEEEEEE;
    }

    for (uint32_t* ptr = 0x000F0000; ptr < 0x00100000; ++ptr) {
        *ptr = 0xFFFFFFFF;
    }
#endif
}

int main(void)
{
    enable_low_ram();
    LOG_DEBUG("low mem enabled at 0x%llx\n", 0x000E0000ull);

    init_dataseg();
    init_heap();

    init_apic();

    return 0;
}

int _start(void)
{
    return main();
}
