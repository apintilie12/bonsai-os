#ifndef _EMMC_H
#define _EMMC_H

#include "base.h"

#define EMMC_BASE               (PBASE + 0x300000)

// Register offsets from EMMC_BASE
#define EMMC_ARG2               0x00    // Argument 2 (used by ACMD23)
#define EMMC_BLKSIZECNT         0x04    // Block size (bits 9:0) | block count (bits 31:16)
#define EMMC_ARG1               0x08    // Argument 1 (command argument)
#define EMMC_CMDTM              0x0C    // Command and transfer mode
#define EMMC_RESP0              0x10    // Response word 0
#define EMMC_RESP1              0x14    // Response word 1
#define EMMC_RESP2              0x18    // Response word 2
#define EMMC_RESP3              0x1C    // Response word 3
#define EMMC_DATA               0x20    // Data register (one 32-bit word at a time)
#define EMMC_STATUS             0x24    // Status (read-only)
#define EMMC_CONTROL0           0x28    // Host control 0
#define EMMC_CONTROL1           0x2C    // Host control 1 (clock, reset)
#define EMMC_INTERRUPT          0x30    // Interrupt flags (write 1 to clear)
#define EMMC_IRPT_MASK          0x34    // Which flags set the interrupt line
#define EMMC_IRPT_EN            0x38    // Which flags are enabled
#define EMMC_CONTROL2           0x3C    // Host control 2
#define EMMC_SLOTISR_VER        0xFC    // Slot interrupt status + host controller version

// Helper: read/write a register by offset
#define EMMC_READ(reg)          (*((volatile unsigned int *)(EMMC_BASE + (reg))))
#define EMMC_WRITE(reg, val)    (*((volatile unsigned int *)(EMMC_BASE + (reg))) = (val))

// CMDTM — command and transfer mode fields
#define CMD_INDEX_SHIFT         24      // Bits 29:24 — command index
#define CMD_RSPNS_NONE          (0 << 16)
#define CMD_RSPNS_136           (1 << 16)   // 136-bit response (R2)
#define CMD_RSPNS_48            (2 << 16)   // 48-bit response (R1, R3, R6, R7)
#define CMD_RSPNS_48B           (3 << 16)   // 48-bit response + busy check (R1b)
#define CMD_CRCCHK_EN           (1 << 19)   // Enable CRC check on response
#define CMD_IXCHK_EN            (1 << 20)   // Enable command index check
#define CMD_DATA_EN             (1 << 21)   // Command involves a data transfer
#define CMD_READ                (1 << 4)    // Transfer direction: card to host

// STATUS register bits
#define STATUS_CMD_INHIBIT      (1 << 0)    // Command line in use
#define STATUS_DAT_INHIBIT      (1 << 1)    // Data line in use
#define STATUS_READ_AVAIL       (1 << 11)   // At least one word available in read FIFO

// INTERRUPT register bits (same layout for IRPT_MASK and IRPT_EN)
#define INT_CMD_DONE            (1 << 0)    // Command complete
#define INT_DATA_DONE           (1 << 1)    // Data transfer complete
#define INT_READ_RDY            (1 << 5)    // Read FIFO has data
#define INT_WRITE_RDY           (1 << 4)    // Write FIFO ready to accept data
#define INT_ERROR               (1 << 15)   // Error summary bit
#define INT_ERR_MASK            0xFFFF0000  // All error bits in upper half

// CONTROL0 bits
#define C0_4BIT_BUS             (1 << 1)    // Use 4-bit data bus

// CONTROL1 bits
#define C1_CLK_INTLEN           (1 << 0)    // Enable internal clock
#define C1_CLK_STABLE           (1 << 1)    // Internal clock stable (read-only)
#define C1_CLK_EN               (1 << 2)    // Enable SD clock to card
#define C1_CLK_FREQ_SHIFT       8           // Clock divisor bits 15:8
#define C1_CLK_FREQ8_SHIFT      6           // Clock divisor extended bits 7:6
#define C1_TOUNIT_MAX           (0xE << 16) // Data timeout = TMCLK * 2^27 (maximum)
#define C1_SRST_HC              (1 << 24)   // Software reset — entire host controller
#define C1_SRST_CMD             (1 << 25)   // Software reset — command circuit only
#define C1_SRST_DAT             (1 << 26)   // Software reset — data circuit only

// SD command numbers (argument to CMD() macro)
#define CMD(index)              ((unsigned int)(index) << CMD_INDEX_SHIFT)

#define SD_CMD0                 (CMD(0)  | CMD_RSPNS_NONE)                          // GO_IDLE_STATE
#define SD_CMD2                 (CMD(2)  | CMD_RSPNS_136)                           // ALL_SEND_CID
#define SD_CMD3                 (CMD(3)  | CMD_RSPNS_48  | CMD_CRCCHK_EN | CMD_IXCHK_EN) // SEND_RELATIVE_ADDR
#define SD_CMD7                 (CMD(7)  | CMD_RSPNS_48B | CMD_CRCCHK_EN | CMD_IXCHK_EN) // SELECT_CARD
#define SD_CMD8                 (CMD(8)  | CMD_RSPNS_48  | CMD_CRCCHK_EN | CMD_IXCHK_EN) // SEND_IF_COND
#define SD_CMD17                (CMD(17) | CMD_RSPNS_48  | CMD_CRCCHK_EN | CMD_IXCHK_EN | CMD_DATA_EN | CMD_READ) // READ_SINGLE_BLOCK
#define SD_CMD55                (CMD(55) | CMD_RSPNS_48  | CMD_CRCCHK_EN | CMD_IXCHK_EN) // APP_CMD prefix
#define SD_ACMD6                (CMD(6)  | CMD_RSPNS_48  | CMD_CRCCHK_EN | CMD_IXCHK_EN) // SET_BUS_WIDTH
#define SD_ACMD41               (CMD(41) | CMD_RSPNS_48)                            // SD_SEND_OP_COND (no CRC/index check)

// CMD8 argument: VHS=1 (2.7–3.6V), check pattern=0xAA

#define CMD8_VHS_ARG            0x000001AA

// ACMD41 argument: HCS=1 (SDHC support), voltage window 3.2–3.4V
#define ACMD41_ARG              0x51FF8000
#define ACMD41_BUSY             (1u << 31)  // 0 = card initialising, 1 = ready
#define ACMD41_CCS              (1u << 30)  // 1 = SDHC/SDXC (block-addressed)

// SD card type
typedef enum {
    SD_TYPE_UNKNOWN = 0,
    SD_TYPE_SD1,    // Standard capacity v1.x  (byte-addressed)
    SD_TYPE_SD2,    // Standard capacity v2.0  (byte-addressed)
    SD_TYPE_SDHC,   // High/Extended capacity  (block-addressed)
} sd_card_type_t;

#ifndef __ASSEMBLER__
typedef struct {
    sd_card_type_t  type;
    unsigned int    rca;         // Relative Card Address (from CMD3 response bits 31:16)
    int             initialized;
} sd_state_t;

int sd_init(void);
int sd_read_block(unsigned int lba, void *buf);
#endif // __ASSEMBLER__

#endif //_EMMC_H