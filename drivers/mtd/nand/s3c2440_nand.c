/*
 * (C) Copyright 2006 OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

#include <nand.h>
#include <s3c2410.h>
#include <asm/io.h>

#define S3C2410_NFCONF_EN          (1<<15)
#define S3C2410_NFCONF_512BYTE     (1<<14)
#define S3C2410_NFCONF_4STEP       (1<<13)
#define S3C2410_NFCONF_INITECC     (1<<12)

#ifdef CONFIG_24410
#define S3C2410_NFCONF_nFCE        (1<<11)
#define S3C2410_NFCONF_TACLS(x)    ((x)<<8)
#define S3C2410_NFCONF_TWRPH0(x)   ((x)<<4)
#define S3C2410_NFCONF_TWRPH1(x)   ((x)<<0)
#else
#define S3C2410_NFCONF_nFCE        (1<<1)
#define S3C2410_NFCONF_TACLS(x)    ((x)<<12)
#define S3C2410_NFCONF_TWRPH0(x)   ((x)<<8)
#define S3C2410_NFCONF_TWRPH1(x)   ((x)<<4)
#endif


#define S3C2410_ADDR_NALE 4
#define S3C2410_ADDR_NCLE 8
/* 根据 ctrl 值设置 地址或命令 */
static void s3c2440_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *chip = mtd->priv;
	struct s3c2410_nand *nand = s3c2410_get_base_nand();


	if (ctrl & NAND_CTRL_CHANGE) {

		if (ctrl & NAND_CLE)
        {
			chip->IO_ADDR_W = (void __iomem *)&nand->NFCMD;
        }
		else if (ctrl & NAND_ALE) {
            chip->IO_ADDR_W = (void __iomem *)&nand->NFADDR;
        }
        else {
            chip->IO_ADDR_W = (void __iomem *)&nand->NFDATA;
        }
			
		if (ctrl & NAND_NCE)
        {
			writel((readl(&nand->NFCONT) & ~S3C2410_NFCONF_nFCE),
			       &nand->NFCONT);
            debugX(1, "Chip Enable\n");
        }
		else
        {
			writel(readl(&nand->NFCONT) | S3C2410_NFCONF_nFCE,
			       &nand->NFCONT);
            debugX(1, "Chip Disable\n");
        }
	}
	
    /* 向相应寄存器写入值 */ 
	if (cmd != NAND_CMD_NONE)
    {
        debugX(1, "write cmd(0x%x) to reg(0x%p)\n", cmd, chip->IO_ADDR_W);
		writeb(cmd, chip->IO_ADDR_W);
    }

}

/* 判断NAND FLASH 设备是否准备好 */
static int s3c2440_dev_ready(struct mtd_info *mtd)
{
	struct s3c2410_nand *nand = s3c2410_get_base_nand();
	int is_ready = readl(&nand->NFSTAT) & 0x01;
    if(is_ready)
    {
        debugX(1, "dev is ready\n");
    }
    else 
    {
        debugX(1, "dev is busy\n");
    }

	return is_ready;
}

#ifdef CONFIG_S3C2410_NAND_HWECC
void s3c2410_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct s3c2410_nand *nand = s3c2410_get_base_nand();
	debugX(1, "s3c2410_nand_enable_hwecc(%p, %d)\n", mtd, mode);
	writel(readl(&nand->NFCONF) | S3C2410_NFCONF_INITECC, &nand->NFCONF);
}

static int s3c2410_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat,
				      u_char *ecc_code)
{
	ecc_code[0] = NFECC0;
	ecc_code[1] = NFECC1;
	ecc_code[2] = NFECC2;
	debugX(1, "s3c2410_nand_calculate_hwecc(%p,): 0x%02x 0x%02x 0x%02x\n",
	       mtd , ecc_code[0], ecc_code[1], ecc_code[2]);

	return 0;
}

static int s3c2410_nand_correct_data(struct mtd_info *mtd, u_char *dat,
				     u_char *read_ecc, u_char *calc_ecc)
{
	if (read_ecc[0] == calc_ecc[0] &&
	    read_ecc[1] == calc_ecc[1] &&
	    read_ecc[2] == calc_ecc[2])
		return 0;

	printf("s3c2410_nand_correct_data: not implemented\n");
	return -1;
}
#endif

/* NAND FLASH 控制器初始 */
int board_nand_init(struct nand_chip *nand)
{
	u_int32_t cfg;
	u_int8_t tacls, twrph0, twrph1;
	struct s3c24x0_clock_power *clk_power = s3c24x0_get_base_clock_power();
	struct s3c2410_nand *nand_reg = s3c2410_get_base_nand();

	debugX(1, "board_nand_init()\n");
    /* 使能 NAND flash 控制器时钟 */
	writel(readl(&clk_power->CLKCON) | (1 << 4), &clk_power->CLKCON);

	/* initialize hardware */
	/* 设置NF控制器的时序参数 */
	tacls = 0;
    twrph0 = 2;
	twrph1 = 1;

	cfg = readl(&nand_reg->NFCONF) & 0xf;
	cfg |= S3C2410_NFCONF_TACLS(tacls);
	cfg |= S3C2410_NFCONF_TWRPH0(twrph0);
	cfg |= S3C2410_NFCONF_TWRPH1(twrph1);
	writel(cfg, &nand_reg->NFCONF);
    
    cfg = (0<<13)|(0<<12)|(0<<10)|(0<<9)|(0<<8)|(0<<6)|(0<<5)|(1<<4)|(0<<1)|(1<<0);
    /* enable nand flash controller */
    writel(cfg, &nand_reg->NFCONT);

	/* initialize nand_chip data structure */
	nand->IO_ADDR_R = nand->IO_ADDR_W = (void __iomem *)&nand_reg->NFDATA;

	/* read_buf and write_buf are default */
	/* read_byte and write_byte are default */

	/* hwcontrol always must be implemented */
	nand->cmd_ctrl = s3c2440_hwcontrol;

	nand->dev_ready = s3c2440_dev_ready;

#ifdef CONFIG_S3C2410_NAND_HWECC
	nand->ecc.hwctl = s3c2410_nand_enable_hwecc;
	nand->ecc.calculate = s3c2410_nand_calculate_ecc;
	nand->ecc.correct = s3c2410_nand_correct_data;
	nand->ecc.mode = NAND_ECC_HW3_512;
#else
	nand->ecc.mode = NAND_ECC_SOFT;
#endif

#ifdef CONFIG_S3C2410_NAND_BBT
	nand->options = NAND_USE_FLASH_BBT;
#else
	nand->options = 0;
#endif

	debugX(1, "end of nand_init\n");
    debugX(1, "nfconf=0x%x\n", readl(&nand_reg->NFCONF));
	return 0;
}
