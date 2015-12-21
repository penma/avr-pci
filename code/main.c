
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "console.h"

#include "pci_signals.h"
#include "master_transaction.h"

static void panic(const char *m) {
	disconnect_bus();
	console_str(m);
	while (1) { }
}

ISR(PCINT2_vect) {
	uint8_t pk = PINK;
	console_fstr("*ERR changed:");
	if (!(pk & (1 << 1))) {
		console_fstr(" PERR");
	}
	if (!(pk & (1 << 2))) {
		console_fstr(" SERR");
	}
	console_fstr("\n");
}

void main() {
	console_reset();

	/* for timing/benchmarking purposes.
	 * compare TCNT3 before and after some code to get a cycle count
	 */
	TCCR3A = 0;
	TCCR3B = (1 << CS30);

	console_fstr("Stuff initialized! Yay! \\o/\n");

	initialize_bus();

	PCMSK2 = (1 << PCINT17) | (1 << PCINT18);
	PCICR |= (1 << PCIE2);
	sei();

	console_fstr("Bus initialized\n");

	/* configuration read, full dword, offset 0 */
	uint32_t vpid = master_read(0, 0b1010, 0b0000);
	uint16_t vendor = vpid & 0xffff;
	uint16_t dev = (vpid >> 16) & 0xffff;

	console_fstr("ID = ");
	console_hex16(vendor);
	console_fstr(":");
	console_hex16(dev);

	/* BARs */
	uint32_t bar_rb;
	for (uint8_t bv = 0x10; bv <= 0x24; bv += 4) {
		master_write(bv, 0b1011, 0b0000, 0xffffffff);
		bar_rb = master_read(bv, 0b1010, 0b0000);
		if (!(bar_rb & 0x80000000)) {
			/* either not valid or the device actually demands
			 * 4G of IO space, which we will just refuse then
			 */
			continue;
		}
		console_fstr("\n");
		console_hex8(bv);
		console_fstr(":");
		if (bar_rb & (1 << 0)) {
			console_fstr("I/O");
		} else {
			console_fstr("Mem");
			if (bar_rb & (1 << 3)) {
				console_fstr("P");
			}
			switch (bar_rb & 0b111) {
			case 0b000: console_fstr("32"); break;
			case 0b010: console_fstr("1M"); break;
			case 0b100: console_fstr("64"); break;
			default:    console_fstr("??"); break;
			}
		}
		uint8_t size = __builtin_ctz(bar_rb & ~((uint32_t) 0b1111));
		console_fstr(" 2^");
		console_hex8(size);
	}

	panic("/done");
}


