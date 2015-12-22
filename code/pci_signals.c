#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include "timing.h"

/* port A */
#define PA_INTA (1 << 7)
#define PA_INTB (1 << 6)
#define PA_INTC (1 << 4)
#define PA_INTD (1 << 5)

/* port B */
#define PB_RST (1 << 6)
#define PB_CLK (1 << 5)
#define PB_GNT (1 << 4)
#define PB_REQ (1 << 0)

/* port F */
#define PF_STOP    (1 << 6)
#define PF_DEVSEL  (1 << 5)
#define PF_TRDY    (1 << 4)
#define PF_IRDY    (1 << 1)
#define PF_FRAME   (1 << 0)

/* port G */
#define PG_IDSEL (1 << 5)

/* port K */
#define PK_PAR (1 << 7)
#define PK_SERR (1 << 2)
#define PK_PERR (1 << 1)
#define PK_LOCK (1 << 0)

/* A/D lines */

void ad_output_mode() {
	DDRC = DDRE = DDRH = DDRJ = 0xff;
}

void ad_tristate() {
	DDRC = DDRE = DDRH = DDRJ = 0x00;
	PORTC = PORTE = PORTH = PORTJ = 0x00;
}

void ad_set(uint32_t v) {
	/* TODO check endianness and swap stuff around if needed */
	PORTC = v & 0xff;
	PORTJ = (v >> 8) & 0xff;
	PORTE = (v >> 16) & 0xff;
	PORTH = (v >> 24) & 0xff;
}

uint32_t ad_get() {
	/* TODO endianness, see above */
	return (uint32_t)PINC | ((uint32_t)PINJ << 8) | ((uint32_t)PINE << 16) | ((uint32_t)PINH << 24);
}

/* PAR line */

void par_output_mode() {
	DDRK |= PK_PAR;
}

void par_tristate() {
	DDRK &= ~PK_PAR;
	PORTK &= ~PK_PAR;
}

void par_set(uint8_t v) {
	if (v) {
		PORTK |= PK_PAR;
	} else {
		PORTK &= ~PK_PAR;
	}
}

uint8_t par_get() {
	return !!(PINK & PK_PAR);
}

/* C/BE lines */

void cbe_output_mode() {
	DDRA |= 0x0f;
}

void cbe_tristate() {
	DDRA &= ~(0x0f);
	PORTA &= ~(0x0f);
}

void cbe_set(uint8_t v) {
	PORTA &= ~(0x0f);
	PORTA |= (v & 0xf);
}

/* CLK line */

__attribute__((always_inline)) void clk_high() {
	PORTB |= PB_CLK;
}

__attribute__((always_inline)) void clk_low() {
	PORTB &= ~PB_CLK;
}

/* a signal is asserted by
 * disabling the pullup (this causes the signal to be driven high for a moment)
 * driving it low
 */
#define _assert_signal(s) do { DDRF |= (s); PORTF &= ~(s); } while (0)
/* deasserting is a two-step process
 * in the first PCI clock cycle the pin is driven high
 * one cycle later, stop driving it and only have the pullup
 */
#define _deassert_signal_1(s) do { PORTF |= (s); } while (0)
#define _deassert_signal_2(s) do { DDRF &= ~(s); } while (0)

/* various signals */

#define FUNCS_SIG(x,y) \
	void assert_##y() { _assert_signal(PF_##x); } \
	void deassert_##y##_1() { _deassert_signal_1(PF_##x); } \
	void deassert_##y##_2() { _deassert_signal_2(PF_##x); } \
	int is_##y##_asserted() { return !(PINF & (PF_##x)); }

FUNCS_SIG(FRAME,  frame)
FUNCS_SIG(STOP,   stop)
FUNCS_SIG(DEVSEL, devsel)
FUNCS_SIG(TRDY,   trdy)
FUNCS_SIG(IRDY,   irdy)

void initialize_bus() {
	/* do a bus reset. for this, assert RST# first.
	 * while we're at it, we start configuring port B correctly
	 * (RST, CLK, GNT as outputs and initially low,
	 * REQ as input with pullup)
	 * 1,2,3,7 are not connected (in case of 1,2,3 not to something we need
	 * at least) and therefore pulled up to prevent floating
	 */
	DDRB = PB_RST | PB_CLK | PB_GNT;
	PORTB = PB_REQ | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 7);

	/* device is now in reset and will remain there for a while.

	/* enable pullups for INTx, tristate C/BE
	 * C/BE are defined as t/s and therefore don't need (shouldn't have?)
	 * a central pullup. ok... (but that assumes that bus parking happens
	 * to provide stable levels when the bus is idle. maybe we can cheat
	 * and provide the stable levels via a pullup instead.) TODO
	 */
	DDRA = 0;
	PORTA = PA_INTA | PA_INTB | PA_INTC | PA_INTD;

	/* pullups for bus signals.
	 * 2, 3, 7 are not connected and pulled up to prevent them from
	 * floating.
	 */
	DDRF = 0;
	PORTF = PF_STOP | PF_DEVSEL | PF_TRDY | PF_IRDY | PF_FRAME | (1 << 2) | (1 << 3) | (1 << 7);

	/* IDSEL is an output and initially high (we only have one slot, so we
	 * can always select it).
	 * 0,1,2,3,4 are not connected and pulled up
	 */
	DDRG = PG_IDSEL;
	PORTG = PG_IDSEL | (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);

	/* PAR etc.
	 * 3,4,5,6 are not connected and pulled up
	 * TODO: figure out if par is pulled up instead of tristated (see CBE)
	 */
	DDRK = 0;
	PORTK = PK_SERR | PK_PERR | PK_LOCK | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);

	/* address bus is completely tri-stated (TODO see above if pullup instead) */
	ad_tristate();

	/* keep reset active for some some time. spec says at least 1ms. */
	_delay_ms(2);

	/* bring device out of reset. */
	PORTB |= PB_RST;

	/* we're supposed to do some clock cycles before any configuration
	 * accesses. According to spec, 2^25 cycles.
	 * This might take some time on our MCU. Could save some with the timer
	 * (CLK is conveniently placed on an output compare pin)
	 * but it would still be some seconds.
	 * As the whole setup is a giant hack anyway we just do some cycles
	 * less and use what works.
	 * We leave it up to the application to decide how many clock cycles
	 * should happen. We just do one here. The Realtek cards are happy
	 * with this.
	 * If the application gets Master Aborts or Target Retrys during
	 * enumeration it can just generate extra cycles as needed.
	 * (start with e.g. 2^12 cycles and do another 2^12 ... 2^24 cycles
	 * until enumeration succeeds. In total, 2^25 cycles will have happened
	 * then.)
	 */

	clk_high();
	clk_low();

	if (0) for (uint32_t c = 0; c <= ((uint32_t)1 << (25-2)); c++) {
		clk_high(); clk_low();
		clk_high(); clk_low();
		clk_high(); clk_low();
		clk_high(); clk_low();
	}
}

/* completely disconnect from the bus.
 * designed to be used in an emergency shutdown of some kind.
 */
static void _disconnect_bus_2() {
	/* configure all PCI signals as inputs and then disable all pullups
	 * we clear the ports with the low addresses first, those are
	 * ACDEFG. HJK are only accessible via slower store to SRAM.
	 * Executed from right to left, we do it in this order:
	 * F (control signals)
	 * A (c/be, int)
	 * C, E (AD)
	 * H, J, K (AD, PAR)
	 * G (only we should drive it anyway, so can come last)
	 * there's enough time for disabling the pullups, so we do it in just
	 * some random order.
	 */
	DDRC = DDRE = DDRA = DDRF = 0;
	DDRH = DDRJ = DDRK = 0;
	DDRG = 0;
	PORTA = PORTC = PORTE = PORTF = PORTG = 0;
	PORTH = PORTJ = PORTK = 0;
}
__attribute__((always_inline)) void disconnect_bus() {
	/* of port B (arbitration signals), tristate all pins, except RST#,
	 * which is set to output low so that targets tristate their outputs
	 * immediately (probably/hopefully faster than we do)
	 * waste as little time as possible by doing this inlined.
	 * first drive RST low (assuming the bus has been initialized properly
	 * before, RST is already an output pin).
	 * next, tristate everything but RST (which becomes an output now if it
	 * wasn't already)
	 * then, for all other pins, pullups are disabled.
	 */
	PORTB &= ~PB_RST;
	DDRB = PB_RST;
	PORTB = 0;

	/* the reset part of this function is supposed to be inlined so that
	 * the reset can happen as quickly as possible.
	 * disabling other signals can happen in a function.
	 */
	_disconnect_bus_2();
}

