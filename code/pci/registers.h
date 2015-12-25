#ifndef PCI_REGISTERS_H
#define PCI_REGISTERS_H

/* stripped down version of freebsd's sys/dev/pci/pcireg.h */

/*-
 * Copyright (c) 1997, Stefan Esser <se@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 *
 */

/*
 * PCIM_xxx: mask to locate subfield in register
 * PCIR_xxx: config register offset
 * PCIC_xxx: device class
 * PCIS_xxx: device subclass
 * PCIP_xxx: device programming interface
 * PCIV_xxx: PCI vendor ID (only required to fixup ancient devices)
 * PCID_xxx: device ID
 * PCIY_xxx: capability identification number
 * PCIZ_xxx: extended capability identification number
 */


/* PCI config header registers for all devices */

#define	PCIR_DEVVENDOR	0x00
#define	PCIR_VENDOR	0x00
#define	PCIR_DEVICE	0x02
#define	PCIR_COMMAND	0x04
#define	PCIM_CMD_PORTEN		0x0001
#define	PCIM_CMD_MEMEN		0x0002
#define	PCIM_CMD_BUSMASTEREN	0x0004
#define	PCIM_CMD_SPECIALEN	0x0008
#define	PCIM_CMD_MWRICEN	0x0010
#define	PCIM_CMD_PERRESPEN	0x0040
#define	PCIM_CMD_SERRESPEN	0x0100
#define	PCIM_CMD_BACKTOBACK	0x0200
#define	PCIM_CMD_INTxDIS	0x0400
#define	PCIR_STATUS	0x06
#define	PCIM_STATUS_INTxSTATE	0x0008
#define	PCIM_STATUS_CAPPRESENT	0x0010
#define	PCIM_STATUS_66CAPABLE	0x0020
#define	PCIM_STATUS_BACKTOBACK	0x0080
#define	PCIM_STATUS_MDPERR	0x0100
#define	PCIM_STATUS_SEL_FAST	0x0000
#define	PCIM_STATUS_SEL_MEDIMUM	0x0200
#define	PCIM_STATUS_SEL_SLOW	0x0400
#define	PCIM_STATUS_SEL_MASK	0x0600
#define	PCIM_STATUS_STABORT	0x0800
#define	PCIM_STATUS_RTABORT	0x1000
#define	PCIM_STATUS_RMABORT	0x2000
#define	PCIM_STATUS_SERR	0x4000
#define	PCIM_STATUS_PERR	0x8000
#define	PCIR_REVID	0x08
#define	PCIR_PROGIF	0x09
#define	PCIR_SUBCLASS	0x0a
#define	PCIR_CLASS	0x0b
#define	PCIR_CACHELNSZ	0x0c
#define	PCIR_LATTIMER	0x0d
#define	PCIR_HDRTYPE	0x0e
#define	PCIM_HDRTYPE		0x7f
#define	PCIM_HDRTYPE_NORMAL	0x00
#define	PCIM_HDRTYPE_BRIDGE	0x01
#define	PCIM_HDRTYPE_CARDBUS	0x02
#define	PCIM_MFDEV		0x80
#define	PCIR_BIST	0x0f

/* config registers for header type 0 devices */

#define	PCIR_BARS	0x10
#define	PCIR_BAR(x)		(PCIR_BARS + (x) * 4)
#define	PCIR_MAX_BAR_0		5
#define	PCI_RID2BAR(rid)	(((rid) - PCIR_BARS) / 4)
#define	PCI_BAR_IO(x)		(((x) & PCIM_BAR_SPACE) == PCIM_BAR_IO_SPACE)
#define	PCI_BAR_MEM(x)		(((x) & PCIM_BAR_SPACE) == PCIM_BAR_MEM_SPACE)
#define	PCIM_BAR_SPACE		0x00000001
#define	PCIM_BAR_MEM_SPACE	0
#define	PCIM_BAR_IO_SPACE	1
#define	PCIM_BAR_MEM_TYPE	0x00000006
#define	PCIM_BAR_MEM_32		0
#define	PCIM_BAR_MEM_1MB	2	/* Locate below 1MB in PCI <= 2.1 */
#define	PCIM_BAR_MEM_64		4
#define	PCIM_BAR_MEM_PREFETCH	0x00000008
#define	PCIM_BAR_MEM_BASE	0xfffffffffffffff0ULL
#define	PCIM_BAR_IO_RESERVED	0x00000002
#define	PCIM_BAR_IO_BASE	0xfffffffc
#define	PCIR_CIS	0x28
#define	PCIM_CIS_ASI_MASK	0x00000007
#define	PCIM_CIS_ASI_CONFIG	0
#define	PCIM_CIS_ASI_BAR0	1
#define	PCIM_CIS_ASI_BAR1	2
#define	PCIM_CIS_ASI_BAR2	3
#define	PCIM_CIS_ASI_BAR3	4
#define	PCIM_CIS_ASI_BAR4	5
#define	PCIM_CIS_ASI_BAR5	6
#define	PCIM_CIS_ASI_ROM	7
#define	PCIM_CIS_ADDR_MASK	0x0ffffff8
#define	PCIM_CIS_ROM_MASK	0xf0000000
#define	PCIM_CIS_CONFIG_MASK	0xff
#define	PCIR_SUBVEND_0	0x2c
#define	PCIR_SUBDEV_0	0x2e
#define	PCIR_BIOS	0x30
#define	PCIM_BIOS_ENABLE	0x01
#define	PCIM_BIOS_ADDR_MASK	0xfffff800
#define	PCIR_CAP_PTR	0x34
#define	PCIR_INTLINE	0x3c
#define	PCIR_INTPIN	0x3d
#define	PCIR_MINGNT	0x3e
#define	PCIR_MAXLAT	0x3f

/* PCI device class, subclass and programming interface definitions */

#define	PCIC_OLD	0x00
#define	PCIS_OLD_NONVGA		0x00
#define	PCIS_OLD_VGA		0x01

#define	PCIC_STORAGE	0x01
#define	PCIS_STORAGE_SCSI	0x00
#define	PCIS_STORAGE_IDE	0x01
#define	PCIP_STORAGE_IDE_MODEPRIM	0x01
#define	PCIP_STORAGE_IDE_PROGINDPRIM	0x02
#define	PCIP_STORAGE_IDE_MODESEC	0x04
#define	PCIP_STORAGE_IDE_PROGINDSEC	0x08
#define	PCIP_STORAGE_IDE_MASTERDEV	0x80
#define	PCIS_STORAGE_FLOPPY	0x02
#define	PCIS_STORAGE_IPI	0x03
#define	PCIS_STORAGE_RAID	0x04
#define	PCIS_STORAGE_ATA_ADMA	0x05
#define	PCIS_STORAGE_SATA	0x06
#define	PCIP_STORAGE_SATA_AHCI_1_0	0x01
#define	PCIS_STORAGE_SAS	0x07
#define	PCIS_STORAGE_NVM	0x08
#define	PCIP_STORAGE_NVM_NVMHCI_1_0	0x01
#define	PCIP_STORAGE_NVM_ENTERPRISE_NVMHCI_1_0	0x02
#define	PCIS_STORAGE_OTHER	0x80

#define	PCIC_NETWORK	0x02
#define	PCIS_NETWORK_ETHERNET	0x00
#define	PCIS_NETWORK_TOKENRING	0x01
#define	PCIS_NETWORK_FDDI	0x02
#define	PCIS_NETWORK_ATM	0x03
#define	PCIS_NETWORK_ISDN	0x04
#define	PCIS_NETWORK_WORLDFIP	0x05
#define	PCIS_NETWORK_PICMG	0x06
#define	PCIS_NETWORK_OTHER	0x80

#define	PCIC_DISPLAY	0x03
#define	PCIS_DISPLAY_VGA	0x00
#define	PCIS_DISPLAY_XGA	0x01
#define	PCIS_DISPLAY_3D		0x02
#define	PCIS_DISPLAY_OTHER	0x80

#define	PCIC_MULTIMEDIA	0x04
#define	PCIS_MULTIMEDIA_VIDEO	0x00
#define	PCIS_MULTIMEDIA_AUDIO	0x01
#define	PCIS_MULTIMEDIA_TELE	0x02
#define	PCIS_MULTIMEDIA_HDA	0x03
#define	PCIS_MULTIMEDIA_OTHER	0x80

#define	PCIC_MEMORY	0x05
#define	PCIS_MEMORY_RAM		0x00
#define	PCIS_MEMORY_FLASH	0x01
#define	PCIS_MEMORY_OTHER	0x80

#define	PCIC_BRIDGE	0x06
#define	PCIS_BRIDGE_HOST	0x00
#define	PCIS_BRIDGE_ISA		0x01
#define	PCIS_BRIDGE_EISA	0x02
#define	PCIS_BRIDGE_MCA		0x03
#define	PCIS_BRIDGE_PCI		0x04
#define	PCIP_BRIDGE_PCI_SUBTRACTIVE	0x01
#define	PCIS_BRIDGE_PCMCIA	0x05
#define	PCIS_BRIDGE_NUBUS	0x06
#define	PCIS_BRIDGE_CARDBUS	0x07
#define	PCIS_BRIDGE_RACEWAY	0x08
#define	PCIS_BRIDGE_PCI_TRANSPARENT 0x09
#define	PCIS_BRIDGE_INFINIBAND	0x0a
#define	PCIS_BRIDGE_OTHER	0x80

#define	PCIC_SIMPLECOMM	0x07
#define	PCIS_SIMPLECOMM_UART	0x00
#define	PCIP_SIMPLECOMM_UART_8250	0x00
#define	PCIP_SIMPLECOMM_UART_16450A	0x01
#define	PCIP_SIMPLECOMM_UART_16550A	0x02
#define	PCIP_SIMPLECOMM_UART_16650A	0x03
#define	PCIP_SIMPLECOMM_UART_16750A	0x04
#define	PCIP_SIMPLECOMM_UART_16850A	0x05
#define	PCIP_SIMPLECOMM_UART_16950A	0x06
#define	PCIS_SIMPLECOMM_PAR	0x01
#define	PCIS_SIMPLECOMM_MULSER	0x02
#define	PCIS_SIMPLECOMM_MODEM	0x03
#define	PCIS_SIMPLECOMM_GPIB	0x04
#define	PCIS_SIMPLECOMM_SMART_CARD 0x05
#define	PCIS_SIMPLECOMM_OTHER	0x80

#define	PCIC_BASEPERIPH	0x08
#define	PCIS_BASEPERIPH_PIC	0x00
#define	PCIP_BASEPERIPH_PIC_8259A	0x00
#define	PCIP_BASEPERIPH_PIC_ISA		0x01
#define	PCIP_BASEPERIPH_PIC_EISA	0x02
#define	PCIP_BASEPERIPH_PIC_IO_APIC	0x10
#define	PCIP_BASEPERIPH_PIC_IOX_APIC	0x20
#define	PCIS_BASEPERIPH_DMA	0x01
#define	PCIS_BASEPERIPH_TIMER	0x02
#define	PCIS_BASEPERIPH_RTC	0x03
#define	PCIS_BASEPERIPH_PCIHOT	0x04
#define	PCIS_BASEPERIPH_SDHC	0x05
#define	PCIS_BASEPERIPH_IOMMU	0x06
#define	PCIS_BASEPERIPH_OTHER	0x80

#define	PCIC_INPUTDEV	0x09
#define	PCIS_INPUTDEV_KEYBOARD	0x00
#define	PCIS_INPUTDEV_DIGITIZER	0x01
#define	PCIS_INPUTDEV_MOUSE	0x02
#define	PCIS_INPUTDEV_SCANNER	0x03
#define	PCIS_INPUTDEV_GAMEPORT	0x04
#define	PCIS_INPUTDEV_OTHER	0x80

#define	PCIC_DOCKING	0x0a
#define	PCIS_DOCKING_GENERIC	0x00
#define	PCIS_DOCKING_OTHER	0x80

#define	PCIC_PROCESSOR	0x0b
#define	PCIS_PROCESSOR_386	0x00
#define	PCIS_PROCESSOR_486	0x01
#define	PCIS_PROCESSOR_PENTIUM	0x02
#define	PCIS_PROCESSOR_ALPHA	0x10
#define	PCIS_PROCESSOR_POWERPC	0x20
#define	PCIS_PROCESSOR_MIPS	0x30
#define	PCIS_PROCESSOR_COPROC	0x40

#define	PCIC_SERIALBUS	0x0c
#define	PCIS_SERIALBUS_FW	0x00
#define	PCIS_SERIALBUS_ACCESS	0x01
#define	PCIS_SERIALBUS_SSA	0x02
#define	PCIS_SERIALBUS_USB	0x03
#define	PCIP_SERIALBUS_USB_UHCI		0x00
#define	PCIP_SERIALBUS_USB_OHCI		0x10
#define	PCIP_SERIALBUS_USB_EHCI		0x20
#define	PCIP_SERIALBUS_USB_XHCI		0x30
#define	PCIP_SERIALBUS_USB_DEVICE	0xfe
#define	PCIS_SERIALBUS_FC	0x04
#define	PCIS_SERIALBUS_SMBUS	0x05
#define	PCIS_SERIALBUS_INFINIBAND 0x06
#define	PCIS_SERIALBUS_IPMI	0x07
#define	PCIP_SERIALBUS_IPMI_SMIC	0x00
#define	PCIP_SERIALBUS_IPMI_KCS		0x01
#define	PCIP_SERIALBUS_IPMI_BT		0x02
#define	PCIS_SERIALBUS_SERCOS	0x08
#define	PCIS_SERIALBUS_CANBUS	0x09

#define	PCIC_WIRELESS	0x0d
#define	PCIS_WIRELESS_IRDA	0x00
#define	PCIS_WIRELESS_IR	0x01
#define	PCIS_WIRELESS_RF	0x10
#define	PCIS_WIRELESS_BLUETOOTH	0x11
#define	PCIS_WIRELESS_BROADBAND	0x12
#define	PCIS_WIRELESS_80211A	0x20
#define	PCIS_WIRELESS_80211B	0x21
#define	PCIS_WIRELESS_OTHER	0x80

#define	PCIC_INTELLIIO	0x0e
#define	PCIS_INTELLIIO_I2O	0x00

#define	PCIC_SATCOM	0x0f
#define	PCIS_SATCOM_TV		0x01
#define	PCIS_SATCOM_AUDIO	0x02
#define	PCIS_SATCOM_VOICE	0x03
#define	PCIS_SATCOM_DATA	0x04

#define	PCIC_CRYPTO	0x10
#define	PCIS_CRYPTO_NETCOMP	0x00
#define	PCIS_CRYPTO_ENTERTAIN	0x10
#define	PCIS_CRYPTO_OTHER	0x80

#define	PCIC_DASP	0x11
#define	PCIS_DASP_DPIO		0x00
#define	PCIS_DASP_PERFCNTRS	0x01
#define	PCIS_DASP_COMM_SYNC	0x10
#define	PCIS_DASP_MGMT_CARD	0x20
#define	PCIS_DASP_OTHER		0x80

#define	PCIC_OTHER	0xff

#endif