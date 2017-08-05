#include <common.h>
#include <linux/mtd/nand.h>
#include <s3c2410.h>
#include <asm/io.h> 

#define NF_CMD(nand, cmd)   writeb(cmd, &nand->NFCMD)
#define NF_ADDR(nand, addr) writeb(addr, &nand->NFADDR)
#define NF_RDDATA8(nand)    readb(&nand->NFDATA)
#define NF_ENABLE_CE(nand)  writel(readl(&nand->NFCONT) & ~(1<<1), &nand->NFCONT)
#define NF_DISABLE_CE(nand) writel(readl(&nand->NFCONT) | (1<<1), &nand->NFCONT)
#define NF_CLEAR_RB(nand)   writel(readl(&nand->NFSTAT) | (1<<2), &nand->NFSTAT)
#define NF_DETECT_RB(nand)  do { \
                                while(!(readl(&nand->NFSTAT)&(1<<2))); \
                            } while(0) 

static void nandll_reset(void)
{
    struct s3c2410_nand *nand = s3c2410_get_base_nand();

    NF_ENABLE_CE(nand);
    NF_CLEAR_RB(nand);
    NF_CMD(nand, NAND_CMD_RESET);
    NF_DETECT_RB(nand);
    NF_DISABLE_CE(nand);
}

static int nandll_read_page(uchar *buf, ulong addr, int large_block)
{
    int i;
    int page_size = 512;
    struct s3c2410_nand *nand = s3c2410_get_base_nand();

    if(large_block)
    {
        page_size = 2048;
    }

    NF_ENABLE_CE(nand);
    NF_CLEAR_RB(nand);
    
    NF_CMD(nand, NAND_CMD_READ0);

    NF_ADDR(nand, 0x0);
    if(large_block)
        NF_ADDR(nand, 0x0);
    
    NF_ADDR(nand, addr&0xff);
    NF_ADDR(nand, (addr>>8)&0xff);
    NF_ADDR(nand, (addr>>16)&0xff);

    if(large_block)
    {
        NF_CMD(nand, NAND_CMD_READSTART);
    }

    NF_DETECT_RB(nand);

    for(i=0; i<page_size; i++)
    {
        buf[i] = NF_RDDATA8(nand);
    }

    NF_DISABLE_CE(nand);

    return 0;
}

static int nandll_read_blocks(ulong dst_addr, ulong size, int large_block)
{
    int i;
    uchar *buf = (uchar *)dst_addr;
    uint page_shift = 9;

    if(large_block)
        page_shift = 11;

    for(i=0; i<(size>>page_shift); i++, buf+= (1<<page_shift))
    {
        nandll_read_page(buf, i, large_block);
    }

    return 0;
}

int copy_to_ram_from_nand(void)
{
    int large_block = 0;
    int i;
    vu_char id;
    struct s3c2410_nand *nand = s3c2410_get_base_nand();

    NF_ENABLE_CE(nand);
    NF_CMD(nand, NAND_CMD_READID);
    NF_ADDR(nand, 0x0);

    for(i=0; i<200; i++);
    id = NF_RDDATA8(nand);
    id = NF_RDDATA8(nand);

    nandll_reset();

    if(id > 0x80)
        large_block = 1;

    return nandll_read_blocks(TEXT_BASE, CONFIG_UBOOT_SIZE, large_block);
}



