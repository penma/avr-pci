#ifndef DEBUG_H
#define DEBUG_H

#include <avr/pgmspace.h>

void debug_init();
void debug_char(uint8_t);
void debug_str(const char *);
#define debug_fstr(x) _debug_fstr(PSTR(x))
void _debug_fstr(const __flash char *);
void debug_hex8(uint8_t);
void debug_hex16(uint16_t);
void debug_dec16(uint16_t);

void lcd_clear();
void lcd_home();
void lcd_setcursor(uint8_t x, uint8_t y);
void lcd_generatechar(uint8_t code, const uint8_t *data);

#define debug_die(x) do { debug_fstr(x); while (1) { }; } while (0)

#endif
