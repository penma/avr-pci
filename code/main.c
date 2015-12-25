
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

#include "lspci.h"
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

	/* device present?! yes we read this twice, but whatever */
	uint32_t pvid = pci_config_read32(PCIR_DEVVENDOR);
	if (pvid == 0xffffffff) {
		panic("/no device");
	} else if (pvid == 0x816910ec) {
		rtl8169_init();
	} else {
		lspci_init();
	}

	panic("/done");
}


