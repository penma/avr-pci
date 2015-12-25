#include <avr/io.h>
#include <avr/interrupt.h>
#include "console.h"
#include "pci/signals.h"
#include "pci/panic.h"

void panic(const char *m) {
	disconnect_bus();
	/* pin change interrupt for PERR and SERR.
	 * as we stop driving them, we might get irrelevant interrupts.
	 * TODO: unsure where this should actually be handled
	 */
	cli();
	console_str(m);
	while (1) { }
}

