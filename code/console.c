
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <string.h>

#include "console.h"
#include "debug.h"

#ifdef LCD_SUPPORT

#define LINES 4
#define COLS 20
static uint8_t textmem[LINES*COLS];
static uint8_t cur_x, cur_y;
static uint8_t debug_initialized = 0;

static uint8_t delayed_newline = 0;

static void refresh_screen() {
	for (int y = 0; y < LINES; y++) {
		lcd_setcursor(0, y);
		for (int x = 0; x < COLS; x++) {
			debug_char(textmem[y*COLS + x]);
		}
	}
	lcd_setcursor(cur_x, cur_y);
}

static void scroll() {
	memmove(textmem, textmem + COLS, (LINES-1) * COLS);
	memset(textmem + (LINES-1) * COLS, ' ', COLS);
}

void console_reset() {
	if (!debug_initialized) {
		debug_init();
		debug_initialized++;
	}
	memset(textmem, ' ', LINES*COLS);
	cur_x = 0;
	cur_y = 0;
	delayed_newline = 0;
	refresh_screen();
}

void console_char(uint8_t data) {
	if (delayed_newline && data != '\n') {
		scroll();
		cur_x = 0;
		refresh_screen();
		delayed_newline = 0;
	}

	if (data != '\n') {
		debug_char(data);
		textmem[cur_y*COLS + cur_x] = data;
		cur_x++;
	} else {
		cur_x = COLS;
	}

	if (cur_x >= COLS) {
		cur_x = 0;
		cur_y++;
		if (cur_y >= LINES) {
			cur_y--;
			delayed_newline = 1;
		}
		lcd_setcursor(cur_x, cur_y);
	}
}

void console_str(const char *wat) {
	while (*wat != 0) {
		console_char(*wat);
		wat++;
	}
}

void _console_fstr(const __flash char *wat) {
	while (*wat != 0) {
		console_char(*wat);
		wat++;
	}
}

static void console_nibble(uint8_t wat) {
	if (wat < 0xa) {
		console_char('0' + wat);
	} else {
		console_char('a' + wat - 0xa);
	}
}

void console_hex8(uint8_t wat) {
	console_nibble(wat >> 4);
	console_nibble(wat & 0xf);
}

void console_hex16(uint16_t wat) {
	console_hex8(wat >> 8);
	console_hex8(wat & 0xff);
}

void console_dec16(uint16_t wat) {
	if (wat >= 10000) goto n5;
	if (wat >= 1000) goto n4;
	if (wat >= 100) goto n3;
	if (wat >= 10) goto n2;
	goto n1;
	n5: console_nibble(wat / 10000);
	wat %= 10000;
	n4: console_nibble(wat / 1000);
	wat %= 1000;
	n3: console_nibble(wat / 100);
	wat %= 100;
	n2: console_nibble(wat / 10);
	wat %= 10;
	n1: console_nibble(wat);
}

#else

void console_reset() { }
void console_char(uint8_t wat) { }
void console_str(const char *wat) { }
void console_hex8(uint8_t wat) { }
void console_hex16(uint16_t wat) { }
void console_dec16(uint16_t wat) { }

#endif
