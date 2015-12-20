#ifndef MASTER_TRANSACTION_H
#define MASTER_TRANSACTION_H

#include <stdint.h>

uint32_t master_read(uint32_t addr, uint8_t cmd, uint8_t be);
void master_write(uint32_t addr, uint8_t cmd, uint8_t be, uint32_t value);

#endif

