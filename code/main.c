
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "console.h"
#include "timing.h"

#include "pci/pci.h"
#include "pci/registers.h"
#include "pci/signals.h"
#include "pci/panic.h"

#include "rtl8169.h"

ISR(PCINT2_vect) {
	uint8_t pk = PINK;
	console_fstr("*ERR changed:");
	if (!(pk & (1 << 1))) {
		console_fstr(" P");
	}
	if (!(pk & (1 << 2))) {
		console_fstr(" S");
	}
	console_fstr("\n");
}

ISR(PCINT0_vect) {
	uint8_t pb = PINB;
	if (!(pb & (1 << 0))) {
		console_fstr("REQ BUS\n");
	}
}

static void identify_subdevice_generic(uint32_t vpid, uint32_t svpid) {
	if (vpid != svpid) {
		uint16_t svendor = svpid & 0xffff;
		uint16_t sdev = (svpid >> 16) & 0xffff;

		console_hex16(svendor);
		console_fstr(":");
		console_hex16(sdev);
		console_fstr("/");
	}
}

void identify_device(uint32_t vpid, uint32_t svpid) {
	if (vpid == 0x816910ec) {
		identify_subdevice_generic(vpid, svpid);
		console_fstr("RTL8169/8110 Gigabit");
	} else if (vpid == 0x813910ec) {
		identify_subdevice_generic(vpid, svpid);
		console_fstr("RTL8139/8100/1L 100M");
	} else if (vpid == 0x00351033 && svpid == 0x010514c2) {
		console_fstr("USB2.0 Host PTI-205N");
	} else if (vpid == 0x000210b6) {
		/* no sub ID, values appear useless (eeeb:dbaf here) */
		console_fstr("Madge 16/4 Ringnode\n");
	} else if (vpid == 0x30381106) {
		identify_subdevice_generic(vpid, svpid);
		console_fstr("USB1.1 Host VIA VT82");
	} else if (vpid == 0x12161113 && svpid == vpid) {
		console_fstr("Accton EN1207F\n");
	} else {
		uint16_t vendor = vpid & 0xffff;
		uint16_t dev = (vpid >> 16) & 0xffff;
		uint16_t svendor = svpid & 0xffff;
		uint16_t sdev = (svpid >> 16) & 0xffff;

		console_hex16(vendor);
		console_fstr(":");
		console_hex16(dev);

		if ((vendor != svendor) || (dev != sdev)) {
			console_fstr("/");
			console_hex16(svendor);
			console_fstr(":");
			console_hex16(sdev);
		}
	}
}

void main() {
	console_reset();

	timing_init();
	sei();

	console_fstr("I'M ALIVE\n");

	initialize_bus();

	PCMSK0 |= (1 << PCINT0);
	PCMSK2 = (1 << PCINT17) | (1 << PCINT18);
	PCICR |= (1 << PCIE0) | (1 << PCIE2);

	console_fstr("PCI Bus initialized\n");

	timing_start();

	/* device present?! yes we read this twice, but whatever */

	if (pci_config_read32(0) == 0xffffffff) {
		panic("/no device");
	}

	/* configuration read, full dword, offset 0 */
	uint32_t vpid = pci_config_read32(PCIR_DEVVENDOR);
	uint32_t svpid = pci_config_read32(PCIR_SUBVEND_0);

	identify_device(vpid, svpid);

	/* BARs */
	uint32_t bar_rb;
	uint8_t bar_ct = 0;
	for (uint8_t bar_no = 0; bar_no <= PCIR_MAX_BAR_0; bar_no++) {
		uint8_t bv = PCIR_BAR(bar_no);
		pci_config_write32(bv, 0xffffffff);
		bar_rb = pci_config_read32(bv);
		if (!(bar_rb & 0x80000000)) {
			/* either not valid or the device actually demands
			 * 4G of IO space, which we will just refuse then
			 */
			continue;
		}
		if (bar_ct) { console_fstr("\n"); }
		bar_ct++;
		console_hex8(bv);
		console_fstr(":");
		if (PCI_BAR_IO(bar_rb)) {
			console_fstr("I/O  ");
		} else {
			console_fstr("Mem");
			if (bar_rb & PCIM_BAR_MEM_PREFETCH) {
				console_fstr("P");
			}
			switch (bar_rb & PCIM_BAR_MEM_TYPE) {
			case PCIM_BAR_MEM_32 : console_fstr("32"); break;
			case PCIM_BAR_MEM_1MB: console_fstr("1M"); break;
			case PCIM_BAR_MEM_64 : console_fstr("64"); break;
			default:    console_fstr("??"); break;
			}
		}

		console_fstr(" ");

		uint8_t size = __builtin_ctz(bar_rb & ~((uint32_t) 0b1111));
		const char *names[] = {
			"1b", "2b", "4b", "8b",
			"16b", "32b", "64b", "128b",
			"256b", "512b", "1K", "2K",
			"4K", "8K", "16K", "32K",
			"64K", "128K", "256K", "512K",
			"1M", "2M", "4M", "8M",
			"16M", "32M", "64M", "128M",
			"256M", "512M", "1G", "2G",
			"4G"
		};
		console_str(names[size]);
	}

	timing_end();

	rtl8169_init();

	panic("/done");
}


