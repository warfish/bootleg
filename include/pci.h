#pragma once

#include <inttypes.h>
#include "io.h"

#define PCI_CONFADDR ((uint16_t)0x0CF8)
#define PCI_CONFDATA ((uint16_t)0x0CFC)

static inline uint32_t pci_make_bdf(uint8_t bus, uint8_t dev, uint8_t func)
{
    return 0x80000000 | ((uint32_t)(func & 0x7) << 8) | ((uint32_t)(dev & 0x1F) << 11) | ((uint32_t)bus << 16);
}

static inline uint8_t pci_read8(uint32_t bdf, uint8_t reg)
{
    out32(PCI_CONFADDR, bdf | reg);
    return in32(PCI_CONFDATA) & 0xFF;
}

static inline uint16_t pci_read16(uint32_t bdf, uint8_t reg)
{
    out32(PCI_CONFADDR, bdf | reg);
    return in32(PCI_CONFDATA) & 0xFFFF;
}

static inline uint32_t pci_read32(uint32_t bdf, uint8_t reg)
{
    out32(PCI_CONFADDR, bdf | reg);
    return in32(PCI_CONFDATA);
}

static inline void pci_write32(uint32_t bdf, uint8_t reg, uint32_t val)
{
    out32(PCI_CONFADDR, bdf | reg);
    out32(PCI_CONFDATA, val);
}
