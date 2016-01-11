/* freebsd sys/dev/mii/mii.h  - TODO copyright notice */

#ifndef MII_H
#define MII_H

#define	MII_BMCR	0x00 /* Basic mode control register (rw) */
#define	BMCR_RESET	0x8000 /* reset */
#define	BMCR_LOOP	0x4000 /* loopback */
#define	BMCR_SPEED0	0x2000 /* speed selection (LSB) */
#define	BMCR_AUTOEN	0x1000 /* autonegotiation enable */
#define	BMCR_PDOWN	0x0800 /* power down */
#define	BMCR_ISO	0x0400 /* isolate */
#define	BMCR_STARTNEG	0x0200 /* restart autonegotiation */
#define	BMCR_FDX	0x0100 /* Set duplex mode */
#define	BMCR_CTEST	0x0080 /* collision test */
#define	BMCR_SPEED1	0x0040 /* speed selection (MSB) */
#define	BMCR_S10	0x0000 /* 10 Mb/s */
#define	BMCR_S100	BMCR_SPEED0 /* 100 Mb/s */
#define	BMCR_S1000	BMCR_SPEED1 /* 1000 Mb/s */
#define	BMCR_SPEED(x) ((x) & (BMCR_SPEED0|BMCR_SPEED1))
#define	MII_BMSR	0x01 /* Basic mode status register (ro) */
#define	BMSR_100T4	0x8000 /* 100 base T4 capable */
#define	BMSR_100TXFDX	0x4000 /* 100 base Tx full duplex capable */
#define	BMSR_100TXHDX	0x2000 /* 100 base Tx half duplex capable */
#define	BMSR_10TFDX	0x1000 /* 10 base T full duplex capable */
#define	BMSR_10THDX	0x0800 /* 10 base T half duplex capable */
#define	BMSR_100T2FDX	0x0400 /* 100 base T2 full duplex capable */
#define	BMSR_100T2HDX	0x0200 /* 100 base T2 half duplex capable */
#define	BMSR_EXTSTAT	0x0100 /* Extended status in register 15 */
#define	BMSR_MFPS	0x0040 /* MII Frame Preamble Suppression */
#define	BMSR_ACOMP	0x0020 /* Autonegotiation complete */
#define	BMSR_RFAULT	0x0010 /* Link partner fault */
#define	BMSR_ANEG	0x0008 /* Autonegotiation capable */
#define	BMSR_LINK	0x0004 /* Link status */
#define	BMSR_JABBER	0x0002 /* Jabber detected */
#define	BMSR_EXTCAP	0x0001 /* Extended capability */

#define	MII_ANAR	0x04 /* Autonegotiation advertisement (rw) */
/* section 28.2.4.1 and 37.2.6.1 */
#define ANAR_NP	0x8000 /* Next page (ro) */
#define	ANAR_ACK	0x4000 /* link partner abilities acknowledged (ro) */
#define ANAR_RF	0x2000 /* remote fault (ro) */
/* Annex 28B.2 */
#define	ANAR_FC	0x0400 /* local device supports PAUSE */
#define ANAR_T4	0x0200 /* local device supports 100bT4 */
#define ANAR_TX_FD	0x0100 /* local device supports 100bTx FD */
#define ANAR_TX	0x0080 /* local device supports 100bTx */
#define ANAR_10_FD	0x0040 /* local device supports 10bT FD */
#define ANAR_10	0x0020 /* local device supports 10bT */
#define	ANAR_CSMA	0x0001 /* protocol selector CSMA/CD */
#define	ANAR_PAUSE_NONE	(0 << 10)
#define	ANAR_PAUSE_SYM	(1 << 10)
#define	ANAR_PAUSE_ASYM	(2 << 10)
#define	ANAR_PAUSE_TOWARDS	(3 << 10)
/* Annex 28D */
#define	ANAR_X_FD	0x0020 /* local device supports 1000BASE-X FD */
#define	ANAR_X_HD	0x0040 /* local device supports 1000BASE-X HD */
#define	ANAR_X_PAUSE_NONE	(0 << 7)
#define	ANAR_X_PAUSE_SYM	(1 << 7)
#define	ANAR_X_PAUSE_ASYM	(2 << 7)
#define	ANAR_X_PAUSE_TOWARDS	(3 << 7)

#define MII_GTCR     0x09    /* This is also the 1000baseT control register */
#define GTCR_ADV_1000TFDX 0x0200 /* adv. 1000baseT FDX */
#define GTCR_ADV_1000THDX 0x0100 /* adv. 1000baseT HDX */


#endif
