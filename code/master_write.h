#ifndef MASTER_WRITE_H
#define MASTER_WRITE_H

#include <stdint.h>

void master_write(uint32_t addr, uint8_t cmd, uint8_t be, uint32_t value);

#endif

