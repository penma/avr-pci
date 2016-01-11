#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "console.h"
#include "master_transaction.h"
#include "pci.h"
#include "pci/commands.h"
#include "pci/signals.h"
#include "pci/panic.h"

/* TODO: alignment sanity checks */

static void _unimplemented() {
	panic("unimplemented");
}

static void _not_32_aligned() {
	panic("memory address not 32 bit aligned");
}

static uint8_t pci_read8(uint32_t addr, uint8_t cmd) {
	uint8_t mask;
	uint8_t shift;

	switch (addr & 0b11) {
	case 0b00: mask = 0b1110; shift =  0; break;
	case 0b01: mask = 0b1101; shift =  8; break;
	case 0b10: mask = 0b1011; shift = 16; break;
	case 0b11: mask = 0b0111; shift = 24; break;
	}

	return (master_read(addr & ~0b11, cmd, mask) >> shift) & 0xff;
}

static uint16_t pci_read16(uint32_t addr, uint8_t cmd) {
	if ((addr & 0b11) == 0b00) {
		return master_read(addr, cmd, 0b1100);
	} else if ((addr & 0b11) == 0b10) {
		return (master_read(addr & ~0b11, cmd, 0b0011) >> 16) & 0xffff;
	} else {
		_unimplemented();
	}
}

static uint32_t pci_read32(uint32_t addr, uint8_t cmd) {
	return master_read(addr, cmd, 0b0000);
}

static void pci_write8(uint32_t addr, uint8_t val, uint8_t cmd) {
	uint8_t mask;
	uint8_t shift;

	switch (addr & 0b11) {
	case 0b00: mask = 0b1110; shift =  0; break;
	case 0b01: mask = 0b1101; shift =  8; break;
	case 0b10: mask = 0b1011; shift = 16; break;
	case 0b11: mask = 0b0111; shift = 24; break;
	}

	master_write(addr & ~0b11, cmd, mask, (uint32_t)val << shift);
}

static void pci_write16(uint32_t addr, uint16_t val, uint8_t cmd) {
	if ((addr & 0b11) == 0b00) {
		master_write(addr, cmd, 0b1100, val);
	} else if ((addr & 0b11) == 0b10) {
		master_write(addr & ~0b11, cmd, 0b0011, (uint32_t)val << 16);
	} else {
		_unimplemented();
	}
}

static void pci_write32(uint32_t addr, uint32_t val, uint8_t cmd) {
	master_write(addr, cmd, 0b0000, val);
}

/* Configuration transactions */

uint32_t pci_config_read32(uint8_t addr) {
	return pci_read32(addr, CMD_CONFIG_READ);
}

void pci_config_write8(uint8_t addr, uint8_t val) {
	pci_write8(addr, val, CMD_CONFIG_WRITE);
}

void pci_config_write16(uint8_t addr, uint16_t val) {
	pci_write16(addr, val, CMD_CONFIG_WRITE);
}

void pci_config_write32(uint8_t addr, uint32_t val) {
	pci_write32(addr, val, CMD_CONFIG_WRITE);
}

/* I/O Access */

uint8_t pci_io_read8(uint32_t addr) {
	return pci_read8(addr, CMD_IO_READ);
}

uint16_t pci_io_read16(uint32_t addr) {
	return pci_read16(addr, CMD_IO_READ);
}

uint32_t pci_io_read32(uint32_t addr) {
	return pci_read32(addr, CMD_IO_READ);
}

void pci_io_write8(uint32_t addr, uint8_t val) {
	pci_write8(addr, val, CMD_IO_WRITE);
}

void pci_io_write16(uint32_t addr, uint16_t val) {
	pci_write16(addr, val, CMD_IO_WRITE);
}

void pci_io_write32(uint32_t addr, uint32_t val) {
	pci_write32(addr, val, CMD_IO_WRITE);
}

/* Mem Access */

uint8_t pci_mem_read8(uint32_t addr) {
	return pci_read8(addr, CMD_MEM_READ);
}

uint16_t pci_mem_read16(uint32_t addr) {
	return pci_read16(addr, CMD_MEM_READ);
}

uint32_t pci_mem_read32(uint32_t addr) {
	return pci_read32(addr, CMD_MEM_READ);
}

void pci_mem_write8(uint32_t addr, uint8_t val) {
	pci_write8(addr, val, CMD_MEM_WRITE);
}

void pci_mem_write16(uint32_t addr, uint16_t val) {
	pci_write16(addr, val, CMD_MEM_WRITE);
}

void pci_mem_write32(uint32_t addr, uint32_t val) {
	pci_write32(addr, val, CMD_MEM_WRITE);
}
