#include "pci/pci.h"
#include "pci/panic.h"
#include "pci/registers.h"

#include "mii.h"

#include "console.h"
#include <util/delay.h>

typedef int bool;

/* RTL8169 registers */
#define IdReg0 0x00
#define MulticastReg0 0x08
#define TxDescNormal 0x20
#define TxDescHigh 0x28
#define Command 0x37
#define  Command_Reset (1 << 4)
#define  Command_RxEn  (1 << 3)
#define  Command_TxEn  (1 << 2)
#define TPPoll 0x38
#define  TPPoll_NPQ (1 << 6)
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
#define Cmd9346 0x50
#define  Cmd9346_Normal 0
#define  Cmd9346_Autoload (1 << 6)
#define  Cmd9346_WE ((1<<7) | (1<<6))
#define Config0 0x51
#define Config1 0x52
#define Config2 0x53
#define Config3 0x54
#define  Config3_GNTSel (1 << 7)
#define Config4 0x55
#define Config5 0x56
#define PhyAccess 0x60
#define  PhyAccess_Write (1UL << 31)
#define  PhyAccess_AddrSh 16
#define  PhyAccess_DataSh 0
#define PhyStatus 0x6c
#define  PhyStatus_EnTBI (1 << 7)
#define  PhyStatus_1000MF (1 << 4)
#define  PhyStatus_100M (1 << 3)
#define  PhyStatus_10M (1 << 2)
#define  PhyStatus_LinkSts (1 << 1)
#define  PhyStatus_FullDup (1 << 0)
#define RxMaxSize 0xda
#define CPCommand 0xe0
#define  CPCommand_Endian (1 << 9)
#define  CPCommand_RxVLAN (1 << 6)
#define  CPCommand_RxChksum (1 << 5)
#define RxDesc 0xe4
#define TxEarlyTh 0xec

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

/* similar to Linux driver:
 * 01 = 8169
 * 02 = 8169s
 * 03 = 8110s
 * 04 = 8169/8110sb
 * 05 = 8169/8110sc
 * 06 = 8169/8110sc
 * ff = unknown
 */
static uint8_t mac_ver;

static void writephy(uint8_t loc, uint16_t val) {
	/*
	RTL_W32(PhyAccess, PhyAccess_Write
		| ((uint32_t)(loc & 0x1f) << PhyAccess_AddrSh)
		| (uint32_t)(val << PhyAccess_DataSh));

	while (RTL_R32(PhyAccess) & PhyAccess_Write) { _delay_us(25); }
	_delay_us(20);
	*/
	console_fstr("B:");
	console_hex32(RTL_R32(PhyAccess));
	RTL_W32(PhyAccess, PhyAccess_Write
		| ((uint32_t)(loc & 0x1f) << PhyAccess_AddrSh)
		| (uint32_t)(val << PhyAccess_DataSh));
	while (1) {
		RTL_R32(0);
		uint32_t r = RTL_R32(PhyAccess);
		console_char(' ');
		console_hex32(r);
		if (!(r & PhyAccess_Write)) { break; }
		_delay_ms(1000);
	}
}

static uint16_t readphy(uint8_t loc) {
	console_fstr("B:");
	console_hex32(RTL_R32(PhyAccess));
	RTL_W32(PhyAccess, (uint32_t)(loc & 0x1f) << PhyAccess_AddrSh);
	while (1) {
		uint32_t r = RTL_R32(PhyAccess);
		console_char(' ');
		console_hex32(r);
		if ((r & PhyAccess_Write)) { return r & 0xffff; }
		_delay_ms(1000);

		/*
		uint32_t r = RTL_R32(PhyAccess);
		if (r & PhyAccess_Write) {
			return r & 0xffff;
		} */
	}
}

/* XMII */

static bool xmii_reset_pending() {
	return readphy(MII_BMCR) & BMCR_RESET;
}

static void xmii_reset_enable() {
	writephy(MII_BMCR, readphy(MII_BMCR) | BMCR_RESET);
}

static void xmii_set_speed() {
	/* currently fixed to autonegotiation enabled, all speeds, duplex */
	writephy(0x1f, 0x0000);

	writephy(MII_ANAR, readphy(MII_ANAR)
		| ANAR_TX_FD | ANAR_TX
		| ANAR_10_FD | ANAR_10
		| ANAR_FC | ANAR_PAUSE_ASYM);
	writephy(MII_GTCR, readphy(MII_GTCR)
		| GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX);
	writephy(MII_BMCR, BMCR_AUTOEN | BMCR_STARTNEG);

	if ((mac_ver == 0x02) || (mac_ver == 0x03)) {
		writephy(0x17, 0x2108);
		writephy(0x0e, 0x0000);
	}
}

static void detect_mac_version() {
	uint32_t txc = RTL_R32(TxConfig);
	switch (txc & 0xfc800000) {
	case 0x00000000: mac_ver = 0x01; break;
	case 0x00800000: mac_ver = 0x02; break;
	case 0x04000000: mac_ver = 0x03; break;
	case 0x10000000: mac_ver = 0x04; break;
	case 0x18000000: mac_ver = 0x05; break;
	case 0x98000000: mac_ver = 0x06; break;
	default: mac_ver = 0xff; break;
	}

	if (mac_ver == 0xff) {
		console_fstr("MAC ver detection failed\n");
	}
}

/* PHY init specialities */
struct _phy_reg {
	uint8_t loc;
	uint16_t val;
};

static const __flash struct _phy_reg phyinit_8169s[] = {
	{ 0x1f, 0x0001 }, { 0x06, 0x006e }, { 0x08, 0x0708 }, { 0x15, 0x4000 }, { 0x18, 0x65c7 }, { 0x1f, 0x0001 },
	{ 0x03, 0x00a1 }, { 0x02, 0x0008 }, { 0x01, 0x0120 }, { 0x00, 0x1000 }, { 0x04, 0x0800 }, { 0x04, 0x0000 },
	{ 0x03, 0xff41 }, { 0x02, 0xdf60 }, { 0x01, 0x0140 }, { 0x00, 0x0077 }, { 0x04, 0x7800 }, { 0x04, 0x7000 },
	{ 0x03, 0x802f }, { 0x02, 0x4f02 }, { 0x01, 0x0409 }, { 0x00, 0xf0f9 }, { 0x04, 0x9800 }, { 0x04, 0x9000 },
	{ 0x03, 0xdf01 }, { 0x02, 0xdf20 }, { 0x01, 0xff95 }, { 0x00, 0xba00 }, { 0x04, 0xa800 }, { 0x04, 0xa000 },
	{ 0x03, 0xff41 }, { 0x02, 0xdf20 }, { 0x01, 0x0140 }, { 0x00, 0x00bb }, { 0x04, 0xb800 }, { 0x04, 0xb000 },
	{ 0x03, 0xdf41 }, { 0x02, 0xdc60 }, { 0x01, 0x6340 }, { 0x00, 0x007d }, { 0x04, 0xd800 }, { 0x04, 0xd000 },
	{ 0x03, 0xdf01 }, { 0x02, 0xdf20 }, { 0x01, 0x100a }, { 0x00, 0xa0ff }, { 0x04, 0xf800 }, { 0x04, 0xf000 },
	{ 0x1f, 0x0000 }, { 0x0b, 0x0000 }, { 0x00, 0x9200 }
};

static void _writephy_batch(const __flash struct _phy_reg *regs, uint8_t len) {
	for (uint8_t i = 0; i < len; i++) {
		console_hex8(i);
		console_char(':');
		console_hex8(regs[i].loc);
		console_char('=');
		console_hex8(regs[i].val);

		writephy(regs[i].loc, regs[i].val);
	}
}
#define writephy_batch(regs) _writephy_batch((regs), sizeof((regs))/sizeof((regs)[0]))

static void hw_phy_config() {
	switch (mac_ver) {
	case 0x01: break;
	case 0x02: /* fallthrough */
	case 0x03: writephy_batch(phyinit_8169s); break;
	case 0x04: case 0x05: case 0x06: console_fstr("init_phy unimplemented\n"); break;
	default: break;
	}
}

static void phy_reset() {
	if (xmii_reset_pending()) { console_fstr("RP before"); } else { console_fstr("nrp before"); }
	xmii_reset_enable();
	if (xmii_reset_pending()) { console_fstr("RP after"); } else { console_fstr("nrp after"); }
	while (xmii_reset_pending()) { }
}

static bool tbi_enabled() {
	return (mac_ver == 0x01) && (RTL_R8(PhyStatus) & PhyStatus_EnTBI);
}

static void init_phy() {
	for (uint8_t x = 0; x < 10; x++) {
		console_hex8(readphy(x));
	}

	while (1) { }

	hw_phy_config();
	if (mac_ver != 0xff) {
		RTL_W8(0x82, 0x01);
	}

	// LATENCY TIMER
	// CACHE LINE SIZE
	
	if (mac_ver == 0x02) {
		RTL_W8(0x82, 0x01);
		writephy(0x0b, 0x0000);
	}

		console_fstr("about to reset phy\n");
	phy_reset();

	xmii_set_speed();
}

static void init_rxcfg() {
	if (mac_ver != 0xff) {
		RTL_W32(RxConfig, RxConfig_RXFTH_No | RxConfig_MXDMA_Unlimited);
	} else {
		console_fstr("TODO");
	}
}

static void hw_reset() {
	RTL_W8(Command, Command_Reset);
	while (RTL_R8(Command) & Command_Reset) { }
} /* TODO: rtl8169_hw_reset da oben rein portieren */

static void init_txcfg() {
	RTL_W32(TxConfig, TxConfig_MXDMA_Unlimited | TxConfig_IFG1 | TxConfig_IFG0);
}

static void hw_start() {
	if (mac_ver == 0x05) {
		console_fstr("TODO");
	}

	RTL_W8(Cmd9346, Cmd9346_WE);
	if ((mac_ver >= 0x01) && (mac_ver <= 0x04)) {
		RTL_W8(Command, Command_RxEn | Command_TxEn);
	}

	init_rxcfg();
	RTL_W8(TxEarlyTh, 0b111111);
	RTL_W16(RxMaxSize, 2048);

	if ((mac_ver >= 0x01) && (mac_ver <= 0x04)) {
		init_txcfg();
	}

	uint16_t cp_cmd = RTL_R16(CPCommand);
	RTL_W16(CPCommand, cp_cmd);
	cp_cmd |= (1 << 3);
	if ((mac_ver == 0x02) || (mac_ver == 0x03)) {
		cp_cmd |= (1 << 14);
	}
	RTL_W16(CPCommand, cp_cmd);

	if (mac_ver == 0x05) {
		/* TODO */
		RTL_W32(0x7c, 0x000fff00);
	} else if (mac_ver == 0x06) {
		RTL_W32(0x7c, 0x00ffff00);
	}

	/* TODO complete */

	/* TODO rx tx desc registers */

	if ((mac_ver == 0x05) || (mac_ver == 0x06)) {
		RTL_W8(Command, Command_RxEn | Command_TxEn);
		init_txcfg();
	}

	RTL_W8(Cmd9346, 0);

	/* ...? */
}



void rtl8169_init() {
	/* Verify that this actually is a RTL8169 */
	if (pci_config_read32(PCIR_DEVVENDOR) != 0x816910ec) {
		panic("/Not a RTL8169");
	}

	console_fstr("r8169");

	/* set up I/O base address register */
	/*
	pci_config_write32(PCIR_BAR(0), 0xffffffff);
	if (pci_config_read32(PCIR_BAR(0)) != (0xffffff00 | PCIM_BAR_IO_SPACE)) {
		panic("/Unexpected I/O BAR values");
	}
	pci_config_write32(PCIR_BAR(0), IO_BASE);
	*/
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
		| PCIM_CMD_MEMEN /* PORTEN */ | PCIM_CMD_BUSMASTEREN
		| PCIM_CMD_PERRESPEN | PCIM_CMD_SERRESPEN);

	/* init_one or parts of it */
	detect_mac_version();
	console_char('.');
	console_dec16(mac_ver);
	console_char(' ');
	init_rxcfg();
	hw_reset();

	RTL_W8(Cmd9346, Cmd9346_WE);
	RTL_W8(Config1, RTL_R8(Config1) | (1<<0));
	RTL_W8(Config5, RTL_R8(Config5) & 0b1110011);
	RTL_W8(Cmd9346, 0);

	/* can dump MAC address now */
	for (int i = 0; i < 6; i++) {
		console_hex8(RTL_R8(IdReg0 + i));
		//if (i == 2) { console_char(' '); }
	}

	init_phy();
	hw_start();

	/*
	RTL_W32(TxDescNormal+0, MEM_TXDESC);
	RTL_W32(TxDescNormal+4, 0);
	RTL_W32(RxDesc+0, MEM_RXDESC);
	RTL_W32(RxDesc+4, 0);
	*/

	while (1) {
		console_hex16(RTL_R16(IntStatus));
		console_char(' ');
		console_hex8(RTL_R8(PhyStatus));
		console_fstr("   ");
		_delay_ms(500);
	}
}


