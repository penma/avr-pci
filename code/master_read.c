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


uint32_t master_read(uint32_t addr, uint8_t cmd, uint8_t be) {
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

	/* turnaround cycle, tristate AD
	 * we assert IRDY, because we are ready to receive the first data word
	 * we deassert FRAME, because it is going to be the last data word
	 */

	clk_high();
	ad_tristate();
	cbe_set(be);

	clk_low();
	assert_irdy();
	deassert_frame_1();
	idsel_low();

	par_output_mode();
	par_set(addr_par);

	/* wait for DEVSEL to be asserted */
	int c = 4;
	while (!is_devsel_asserted()) {
		clk_high();
		par_tristate();
		clk_low();
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
			console_fstr("unreal master abort");
			goto master_abort;
		}
	}

	/* TRDY is asserted and as we have previously asserted IRDY, a data
	 * phase is about to happen. therefore read from AD
	 */
	uint32_t adval = ad_get();
	clk_high();

	par_tristate();
	deassert_irdy_1();
	deassert_frame_2();
	cbe_tristate();
	clk_low();

	/* target should now also return bus to idle
	 * target also provides parity for the data word */
	uint8_t adval_par = par_get();
	clk_high();
	deassert_irdy_2();
	clk_low();

	/* verify parity of received data word against that of the PAR pin */
	if (adval_par != ad_cbe_parity(adval, be)) {
		/* TODO handle properly */
		panic("Parity error");
	}

	sanity_deasserted_devsel_trdy();

	return adval;

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
	/* TODO - currently unimplemented (RTL8139/69 never respond with
	 * Target Retry
	 */
	panic("Target requested Retry but this is unimplemented");
}


