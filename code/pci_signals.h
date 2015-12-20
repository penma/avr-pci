#ifndef PCI_SIGNALS_H
#define PCI_SIGNALS_H

#include <stdint.h>

void ad_output_mode();
void ad_tristate();
void ad_set(uint32_t v);
uint32_t ad_get();

void par_output_mode();
void par_tristate();
void par_set(uint8_t v);
uint8_t par_get();

void cbe_output_mode();
void cbe_tristate();
void cbe_set(uint8_t v);

void clk_high();
void clk_low();

void assert_frame();
void deassert_frame_1();
void deassert_frame_2();
int is_frame_asserted();

void assert_stop();
void deassert_stop_1();
void deassert_stop_2();
int is_stop_asserted();

void assert_devsel();
void deassert_devsel_1();
void deassert_devsel_2();
int is_devsel_asserted();

void assert_trdy();
void deassert_trdy_1();
void deassert_trdy_2();
int is_trdy_asserted();

void assert_irdy();
void deassert_irdy_1();
void deassert_irdy_2();
int is_irdy_asserted();

void initialize_bus();
void disconnect_bus();

#endif
