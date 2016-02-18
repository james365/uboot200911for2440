/*
 *  Copyright (C) 2004 PaulReynolds@lhsolutions.com
 *
 * (C) Copyright 2007
 * Stefan Roese, DENX Software Engineering, sr@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/processor.h>
#include <spd_sdram.h>
#include <ppc4xx_enet.h>
#include <netdev.h>

#ifdef CONFIG_SYS_INIT_SHOW_RESET_REG
void show_reset_reg(void);
#endif

DECLARE_GLOBAL_DATA_PTR;

int lcd_init(void);

int board_early_init_f (void)
{
	unsigned long reg;
	volatile unsigned int *GpioOdr;
	volatile unsigned int *GpioTcr;
	volatile unsigned int *GpioOr;

	/*-------------------------------------------------------------------------+
	  | Initialize EBC CONFIG
	  +-------------------------------------------------------------------------*/
	mtebc(EBC0_CFG, EBC_CFG_LE_UNLOCK |
	      EBC_CFG_PTD_ENABLE | EBC_CFG_RTC_64PERCLK |
	      EBC_CFG_ATC_PREVIOUS | EBC_CFG_DTC_PREVIOUS |
	      EBC_CFG_CTC_PREVIOUS | EBC_CFG_EMC_DEFAULT |
	      EBC_CFG_PME_DISABLE | EBC_CFG_PR_32);

	/*-------------------------------------------------------------------------+
	  | 64MB FLASH. Initialize bank 0 with default values.
	  +-------------------------------------------------------------------------*/
	mtebc(PB0AP, EBC_BXAP_BME_DISABLED|EBC_BXAP_TWT_ENCODE(15) |
	      EBC_BXAP_BCE_DISABLE |
	      EBC_BXAP_CSN_ENCODE(1) | EBC_BXAP_OEN_ENCODE(1) |
	      EBC_BXAP_WBN_ENCODE(1) | EBC_BXAP_WBF_ENCODE(1) |
	      EBC_BXAP_TH_ENCODE(3) | EBC_BXAP_RE_DISABLED |
	      EBC_BXAP_BEM_WRITEONLY |
	      EBC_BXAP_PEN_DISABLED);
	mtebc(PB0CR, EBC_BXCR_BAS_ENCODE(CONFIG_SYS_FLASH_BASE) |
	      EBC_BXCR_BS_64MB | EBC_BXCR_BU_RW|EBC_BXCR_BW_32BIT);

	/*-------------------------------------------------------------------------+
	  | FPGA. Initialize bank 1 with default values.
	  +-------------------------------------------------------------------------*/
	mtebc(PB1AP, EBC_BXAP_BME_DISABLED|EBC_BXAP_TWT_ENCODE(5) |
	      EBC_BXAP_BCE_DISABLE |
	      EBC_BXAP_CSN_ENCODE(1) | EBC_BXAP_OEN_ENCODE(1) |
	      EBC_BXAP_WBN_ENCODE(1) | EBC_BXAP_WBF_ENCODE(1) |
	      EBC_BXAP_TH_ENCODE(3) | EBC_BXAP_RE_DISABLED |
	      EBC_BXAP_BEM_WRITEONLY |
	      EBC_BXAP_PEN_DISABLED);
	mtebc(PB1CR, EBC_BXCR_BAS_ENCODE(0x41000000) |
	      EBC_BXCR_BS_1MB | EBC_BXCR_BU_RW | EBC_BXCR_BW_8BIT);

	/*-------------------------------------------------------------------------+
	  | LCM. Initialize bank 2 with default values.
	  +-------------------------------------------------------------------------*/
	mtebc(PB2AP, EBC_BXAP_BME_DISABLED | EBC_BXAP_TWT_ENCODE(64) |
	      EBC_BXAP_BCE_DISABLE |
	      EBC_BXAP_CSN_ENCODE(3) | EBC_BXAP_OEN_ENCODE(3) |
	      EBC_BXAP_WBN_ENCODE(3) | EBC_BXAP_WBF_ENCODE(3) |
	      EBC_BXAP_TH_ENCODE(7) | EBC_BXAP_RE_DISABLED |
	      EBC_BXAP_BEM_WRITEONLY |
	      EBC_BXAP_PEN_DISABLED);
	mtebc(PB2CR, EBC_BXCR_BAS_ENCODE(0x42000000) |
	      EBC_BXCR_BS_1MB | EBC_BXCR_BU_RW|EBC_BXCR_BW_8BIT);

	/*-------------------------------------------------------------------------+
	  | TMP. Initialize bank 3 with default values.
	  +-------------------------------------------------------------------------*/
	mtebc(PB3AP, EBC_BXAP_BME_DISABLED | EBC_BXAP_TWT_ENCODE(128) |
	      EBC_BXAP_BCE_DISABLE |
	      EBC_BXAP_CSN_ENCODE(3) | EBC_BXAP_OEN_ENCODE(3) |
	      EBC_BXAP_WBN_ENCODE(3) | EBC_BXAP_WBF_ENCODE(3) |
	      EBC_BXAP_TH_ENCODE(7) | EBC_BXAP_RE_DISABLED |
	      EBC_BXAP_BEM_WRITEONLY |
	      EBC_BXAP_PEN_DISABLED);
	mtebc(PB3CR, EBC_BXCR_BAS_ENCODE(0x48000000) |
	      EBC_BXCR_BS_64MB | EBC_BXCR_BU_RW | EBC_BXCR_BW_32BIT);

	/*-------------------------------------------------------------------------+
	  | Connector 4~7. Initialize bank 3~ 7 with default values.
	  +-------------------------------------------------------------------------*/
	mtebc(PB4AP,0);
	mtebc(PB4CR,0);
	mtebc(PB5AP,0);
	mtebc(PB5CR,0);
	mtebc(PB6AP,0);
	mtebc(PB6CR,0);
	mtebc(PB7AP,0);
	mtebc(PB7CR,0);

	/*--------------------------------------------------------------------
	 * Setup the interrupt controller polarities, triggers, etc.
	 *-------------------------------------------------------------------*/
	/*
	 * Because of the interrupt handling rework to handle 440GX interrupts
	 * with the common code, we needed to change names of the UIC registers.
	 * Here the new relationship:
	 *
	 * U-Boot name	440GX name
	 * -----------------------
	 * UIC0		UICB0
	 * UIC1		UIC0
	 * UIC2		UIC1
	 * UIC3		UIC2
	 */
	mtdcr (UIC1SR, 0xffffffff);	/* clear all */
	mtdcr (UIC1ER, 0x00000000);	/* disable all */
	mtdcr (UIC1CR, 0x00000009);	/* SMI & UIC1 crit are critical */
	mtdcr (UIC1PR, 0xfffffe13);	/* per ref-board manual */
	mtdcr (UIC1TR, 0x01c00008);	/* per ref-board manual */
	mtdcr (UIC1VR, 0x00000001);	/* int31 highest, base=0x000 */
	mtdcr (UIC1SR, 0xffffffff);	/* clear all */

	mtdcr (UIC2SR, 0xffffffff);	/* clear all */
	mtdcr (UIC2ER, 0x00000000);	/* disable all */
	mtdcr (UIC2CR, 0x00000000);	/* all non-critical */
	mtdcr (UIC2PR, 0xffffe0ff);	/* per ref-board manual */
	mtdcr (UIC2TR, 0x00ffc000);	/* per ref-board manual */
	mtdcr (UIC2VR, 0x00000001);	/* int31 highest, base=0x000 */
	mtdcr (UIC2SR, 0xffffffff);	/* clear all */

	mtdcr (UIC3SR, 0xffffffff);	/* clear all */
	mtdcr (UIC3ER, 0x00000000);	/* disable all */
	mtdcr (UIC3CR, 0x00000000);	/* all non-critical */
	mtdcr (UIC3PR, 0xffffffff);	/* per ref-board manual */
	mtdcr (UIC3TR, 0x00ff8c0f);	/* per ref-board manual */
	mtdcr (UIC3VR, 0x00000001);	/* int31 highest, base=0x000 */
	mtdcr (UIC3SR, 0xffffffff);	/* clear all */

	mtdcr (UIC0SR, 0xfc000000);	/* clear all */
	mtdcr (UIC0ER, 0x00000000);	/* disable all */
	mtdcr (UIC0CR, 0x00000000);	/* all non-critical */
	mtdcr (UIC0PR, 0xfc000000);	/* */
	mtdcr (UIC0TR, 0x00000000);	/* */
	mtdcr (UIC0VR, 0x00000001);	/* */

	/* Enable two GPIO 10~11 and TraceA signal */
	mfsdr(SDR0_PFC0,reg);
	reg |= 0x00300000;
	mtsdr(SDR0_PFC0,reg);

	mfsdr(SDR0_PFC1,reg);
	reg |= 0x00100000;
	mtsdr(SDR0_PFC1,reg);

	/* Set GPIO 10 and 11 as output */
	GpioOdr	= (volatile unsigned int*)(CONFIG_SYS_PERIPHERAL_BASE+0x718);
	GpioTcr = (volatile unsigned int*)(CONFIG_SYS_PERIPHERAL_BASE+0x704);
	GpioOr  = (volatile unsigned int*)(CONFIG_SYS_PERIPHERAL_BASE+0x700);

	*GpioOdr &= ~(0x00300000);
	*GpioTcr |= 0x00300000;
	*GpioOr  |= 0x00300000;

	return 0;
}

int misc_init_r(void)
{
	lcd_init();

	return 0;
}

int checkboard (void)
{
	char *s = getenv ("serial#");

	printf ("Board: Taishan - AMCC PPC440GX Evaluation Board");
	if (s != NULL) {
		puts (", serial# ");
		puts (s);
	}
	putc ('\n');

#ifdef CONFIG_SYS_INIT_SHOW_RESET_REG
	show_reset_reg();
#endif

	return (0);
}

/*************************************************************************
 *  pci_pre_init
 *
 *  This routine is called just prior to registering the hose and gives
 *  the board the opportunity to check things. Returning a value of zero
 *  indicates that things are bad & PCI initialization should be aborted.
 *
 *	Different boards may wish to customize the pci controller structure
 *	(add regions, override default access routines, etc) or perform
 *	certain pre-initialization actions.
 *
 ************************************************************************/
#if defined(CONFIG_PCI)
int pci_pre_init(struct pci_controller * hose )
{
	unsigned long strap;

	/*--------------------------------------------------------------------------+
	 *	The ocotea board is always configured as the host & requires the
	 *	PCI arbiter to be enabled.
	 *--------------------------------------------------------------------------*/
	mfsdr(SDR0_SDSTP1, strap);
	if( (strap & SDR0_SDSTP1_PAE_MASK) == 0 ){
		printf("PCI: SDR0_STRP1[%08lX] - PCI Arbiter disabled.\n",strap);
		return 0;
	}

	return 1;
}
#endif /* defined(CONFIG_PCI) */

/*************************************************************************
 *  pci_target_init
 *
 *	The bootstrap configuration provides default settings for the pci
 *	inbound map (PIM). But the bootstrap config choices are limited and
 *	may not be sufficient for a given board.
 *
 ************************************************************************/
#if defined(CONFIG_PCI) && defined(CONFIG_SYS_PCI_TARGET_INIT)
void pci_target_init(struct pci_controller * hose )
{
	/*--------------------------------------------------------------------------+
	 * Disable everything
	 *--------------------------------------------------------------------------*/
	out32r( PCIL0_PIM0SA, 0 ); /* disable */
	out32r( PCIL0_PIM1SA, 0 ); /* disable */
	out32r( PCIL0_PIM2SA, 0 ); /* disable */
	out32r( PCIL0_EROMBA, 0 ); /* disable expansion rom */

	/*--------------------------------------------------------------------------+
	 * Map all of SDRAM to PCI address 0x0000_0000. Note that the 440 strapping
	 * options to not support sizes such as 128/256 MB.
	 *--------------------------------------------------------------------------*/
	out32r( PCIL0_PIM0LAL, CONFIG_SYS_SDRAM_BASE );
	out32r( PCIL0_PIM0LAH, 0 );
	out32r( PCIL0_PIM0SA, ~(gd->ram_size - 1) | 1 );

	out32r( PCIL0_BAR0, 0 );

	/*--------------------------------------------------------------------------+
	 * Program the board's subsystem id/vendor id
	 *--------------------------------------------------------------------------*/
	out16r( PCIL0_SBSYSVID, CONFIG_SYS_PCI_SUBSYS_VENDORID );
	out16r( PCIL0_SBSYSID, CONFIG_SYS_PCI_SUBSYS_DEVICEID );

	out16r( PCIL0_CMD, in16r(PCIL0_CMD) | PCI_COMMAND_MEMORY );
}
#endif /* defined(CONFIG_PCI) && defined(CONFIG_SYS_PCI_TARGET_INIT) */

/*************************************************************************
 *  is_pci_host
 *
 *	This routine is called to determine if a pci scan should be
 *	performed. With various hardware environments (especially cPCI and
 *	PPMC) it's insufficient to depend on the state of the arbiter enable
 *	bit in the strap register, or generic host/adapter assumptions.
 *
 *	Rather than hard-code a bad assumption in the general 440 code, the
 *	440 pci code requires the board to decide at runtime.
 *
 *	Return 0 for adapter mode, non-zero for host (monarch) mode.
 *
 *
 ************************************************************************/
#if defined(CONFIG_PCI)
int is_pci_host(struct pci_controller *hose)
{
	/* The ocotea board is always configured as host. */
	return(1);
}
#endif /* defined(CONFIG_PCI) */

#ifdef CONFIG_POST
/*
 * Returns 1 if keys pressed to start the power-on long-running tests
 * Called from board_init_f().
 */
int post_hotkeys_pressed(void)
{
	return (ctrlc());
}
#endif

int board_eth_init(bd_t *bis)
{
	cpu_eth_init(bis);
	return pci_eth_init(bis);
}
