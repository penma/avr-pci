#include "pci/pci.h"
#include "pci/panic.h"
#include "pci/registers.h"

#include "mii.h"

#include "console.h"
#include <util/delay.h>

typedef int bool;

/* RTL8139 registers */
#define IdReg0 0x00
#define MulticastReg0 0x08
#define TxAddr0 0x20
#define RxBuf 0x30
#define Command 0x37
#define  Command_ClrMask 0xe2
#define  Command_Reset (1 << 4)
#define  Command_RxEn  (1 << 3)
#define  Command_TxEn  (1 << 2)
#define  Command_RxBufEmpty (1 << 0)
#define RxBufPtr 0x38
#define RxBufAddr 0x3a
#define IntMask 0x3c
#define IntStatus 0x3e
#define  Int_SERR      (1 << 15)
#define  Int_TxDescNA  (1 << 7)
#define  Int_RxFOvf    (1 << 6)
#define  Int_PU_LC     (1 << 5)
#define  Int_RxDescNA  (1 << 4)
#define  Int_TxErr     (1 << 3)
#define  Int_TxOK      (1 << 2)
#define  Int_RxErr     (1 << 1)
#define  Int_RxOK      (1 << 0)
#define TxConfig 0x40
#define  TxConfig_IFG1 (1UL << 25)
#define  TxConfig_IFG0 (1UL << 24)
#define  TxConfig_IFG2 (1UL << 19)
#define  TxConfig_CRC  (1UL << 16)
#define  TxConfig_MXDMA_Unlimited (0b111 << 8)
#define RxConfig 0x44
#define  RxConfig_RXFTH_No (0b111UL << 13)
#define  RxConfig_MXDMA_Unlimited (0b111 << 8)
#define  RxConfig_AB (1 << 3)
#define  RxConfig_AM (1 << 2)
#define  RxConfig_APM (1 << 1)
#define  RxConfig_AAP (1 << 0)
#define MissedPkt 0x4c
#define Cmd9346 0x50
#define  Cmd9346_Normal 0
#define  Cmd9346_Autoload (1 << 6)
#define  Cmd9346_WE ((1<<7) | (1<<6))
#define Config0 0x51
#define Config1 0x52
#define Config3 0x59
#define Config4 0x5a
#define Config5 0xd8
#define FifoTms 0x70
#define Cscr 0x74
#define Para78 0x78
#define FlashReg 0xd4
#define Para7c 0x7c

#define BasicModeCtrl 0x62
#define BasicModeStatus 0x64
#define NWayAdvert 0x66
#define NWayLPAR 0x68
#define NWayExpansion 0x6a

/* I/O and Memory addresses that are used for accessing the card */
#define IO_BASE 0xcc000000
#define MEM_TXDESC 0xaa000000
#define MEM_RXDESC 0xbb000000

#define RTL_W8(reg,val8)   pci_mem_write8(IO_BASE + (reg), (val8))
#define RTL_W16(reg,val16)   pci_mem_write16(IO_BASE + (reg), (val16))
#define RTL_W32(reg,val32) pci_mem_write32(IO_BASE + (reg), (val32))
#define RTL_R8(reg) pci_mem_read8(IO_BASE + (reg))
#define RTL_R16(reg) pci_mem_read16(IO_BASE + (reg))
#define RTL_R32(reg) pci_mem_read32(IO_BASE + (reg))

enum chip {
	R8139, R8139K, R8139A, R8139AG, R8139B, R8130, R8139C, R8100, R8139D, R8101
};

static uint8_t mac_ver;

static uint8_t reg_map(uint8_t loc) {
	switch (loc) {
	case 0: return BasicModeCtrl;
	case 1: return BasicModeStatus;
	case 4: return NWayAdvert;
	case 5: return NWayLPAR;
	case 6: return NWayExpansion;
	default: panic("Invalid PHY register");
	}
}

static void writephy(uint8_t loc, uint16_t val) {
	if (loc == 0) {
		RTL_W8(Cmd9346, Cmd9346_WE);
		RTL_W16(BasicModeCtrl, val);
		RTL_W8(Cmd9346, 0);
	} else {
		RTL_W16(reg_map(loc), val);
	}
}

static uint16_t readphy(uint8_t loc) {
	return RTL_R16(reg_map(loc));
}

static void detect_mac_version() {
	uint32_t txc = RTL_R32(TxConfig);
	switch (txc & 0x7cc00000) {
	case 0x74400000: mac_ver = 0x01; console_fstr("D "); break; /* D */
	default: mac_ver = 0xff; break;
	}

	if (mac_ver == 0xff) {
		console_fstr("MAC ver detection failed\n");
	}
}

static void rtl8139_chip_reset() {
	RTL_W8(Command, Command_Reset);
	while (RTL_R8(Command) & Command_Reset) { }
}

void rtl8139_init() {
	/* Verify that this actually is a RTL8139 */
	if (pci_config_read32(PCIR_DEVVENDOR) != 0x813910ec) {
		panic("/Not a RTL8139");
	}

	console_fstr("r8139");

	/* set up I/O base address register */
	pci_config_write32(PCIR_BAR(0), 0xffffffff);
	if (pci_config_read32(PCIR_BAR(0)) != (0xffffff00 | PCIM_BAR_IO_SPACE)) {
		panic("/Unexpected I/O BAR values");
	}
	pci_config_write32(PCIR_BAR(0), IO_BASE);

	pci_config_write32(PCIR_BAR(1), 0xffffffff);
	if (pci_config_read32(PCIR_BAR(1)) != (0xffffff00 | PCIM_BAR_MEM_SPACE)) {
		panic("/Unexpected I/O BAR values");
	}
	pci_config_write32(PCIR_BAR(1), IO_BASE);


	/* configure command register.
	 * card should respond to register accesses in I/O space (but not in
	 * memory space),
	 * card is allowed to read/write RAM as a bus master,
	 * card should assert PERR/SERR pins when necessary.
	 * at the same time, the status register is cleared by writing all 1
	 * to it
	 */
	pci_config_write32(PCIR_COMMAND, 0xffff0000
		| PCIM_CMD_MEMEN /*PORTEN*/ | PCIM_CMD_BUSMASTEREN
		| PCIM_CMD_PERRESPEN | PCIM_CMD_SERRESPEN);

	/* init_one or parts of it */
	detect_mac_version();

	/* (lo)lwake */

	uint8_t tmp8 = RTL_R8(Config1);
	uint8_t new_tmp8 = tmp8;
	new_tmp8 &= ~0x10;
	new_tmp8 |= 0x01;
	if (new_tmp8 != tmp8) {
		console_fstr("W1");
		RTL_W8(Cmd9346, Cmd9346_WE);
		RTL_W8(Config1, tmp8);
		RTL_W8(Cmd9346, 0);
	}

	tmp8 = RTL_R8(Config4);
	if (tmp8 & (1 << 2)) {
		console_fstr("W2");
		RTL_W8(Cmd9346, Cmd9346_WE);
		RTL_W8(Config4, tmp8 & ~(1<<2));
		RTL_W8(Cmd9346, 0);
	}

	rtl8139_chip_reset();

	/* can dump MAC address now */
	for (int i = 0; i < 6; i++) {
		console_hex8(RTL_R8(IdReg0 + i));
		//if (i == 2) { console_char(' '); }
	}

	rtl8139_chip_reset();

	RTL_W8(0x5b, 'R');

		RTL_W8(Cmd9346, Cmd9346_WE);
	writephy(MII_ANAR, readphy(MII_ANAR) /* & ~ANAR_TX_FD & ~ANAR_TX */);
	writephy(MII_BMCR, BMCR_AUTOEN | BMCR_STARTNEG);
		RTL_W8(Cmd9346, 0);

	RTL_W32(RxBuf, 0x66000000);
	RTL_W8(Command, Command_RxEn);
	RTL_W8(Command, Command_RxEn | Command_TxEn);
	RTL_W32(TxConfig, 0x03000000 | 0x00000700);
	RTL_W32(RxConfig, /* RxConfig_RXFTH_No */ (0b011UL << 13) | (0b11 << 11) | RxConfig_MXDMA_Unlimited | RxConfig_AAP | RxConfig_AB | RxConfig_AM | RxConfig_APM | 0b111111);
	RTL_W32(MulticastReg0, 0xffffffff);
	RTL_W32(MulticastReg0+4, 0xffffffff);
	RTL_W32(MissedPkt, 0);
	RTL_W16(IntStatus, 0x20);
	RTL_W8(Command, Command_RxEn | Command_TxEn);

	RTL_W16(RxBufPtr, 0x800);

	while (1) {
		console_reset();

		uint16_t is = RTL_R16(IntStatus);
		console_fstr("I:Rx[");
		if (is & 0x1) { console_fstr("Ok"); } else { console_fstr("  "); }
		if (is & 0x2) { console_fstr("Er"); } else { console_fstr("  "); }
		if (is & 0x10) { console_fstr("Bo"); } else { console_fstr("  "); }
		if (is & 0x40) { console_fstr("Fo"); }else { console_fstr("  "); }
		console_fstr("]");
		if (is & 0x20) { console_fstr("Lc"); } else { console_fstr("  "); }
		if (is & ~(0x1|0x2|0x10|0x20|0x40)) { console_hex8(is & ~(0x1|0x2|0x10|0x20|0x40)); }

		RTL_W16(IntStatus, 0x20);

		console_fstr("\n");
		uint8_t cmd = RTL_R8(Command);
		console_fstr("C:");
		if (cmd & Command_RxEn) { console_fstr("Rx"); } else { console_fstr("  "); }
		if (cmd & Command_TxEn) { console_fstr("Tx"); } else { console_fstr("  "); }
		if (cmd & Command_RxBufEmpty) { console_fstr("Be"); } else { console_fstr("  "); }
		if (cmd & ~(Command_RxEn | Command_TxEn | Command_RxBufEmpty)) { console_hex8(cmd & ~(Command_RxEn | Command_TxEn | Command_RxBufEmpty)); } else { console_fstr("  "); }

		console_fstr(" B"); console_hex16(RTL_R16(RxBufPtr));
		console_fstr(" ");
		console_hex8(RTL_R8(0x36));
		console_fstr("\n");

		console_fstr("Con:");
		console_hex8(RTL_R8(Config0)); console_char(' ');
		console_hex8(RTL_R8(Config1)); console_char(' ');
		console_hex8(RTL_R8(Config3)); console_char(' ');
		console_hex8(RTL_R8(Config4)); console_char(' ');
		console_hex8(RTL_R8(Config5)); console_char(' ');
		if (PINB & 0x1) {
			console_char('R');
		} else {
			console_char('r');
		}

		_delay_ms(500);
	}
}


