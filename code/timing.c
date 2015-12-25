#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "console.h"
#include "pci/panic.h"

ISR(TIMER3_OVF_vect) {
	/* we abuse Timer 4 value as higher-order bits */
	TCNT4++;
}

void timing_init() {
	TCCR3A = 0;
	TCCR3B = (1 << CS30);
	TIMSK3 |= (1 << TOIE3);
	/* interrupts must be enabled for higher 16 bits */
}

static uint16_t s_c3, s_c4a, s_c4b;
static uint16_t e_c3, e_c4a, e_c4b;

void timing_start() {
	s_c4a = TCNT4;
	s_c3  = TCNT3;
	s_c4b = TCNT4;
}

void timing_end() {
	e_c4a = TCNT4;
	e_c3  = TCNT3;
	e_c4b = TCNT4;

	uint16_t s_c4, e_c4;

	if (s_c4a == s_c4b) {
		s_c4 = s_c4a;
	} else if (s_c4a + 1 == s_c4b) {
		console_fstr("*");
		if (s_c3 < 0x8000) {
			s_c4 = s_c4b;
		} else {
			s_c4 = s_c4a;
		}
	} else {
		console_fstr("Tstart:");
		console_hex16(s_c4a);
		console_fstr("/");
		console_hex16(s_c3);
		console_fstr("/");
		console_hex16(s_c4b);
		panic("");
	}

	if (e_c4a == e_c4b) {
		e_c4 = e_c4a;
	} else if (e_c4a + 1 == e_c4b) {
		console_fstr("*");
		if (e_c3 < 0x8000) {
			e_c4 = e_c4b;
		} else {
			e_c4 = e_c4a;
		}
	} else {
		console_fstr("Tend:");
		console_hex16(e_c4a);
		console_fstr("/");
		console_hex16(e_c3);
		console_fstr("/");
		console_hex16(e_c4b);
		panic("");
	}

	uint32_t tdiff = (e_c3 | (uint32_t)(e_c4) << 16) - (s_c3 | (uint32_t)(s_c4) << 16);

	uint16_t sec = tdiff / F_CPU;
	uint16_t msec = (tdiff / (F_CPU / 1000)) % 1000;
	uint8_t msec100 = (msec / 100) % 10;
	uint8_t msec10 = (msec / 10) % 10;
	uint8_t msec1 = msec % 10;
	console_dec16(sec);
	console_fstr(".");
	console_char(msec100 + '0');
	console_char(msec10 + '0');
	console_char(msec1 + '0');
	console_fstr("s");
}

