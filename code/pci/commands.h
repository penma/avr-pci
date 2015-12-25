#ifndef PCI_COMMANDS_H
#define PCI_COMMANDS_H

#define CMD_IO_READ       0b0010
#define CMD_IO_WRITE      0b0011
#define CMD_MEM_READ      0b0110
#define CMD_MEM_WRITE     0b0111
#define CMD_CONFIG_READ   0b1010
#define CMD_CONFIG_WRITE  0b1011
#define CMD_MEM_READ_MULT 0b1100 /* we only expect this from targets */
#define CMD_MEM_READ_LINE 0b1110 /* same */
#define CMD_MEM_WRITE_INV 0b1111 /* same */

#endif
