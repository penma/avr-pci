#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "console.h"
#include "pci_signals.h"

__attribute__((noreturn)) static void panic(const char *m) {
	disconnect_bus();
	PCICR &= ~(1 << PCIE2);
	console_str(m);
	while (1) { }
}

static uint8_t ad_cbe_parity(uint32_t addr, uint8_t cbe) {
	/* even number of ones in addr, cbe, par */
	return !!(__builtin_parityl(addr) ^ __builtin_parity(cbe));
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

enum _rw_type { READ_TRANSACTION, WRITE_TRANSACTION };

__attribute__((always_inline)) static uint32_t master_transaction(uint32_t addr, uint8_t cmd, uint8_t be, uint32_t value, enum _rw_type type) {
	sanity_deasserted_frame_irdy();

	/* Address phase */

	clk_high();
	clk_low();
	assert_frame();

	ad_output_mode();
	ad_set(addr);
	cbe_output_mode();
	cbe_set(cmd);
	uint8_t addr_par = ad_cbe_parity(addr, cmd);

	/* prepare for the first data phase
	 * we assert IRDY, because we are ready to transfer the first data word
	 * we deassert FRAME, because it is going to be the last data word
	 */

	clk_high();
	uint8_t data_par;
	if (type == READ_TRANSACTION) {
		/* the following cycle is a turnaround cycle.
		 * the necessary extra cycle is enforced by the target in
		 * this case
		 */
		ad_tristate();
	} else {
		/* we directly provide the first data word */
		ad_set(value);
		data_par = ad_cbe_parity(value, be);
	}
	cbe_set(be);

	clk_low();
	assert_irdy();
	deassert_frame_1();

	par_output_mode();
	par_set(addr_par);

	/* wait for DEVSEL to be asserted */
	int c = 4;
	while (!is_devsel_asserted()) {
		clk_high();
		if (type == READ_TRANSACTION) {
			par_tristate();
		} else {
			par_set(data_par);
		}
		clk_low();
		c--;

		if (c == 0) {
			console_fstr("Master Abort/!DEVSEL");
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
		if (type == READ_TRANSACTION) {
			par_tristate();
		} else {
			par_set(data_par);
		}
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
	 * phase is about to happen.
	 */
	uint32_t adval;
	if (type == READ_TRANSACTION) {
		adval = ad_get();
		clk_high();
		par_tristate();
	} else {
		clk_high();
		par_set(data_par);
		ad_tristate();
	}
	deassert_irdy_1();
	deassert_frame_2();
	cbe_tristate();
	clk_low();

	/* target should now also return bus to idle
	 * also, this is one clock after the data phase, so the last cycle for
	 * parity
	 */
	if (type == READ_TRANSACTION) {
		/* target provides parity, we read and verify it */
		uint8_t adval_par = par_get();
		if (adval_par != ad_cbe_parity(adval, be)) {
			/* TODO handle properly */
			panic("Parity error");
		}
	}
	clk_high();
	deassert_irdy_2();
	par_tristate();
	clk_low();

	sanity_deasserted_devsel_trdy();

	if (type == READ_TRANSACTION) {
		return adval;
	} else {
		return 0;
	}

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


uint32_t master_read(uint32_t addr, uint8_t cmd, uint8_t be) {
	return master_transaction(addr, cmd, be, 0, READ_TRANSACTION);
}

void master_write(uint32_t addr, uint8_t cmd, uint8_t be, uint32_t value) {
	master_transaction(addr, cmd, be, value, WRITE_TRANSACTION);
}
