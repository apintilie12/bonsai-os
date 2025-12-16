#ifndef _MMU_H
#define _MMU_H

#define MM_TYPE_PAGE_TABLE		0x3
#define MM_TYPE_PAGE 			0x3
#define MM_TYPE_BLOCK			0x1
#define MM_ACCESS			(0x1 << 10)
#define MM_ACCESS_PERMISSION		(0x01 << 6) 

/*
 * Memory region attributes:
 *
 *   n = AttrIndx[2:0]1
 *			n	MAIR
 *   DEVICE_nGnRnE	000	00000000
 *   NORMAL_NC		001	01000100
 */
#define MT_DEVICE_nGnRnE 			0x0
#define MT_NORMAL				0x1
#define MT_DEVICE_nGnRnE_FLAGS			0x00
#define MT_NORMAL_FLAGS				0xFF /* Write-Back, RW-Allocate */
#define MAIR_VALUE				(MT_DEVICE_nGnRnE_FLAGS << (8 * MT_DEVICE_nGnRnE)) | (MT_NORMAL_FLAGS << (8 * MT_NORMAL))

#define MMU_FLAGS	 		(MM_TYPE_BLOCK | (MT_NORMAL << 2) | (3 << 8) | MM_ACCESS)	
#define MMU_DEVICE_FLAGS		(MM_TYPE_BLOCK | (MT_DEVICE_nGnRnE << 2) | MM_ACCESS)	
#define MMU_PTE_FLAGS			(MM_TYPE_PAGE | (MT_NORMAL << 2) | (3 << 8) | MM_ACCESS | MM_ACCESS_PERMISSION)	

#define TCR_T0SZ			(64 - 48) 
#define TCR_T1SZ			((64 - 48) << 16)
#define TCR_TG0_4K			(0 << 14)
#define TCR_TG1_4K			(2 << 30)
#define TCR_SH1_INNER			(3 << 28)
#define TCR_ORGN1_WBWA			(1 << 26)
#define TCR_IRGN1_WBWA			(1 << 24)
#define TCR_SH0_INNER			(3 << 12)
#define TCR_ORGN0_WBWA			(1 << 10)
#define TCR_IRGN0_WBWA			(1 << 8)
#define TCR_VALUE			(TCR_T0SZ | TCR_T1SZ | TCR_TG0_4K | TCR_TG1_4K | TCR_SH1_INNER | TCR_ORGN1_WBWA | TCR_IRGN1_WBWA | TCR_SH0_INNER | TCR_ORGN0_WBWA | TCR_IRGN0_WBWA)

#endif
