#ifndef PCI_H
#define PCI_H

#include <stdint.h>

uint32_t pci_config_read32(uint8_t addr);
void pci_config_write8(uint8_t addr, uint8_t val);
void pci_config_write16(uint8_t addr, uint16_t val);
void pci_config_write32(uint8_t addr, uint32_t val);

uint8_t pci_io_read8(uint32_t addr);
uint16_t pci_io_read16(uint32_t addr);
uint32_t pci_io_read32(uint32_t addr);
void pci_io_write8(uint32_t addr, uint8_t val);
void pci_io_write16(uint32_t addr, uint16_t val);
void pci_io_write32(uint32_t addr, uint32_t val);

uint8_t pci_mem_read8(uint32_t addr);
uint16_t pci_mem_read16(uint32_t addr);
uint32_t pci_mem_read32(uint32_t addr);
void pci_mem_write8(uint32_t addr, uint8_t val);
void pci_mem_write16(uint32_t addr, uint16_t val);
void pci_mem_write32(uint32_t addr, uint32_t val);

#endif
