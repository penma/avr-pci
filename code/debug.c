
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "debug.h"

/* regular debug port */
/*
#define DBG_SHIFT PL7
#define DBG_SYNC PL6
#define DBG_DATA PL5
*/

/* 6ISP repurposed as debug port */
#define DBG_SHIFT PB1
#define DBG_SYNC PB3
#define DBG_DATA PB2

#ifdef LCD_SUPPORT

#define LCD_ENABLE_US           20

// Clear Display -------------- 0b00000001
#define LCD_CLEAR_DISPLAY       0x01

// Cursor Home ---------------- 0b0000001x
#define LCD_CURSOR_HOME         0x02

// Set Entry Mode ------------- 0b000001xx
#define LCD_SET_ENTRY           0x04

#define LCD_ENTRY_DECREASE      0x00
#define LCD_ENTRY_INCREASE      0x02
#define LCD_ENTRY_NOSHIFT       0x00
#define LCD_ENTRY_SHIFT         0x01

// Set Display ---------------- 0b00001xxx
#define LCD_SET_DISPLAY         0x08

#define LCD_DISPLAY_OFF         0x00
#define LCD_DISPLAY_ON          0x04
#define LCD_CURSOR_OFF          0x00
#define LCD_CURSOR_ON           0x02
#define LCD_BLINKING_OFF        0x00
#define LCD_BLINKING_ON         0x01

// Set Shift ------------------ 0b0001xxxx
#define LCD_SET_SHIFT           0x10

#define LCD_CURSOR_MOVE         0x00
#define LCD_DISPLAY_SHIFT       0x08
#define LCD_SHIFT_LEFT          0x00
#define LCD_SHIFT_RIGHT         0x04

// Set Function --------------- 0b001xxxxx
#define LCD_SET_FUNCTION        0x20

#define LCD_FUNCTION_4BIT       0x00
#define LCD_FUNCTION_8BIT       0x10
#define LCD_FUNCTION_1LINE      0x00
#define LCD_FUNCTION_2LINE      0x08
#define LCD_FUNCTION_5X7        0x00
#define LCD_FUNCTION_5X10       0x04

// Set CG RAM Address --------- 0b01xxxxxx  (Character Generator RAM)
#define LCD_SET_CGADR           0x40

// Set DD RAM Address --------- 0b1xxxxxxx  (Display Data RAM)
#define LCD_SET_DDADR           0x80


enum LCD_RS { LCD_COMMAND = 0, LCD_DATA = 1 };

static void shiftreg_set(uint16_t sreg) {
	for (signed int i = 15; i >= 0; i--) {
		if ((sreg >> i) & 1) {
			PORTB |= (1 << DBG_DATA);
		} else {
			PORTB &= ~(1 << DBG_DATA);
		}

		PORTB |= (1 << DBG_SHIFT);
		PORTB &= ~(1 << DBG_SHIFT);
	}

	PORTB |= (1 << DBG_SYNC);
	PORTB &= ~(1 << DBG_SYNC);
}

static void write_byte(enum LCD_RS rs, uint8_t d) {
	uint16_t sreg = d;
	sreg <<= 8;
	if (rs == LCD_DATA) sreg |= 1 << 2;
	shiftreg_set(sreg);
	shiftreg_set(sreg | (1 << 3));
	shiftreg_set(sreg);
}

static void lcd_data(uint8_t data) {
	write_byte(LCD_DATA, data);
	_delay_us(46);
}

static void lcd_command(uint8_t data) {
	write_byte(LCD_COMMAND, data);
	_delay_us(42);
}

void lcd_clear() {
	lcd_command(LCD_CLEAR_DISPLAY);
	_delay_ms(2);
}

void lcd_home() {
	lcd_command(LCD_CURSOR_HOME);
	_delay_ms(2);
}

void lcd_setcursor(uint8_t x, uint8_t y) {
	const uint8_t ddadr[] = { 0x00, 0x40, 0x14, 0x54 };

	if (y >= 4) {
		return;
	}

	lcd_command(LCD_SET_DDADR | (ddadr[y] + x));
}

void lcd_generatechar(uint8_t code, const uint8_t *data) {
	lcd_command(LCD_SET_CGADR | (code << 3));

	for (int i = 0; i < 8; i++) {
		lcd_data(data[i]);
	}
}

void debug_init() {
	DDRB |= (1 << DBG_DATA) | (1 << DBG_SHIFT) | (1 << DBG_SYNC);
	PORTB &= ~( (1 << DBG_DATA) | (1 << DBG_SHIFT) | (1 << DBG_SYNC) );

	/* wait for display to be ready after bootup */
	_delay_ms(15);

	/* do a softreset
	 * setting it to 8bit mode three times allows a reset from any state.
	 */
	lcd_command(LCD_SET_FUNCTION | LCD_FUNCTION_8BIT);
	_delay_ms(5);

	lcd_command(LCD_SET_FUNCTION | LCD_FUNCTION_8BIT);
	_delay_ms(1);

	lcd_command(LCD_SET_FUNCTION | LCD_FUNCTION_8BIT);
	_delay_ms(1);

	/* now configure */
	lcd_command(LCD_SET_FUNCTION | LCD_FUNCTION_8BIT | LCD_FUNCTION_2LINE | LCD_FUNCTION_5X7);
	lcd_command(LCD_SET_DISPLAY | LCD_DISPLAY_ON | LCD_CURSOR_ON | LCD_BLINKING_OFF);
	lcd_command(LCD_SET_ENTRY | LCD_ENTRY_INCREASE | LCD_ENTRY_NOSHIFT);

	lcd_clear();

	debug_fstr("o hai ");
}

void debug_char(uint8_t data) {
	lcd_data(data);
}

void debug_str(const char * wat) {
	while (*wat != 0) {
		debug_char(*wat);
		wat++;
	}
}

void _debug_fstr(const __flash char *wat) {
	while (*wat != 0) {
		debug_char(*wat);
		wat++;
	}
}

static void debug_nibble(uint8_t wat) {
	if (wat < 0xa) {
		debug_char('0' + wat);
	} else {
		debug_char('a' + wat - 0xa);
	}
}

void debug_hex8(uint8_t wat) {
	debug_nibble(wat >> 4);
	debug_nibble(wat & 0xf);
}

void debug_hex16(uint16_t wat) {
	debug_hex8(wat >> 8);
	debug_hex8(wat & 0xff);
}

void debug_dec16(uint16_t wat) {
	if (wat >= 10000) goto n5;
	if (wat >= 1000) goto n4;
	if (wat >= 100) goto n3;
	if (wat >= 10) goto n2;
	goto n1;
	n5: debug_nibble(wat / 10000);
	wat %= 10000;
	n4: debug_nibble(wat / 1000);
	wat %= 1000;
	n3: debug_nibble(wat / 100);
	wat %= 100;
	n2: debug_nibble(wat / 10);
	wat %= 10;
	n1: debug_nibble(wat);
}

#else

void debug_init() {
	DDRB |= (1 << DBG_DATA) | (1 << DBG_SHIFT) | (1 << DBG_SYNC);
	PORTB &= ~( (1 << DBG_DATA) | (1 << DBG_SHIFT) | (1 << DBG_SYNC) );
}

void debug_char(uint8_t data) {
}

void debug_str(const char * wat) {
}

void _debug_fstr(const __flash char *wat) {
}

static void debug_nibble(uint8_t wat) {
}

void debug_hex8(uint8_t wat) {
}

void debug_hex16(uint16_t wat) {
}

void debug_dec16(uint16_t wat) {
}


#endif
