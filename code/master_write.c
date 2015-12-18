#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "console.h"
#include "pci_signals.h"

static void panic(const char *m) {
	disconnect_bus();
	console_str(m);
	while (1) { }
}

static uint8_t ad_cbe_parity(uint32_t addr, uint8_t cbe) {
	/* even number of ones in addr, cbe, par */
	return !!(__builtin_parity(addr) ^ __builtin_parity(cbe));
}

static void sanity_deasserted_devsel_trdy() {
	/* sanity check: DEVSEL and TRDY should now be deasserted */
	if (is_devsel_asserted() || is_trdy_asserted()) {
		panic("DEVSEL or TRDY still asserted");
	}
}

static void sanity_deasserted_frame_irdy() {
	/* sanity check: FRAME and IRDY should not be asserted because the
	 * bus is idle */
	if (is_frame_asserted() || is_irdy_asserted()) {
		panic("FRAME or IRDY asserted on idle bus");
	}
}

static void dumpsignals() {
	console_fstr("cbe");
	console_hex8(PINA);
	console_fstr(" ");
	if (is_devsel_asserted()) { console_fstr("DS "); } else { console_fstr("ds "); }
	if (is_frame_asserted()) { console_fstr("FR "); } else { console_fstr("fr "); }
	if (is_trdy_asserted()) { console_fstr("TR "); } else { console_fstr("tr "); }
	if (is_irdy_asserted()) { console_fstr("IR "); } else { console_fstr("ir "); }
	console_fstr("\n");
	_delay_ms(1000);
}


void master_write(uint32_t addr, uint8_t cmd, uint8_t be, uint32_t value) {
	sanity_deasserted_frame_irdy();

	/* Address phase */

	clk_high();
	clk_low();
	assert_frame();
	idsel_high();

	ad_output_mode();
	ad_set(addr);
	cbe_output_mode();
	cbe_set(cmd);
	uint8_t addr_par = ad_cbe_parity(addr, cmd);

	/* start of first and last data phase
	 * assert IRDY, because going to send the first word
	 * deassert FRAME, because this is the last word
	 */

	clk_high();
	clk_low();

	assert_irdy();
	deassert_frame_1();
	idsel_low();

	ad_set(value);
	cbe_set(be);
	uint8_t data_par = ad_cbe_parity(value, be);

	par_output_mode();
	par_set(addr_par);

	/* wait for DEVSEL to be asserted */
	int c = 4;
	while (!is_devsel_asserted()) {
		clk_high();
		clk_low();
		par_set(data_par);
		c--;

		if (c == 0) {
			console_fstr("Master abort - DEVSEL not asserted\n");
			goto master_abort;
		}
	}

	/* wait for TRDY to be asserted */
	c = 12;
	while (!is_trdy_asserted()) {
		if (is_stop_asserted()) {
			if (is_devsel_asserted()) {
				goto retry;
			} else {
				console_fstr("target abort\n");
				goto target_abort;
			}
		}

		clk_high();
		clk_low();
		par_set(data_par);
		c--;

		if (c == 0) {
			/* actually not a master abort, but we handle it
			 * exactly like one.
			 * (not sure if this can actually happen according to
			 * the spec, but it's a good idea to have some kind of
			 * timeout here...)
			 */
			console_fstr("unreal master abort");
			goto master_abort;
		}
	}

	/* TRDY is asserted and as we have previously asserted IRDY, a data
	 * phase is about to happen. pulse clock, the target will do the write.
	 */
	clk_high();
	clk_low();

	par_set(data_par);
	deassert_irdy_1();
	deassert_frame_2();
	cbe_tristate();
	ad_tristate();

	/* target should now also return bus to idle
	 * afterwards, we stop providing parity */
	clk_high();
	clk_low();
	deassert_irdy_2();
	par_tristate();

	sanity_deasserted_devsel_trdy();

master_abort:
target_abort:
	deassert_irdy_1();
	deassert_frame_1();
	ad_tristate();
	cbe_tristate();
	par_tristate();
	clk_high();
	deassert_irdy_2();
	deassert_frame_2();
	clk_low();

	sanity_deasserted_devsel_trdy();

	return 0xffffffff;

retry:
	/* TODO */
	panic("Target requested Retry but this is unimplemented");
}


