#include "pci/pci.h"
#include "pci/panic.h"
#include "pci/registers.h"

#include "console.h"
#include <util/delay.h>

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
#define IO_BASE 0
#define MEM_TXDESC 0xaa000000
#define MEM_RXDESC 0xbb000000

#define RTL_W8(reg,val8)   pci_io_write8(IO_BASE + (reg), (val8))
#define RTL_W16(reg,val16)   pci_io_write16(IO_BASE + (reg), (val16))
#define RTL_W32(reg,val32) pci_io_write32(IO_BASE + (reg), (val32))
#define RTL_R8(reg) pci_io_read8(IO_BASE + (reg))
#define RTL_R32(reg) pci_io_read32(IO_BASE + (reg))

void rtl8169_init() {
	/* Verify that this actually is a RTL8169 */
	if (pci_config_read32(PCIR_DEVVENDOR) != 0x816910ec) {
		panic("/Not a RTL8169");
	}

	console_fstr("r8169 ");

	/* set up I/O base address register */
	pci_config_write32(PCIR_BAR(0), 0xffffffff);
	if (pci_config_read32(PCIR_BAR(0)) != (0xffffff00 | PCIM_BAR_IO_SPACE)) {
		panic("/Unexpected I/O BAR values");
	}

	pci_config_write32(PCIR_BAR(0), IO_BASE);

	/* configure command register.
	 * card should respond to register accesses in I/O space (but not in
	 * memory space),
	 * card is allowed to read/write RAM as a bus master,
	 * card should assert PERR/SERR pins when necessary.
	 * at the same time, the status register is cleared by writing all 1
	 * to it
	 */
	pci_config_write32(PCIR_COMMAND, 0xffff0000
		| PCIM_CMD_PORTEN | PCIM_CMD_BUSMASTEREN
		| PCIM_CMD_PERRESPEN | PCIM_CMD_SERRESPEN);

	/* reset the card and wait for the reset to complete */
	RTL_W8(Command, Command_Reset);
	while (RTL_R8(Command) & Command_Reset) { }

	/* can dump MAC address now */
	for (int i = 0; i < 6; i++) {
		console_hex8(RTL_R8(IdReg0 + i));
		if (i == 2) { console_char(' '); }
	}
	console_fstr("\n");

	/* fill config registers */
	RTL_W8(Cmd9346, Cmd9346_WE);
	RTL_W32(RxConfig, RxConfig_RXFTH_No | RxConfig_MXDMA_Unlimited | RxConfig_AAP);
	RTL_W8(Command, Command_TxEn);
	RTL_W32(TxConfig, TxConfig_MXDMA_Unlimited | TxConfig_IFG1 | TxConfig_IFG0);
	RTL_W16(RxMaxSize, 2048);
	RTL_W8(TxEarlyTh, 0b111111);

	RTL_W32(TxDescNormal+0, MEM_TXDESC);
	RTL_W32(TxDescNormal+4, 0);
	RTL_W32(RxDesc+0, MEM_RXDESC);
	RTL_W32(RxDesc+4, 0);

	RTL_W8(Command, Command_RxEn | Command_TxEn);
	RTL_W8(Cmd9346, 0);
}


