#include <avr/io.h>
#include <stdint.h>
#include "console.h"

#define panic(m) do { console_fstr(m); while (1) { } } while (0)

static inline void clk_high() __attribute__((always_inline));
static inline void clk_high() {
	PORTB |= 1 << 5;
}

static inline void clk_low() __attribute__((always_inline));
static inline void clk_low() {
	PORTB &= ~(1 << 5);
}

/* port F control signals */
#define FRAME   (1 << 0)
#define IRDY    (1 << 1)
#define TRDY    (1 << 4)
#define DEVSEL  (1 << 5)
#define STOP    (1 << 6)

/* a signal is asserted by
 * disabling the pullup (this causes the signal to be driven high for a moment)
 * driving it low
 */
#define assert_signal(s) do { DDRF |= (s); PORTF &= ~(s); } while (0)
/* deasserting is a two-step process
 * in the first PCI clock cycle the pin is driven high
 * one cycle later, stop driving it and only have the pullup
 */
#define deassert_signal_1(s) do { PORTF |= (s); } while (0)
#define deassert_signal_2(s) do { DDRF &= ~(s); } while (0)

#define is_signal_asserted(s) (!(PINF | (s)))

static inline void idsel_high() {
	PORTG |= 1 << 5;
}

static inline void idsel_low() {
	PORTG &= ~(1 << 5);
}

static void ad_output_mode() {
	DDRC = DDRE = DDRH = DDRJ = 0xff;
}

static void ad_tristate() {
	DDRC = DDRE = DDRH = DDRJ = 0x00;
	PORTC = PORTE = PORTH = PORTJ = 0x00;
}

static void ad_set(uint32_t v) {
	/* TODO check endianness and swap stuff around if needed */
	PORTC = v & 0xff;
	PORTJ = (v >> 8) & 0xff;
	PORTE = (v >> 16) & 0xff;
	PORTH = (v >> 24) & 0xff;
}

static uint32_t ad_get() {
	/* TODO endianness, see above */
	return PORTC | ((uint32_t)PORTJ << 8) | ((uint32_t)PORTE << 16) | ((uint32_t)PORTH << 24);
}

static void par_output_mode() {
	DDRK |= 1 << 7;
}

static void par_tristate() {
	DDRK &= ~(1 << 7);
	PORTK &= ~(1 << 7);
}

static void par_set(uint8_t v) {
	if (v) {
		PORTK |= 1 << 7;
	} else {
		PORTK &= ~(1 << 7);
	}
}

static inline uint8_t par_get() {
	return PINK & (1 << 7);
}

static void cbe_output_mode() {
	DDRA |= 0x0f;
}

static void cbe_tristate() {
	DDRA &= ~(0x0f);
	PORTA &= ~(0x0f);
}

static void cbe_set(uint8_t v) {
	PORTA &= ~(0x0f);
	PORTA |= (v & 0xf);
}

uint8_t ad_cbe_parity(uint32_t addr, uint8_t cbe) {
	/* even number of ones in addr, cbe, par */
	return __builtin_parity(addr) ^ __builtin_parity(cbe);
}

static void sanity_deasserted_devsel_trdy() {
	/* sanity check: DEVSEL and TRDY should now be deasserted */
	if (is_signal_asserted(DEVSEL) || is_signal_asserted(TRDY)) {
		panic("DEVSEL or TRDY still asserted");
	}
}

static void sanity_deasserted_frame_irdy() {
	/* sanity check: FRAME and IRDY should not be asserted because the
	 * bus is idle */
	if (is_signal_asserted(FRAME) || is_signal_asserted(IRDY)) {
		panic("FRAME or IRDY asserted on idle bus");
	}
}


uint32_t master_read(uint32_t addr, uint8_t cmd, uint8_t be) {
	sanity_deasserted_frame_irdy();

	/* Address phase */

	clk_high();
	clk_low();
	assert_signal(FRAME);
	idsel_high();

	ad_output_mode();
	ad_set(addr);
	cbe_output_mode();
	cbe_set(cmd);
	uint8_t addr_par = ad_cbe_parity(addr, cmd);

	/* turnaround cycle */

	clk_high();
	ad_tristate();
	clk_low();
	assert_signal(IRDY);
	deassert_signal_1(FRAME);
	idsel_low();

	par_output_mode();
	par_set(addr_par);
	cbe_set(be);

	/* wait for DEVSEL to be asserted */
	int c = 4;
	while (!is_signal_asserted(DEVSEL)) {
		clk_high();
		par_tristate();
		clk_low();
		c--;

		if (c == 0) {
			goto master_abort;
		}
	}

	/* wait for TRDY to be asserted */
	c = 12;
	while (!is_signal_asserted(TRDY)) {
		if (is_signal_asserted(STOP)) {
			if (is_signal_asserted(DEVSEL)) {
				goto retry;
			} else {
				goto target_abort;
			}
		}

		clk_high();
		par_tristate();
		clk_low();
		c--;

		if (c == 0) {
			/* actually not a master abort, but we handle it
			 * exactly like one.
			 * (not sure if this can actually happen according to
			 * the spec, but it's a good idea to have some kind of
			 * timeout here...)
			 */
			goto master_abort;
		}
	}

	/* TRDY is asserted and as we have previously asserted IRDY, a data
	 * phase is about to happen
	 */
	uint32_t adval = ad_get();
	clk_high();
	par_tristate();
	deassert_signal_1(IRDY);
	deassert_signal_2(FRAME);
	cbe_tristate();
	clk_low();

	/* target should now also return bus to idle
	 * target also provides parity for the data word */
	uint8_t adval_par = par_get();
	clk_high();
	deassert_signal_2(IRDY);
	clk_low();

	/* TODO: verify parity of received data word against that of the PAR pin */

	sanity_deasserted_devsel_trdy();

	return adval;

master_abort:
target_abort:
	deassert_signal_1(IRDY);
	deassert_signal_1(FRAME);
	cbe_tristate();
	clk_high();
	deassert_signal_2(IRDY);
	deassert_signal_2(FRAME);
	clk_low();

	sanity_deasserted_devsel_trdy();

	return 0xffffffff;

retry:
	/* TODO */
	panic("Target requested Retry but this is unimplemented");
}





volatile uint32_t dummyaddr;
volatile uint8_t cbedummy;
void main() {
	while (1) {
		master_read(dummyaddr, cbedummy, 0xf);
	}
}

