#ifndef CONSOLE_H
#define CONSOLE_H

#include <avr/pgmspace.h>

#ifdef LCD_SUPPORT

void console_reset();
void console_char(uint8_t);
void console_str(const char *);
#define console_fstr(x) _console_fstr(PSTR(x))
void _console_fstr(const __flash char *);
void console_hex8(uint8_t);
void console_hex16(uint16_t);
void console_dec16(uint16_t);

#define console_die(x) do { console_fstr(x); while (1) { }; } while (0)

#else

void console_reset();
void console_char(uint8_t);
void console_str(const char *);
void console_hex8(uint8_t);
void console_hex16(uint16_t);
void console_dec16(uint16_t);
#define console_fstr(x) do { } while (0)
#define console_die(x) do { } while (0)

#endif

#endif
