#include "peripherals/emmc.h"
#include "drivers/block_dev.h"
#include "arch/utils.h"
#include "lib/log.h"


#define CMD_POLL_TIMEOUT    1000    // iterations waiting for command completion (~0.5 ms)
#define DATA_POLL_TIMEOUT   100000  // iterations waiting for data FIFO (~50 ms)
#define ACMD41_RETRIES      1000    // max loops waiting for card to power up


#define CLK_DIV_ID      104     // ~400 kHz - identification mode
#define CLK_DIV_DATA    2       // ~20  MHz - data transfer mode

static sd_state_t sd_state;

// Wait until (reg & mask) == 0. Returns 0 on success, -1 on timeout.
static int poll_clear(unsigned int reg, unsigned int mask, int timeout)
{
	for (int i = 0; i < timeout; i++) {
		if (!(EMMC_READ(reg) & mask))
			return 0;
		delay(100);
	}
	return -1;
}

// Wait until (reg & mask) != 0. Returns 0 on success, -1 on timeout.
static int poll_set(unsigned int reg, unsigned int mask, int timeout)
{
	for (int i = 0; i < timeout; i++) {
		if (EMMC_READ(reg) & mask)
			return 0;
		delay(100);
	}
	return -1;
}

// Set SD clock to base_clock / divisor.
// divisor is a 10-bit value: upper 8 bits go to CLKFREQ (bits 15:8),
// lower 2 bits go to CLKFREQMS (bits 7:6).
static void set_clock(unsigned int divisor)
{
	// Disable SD clock output
	EMMC_WRITE(EMMC_CONTROL1, EMMC_READ(EMMC_CONTROL1) & ~C1_CLK_EN);
	delay(100);

	unsigned int c1 = EMMC_READ(EMMC_CONTROL1);
	c1 &= ~0x0000FFC0u;   // clear bits 15:6 (both divisor fields)
	c1 |= ((divisor & 0xFFu)       << C1_CLK_FREQ_SHIFT);   // bits 15:8
	c1 |= (((divisor >> 8) & 0x3u) << C1_CLK_FREQ8_SHIFT);  // bits 7:6
	c1 |= C1_CLK_INTLEN;
	EMMC_WRITE(EMMC_CONTROL1, c1);

	if (poll_set(EMMC_CONTROL1, C1_CLK_STABLE, CMD_POLL_TIMEOUT) < 0) {
		LOG_DEBUG("SD: clock did not stabilise\r\n");
		return;
	}

	EMMC_WRITE(EMMC_CONTROL1, EMMC_READ(EMMC_CONTROL1) | C1_CLK_EN);
	delay(100);
}

// Send one SD command and collect the response.
// resp[] must hold 4 words. Only resp[0] is filled for 48-bit responses;
// all four words are filled for 136-bit (R2) responses.
// Returns 0 on success, -1 on error.
static int emmc_send_cmd(unsigned int cmd, unsigned int arg, unsigned int resp[4])
{
	if (poll_clear(EMMC_STATUS, STATUS_CMD_INHIBIT, CMD_POLL_TIMEOUT) < 0) {
		LOG_DEBUG("SD: CMD_INHIBIT timeout (cmd=0x%x)\r\n", cmd);
		return -1;
	}

	if ((cmd & CMD_DATA_EN) && poll_clear(EMMC_STATUS, STATUS_DAT_INHIBIT, CMD_POLL_TIMEOUT) < 0) {
		LOG_DEBUG("SD: DAT_INHIBIT timeout (cmd=0x%x)\r\n", cmd);
		return -1;
	}

	// Clear all pending interrupt flags before issuing the command
	EMMC_WRITE(EMMC_INTERRUPT, 0xFFFFFFFFu);

	EMMC_WRITE(EMMC_ARG1,  arg);
	EMMC_WRITE(EMMC_CMDTM, cmd);

	if (poll_set(EMMC_INTERRUPT, INT_CMD_DONE | INT_ERROR, CMD_POLL_TIMEOUT) < 0) {
		LOG_DEBUG("SD: command timeout (cmd=0x%x)\r\n", cmd);
		return -1;
	}

	unsigned int irpt = EMMC_READ(EMMC_INTERRUPT);
	EMMC_WRITE(EMMC_INTERRUPT, INT_CMD_DONE | INT_ERROR);

	if (irpt & INT_ERROR) {
		LOG_DEBUG("SD: command error (cmd=0x%x irpt=0x%x)\r\n", cmd, irpt);
		return -1;
	}

	if (resp) {
		resp[0] = EMMC_READ(EMMC_RESP0);
		if ((cmd & (3u << 16)) == CMD_RSPNS_136) {
			resp[1] = EMMC_READ(EMMC_RESP1);
			resp[2] = EMMC_READ(EMMC_RESP2);
			resp[3] = EMMC_READ(EMMC_RESP3);
		}
	}

	return 0;
}

// Send an application-specific command (ACMD) preceded by CMD55.
// rca should be 0 before card selection, sd_state.rca afterwards.
static int emmc_send_acmd(unsigned int acmd, unsigned int arg, unsigned int rca,
                          unsigned int resp[4])
{
	unsigned int r[4];
	if (emmc_send_cmd(SD_CMD55, (unsigned int)rca << 16, r) < 0)
		return -1;
	return emmc_send_cmd(acmd, arg, resp);
}

int sd_init(void)
{
	unsigned int resp[4];

	sd_state.type        = SD_TYPE_UNKNOWN;
	sd_state.rca         = 0;
	sd_state.initialized = 0;

	// Reset host controller
	EMMC_WRITE(EMMC_CONTROL1, C1_SRST_HC);
	if (poll_clear(EMMC_CONTROL1, C1_SRST_HC, CMD_POLL_TIMEOUT) < 0) {
		LOG_DEBUG("SD: host controller reset timeout\r\n");
		return -1;
	}

	// Maximum data timeout, clocks still off
	EMMC_WRITE(EMMC_CONTROL1, C1_TOUNIT_MAX);

	// Keep all interrupt flags visible for polling; don't route to IRQ line
	EMMC_WRITE(EMMC_IRPT_EN,   0);
	EMMC_WRITE(EMMC_IRPT_MASK, 0xFFFFFFFFu);

	// Identification-mode clock (<=400 kHz required by SD spec)
	set_clock(CLK_DIV_ID);
	delay(10000);

	// CMD0: GO_IDLE_STATE - reset card to idle
	emmc_send_cmd(SD_CMD0, 0, 0);
	delay(1000);

	// CMD8: SEND_IF_COND - distinguish SD v1 from v2
	if (emmc_send_cmd(SD_CMD8, CMD8_VHS_ARG, resp) < 0) {
		// CMD8 not supported: SD v1 card (byte-addressed, no HCS)
		sd_state.type = SD_TYPE_SD1;
	} else {
		if ((resp[0] & 0xFFu) != 0xAAu) {
			LOG_DEBUG("SD: CMD8 check pattern mismatch (0x%x)\r\n", resp[0]);
			return -1;
		}
		sd_state.type = SD_TYPE_SD2; // may be promoted to SDHC after ACMD41
	}

	// ACMD41: SD_SEND_OP_COND - wait for card to finish power-up
	// HCS bit only meaningful for v2 cards
	unsigned int acmd41_arg = (sd_state.type == SD_TYPE_SD1) ? 0x00FF8000u : ACMD41_ARG;

	int ready = 0;
	for (int i = 0; i < ACMD41_RETRIES; i++) {
		if (emmc_send_acmd(SD_ACMD41, acmd41_arg, 0, resp) < 0) {
			LOG_DEBUG("SD: ACMD41 failed\r\n");
			return -1;
		}
		if (resp[0] & ACMD41_BUSY) {
			ready = 1;
			break;
		}
		delay(1000);
	}
	if (!ready) {
		LOG_DEBUG("SD: ACMD41 timed out (card never became ready)\r\n");
		return -1;
	}

	if (sd_state.type == SD_TYPE_SD2 && (resp[0] & ACMD41_CCS))
		sd_state.type = SD_TYPE_SDHC;

	// CMD2: ALL_SEND_CID - read card identification register
	if (emmc_send_cmd(SD_CMD2, 0, resp) < 0) {
		LOG_DEBUG("SD: CMD2 (ALL_SEND_CID) failed\r\n");
		return -1;
	}

	// CMD3: SEND_RELATIVE_ADDR - card publishes its RCA
	if (emmc_send_cmd(SD_CMD3, 0, resp) < 0) {
		LOG_DEBUG("SD: CMD3 (SEND_RELATIVE_ADDR) failed\r\n");
		return -1;
	}
	sd_state.rca = resp[0] >> 16;

	// CMD7: SELECT_CARD - move card to Transfer state
	if (emmc_send_cmd(SD_CMD7, sd_state.rca << 16, resp) < 0) {
		LOG_DEBUG("SD: CMD7 (SELECT_CARD) failed\r\n");
		return -1;
	}

	// Safe to increase clock now that the card is selected
	set_clock(CLK_DIV_DATA);

	// ACMD6: SET_BUS_WIDTH to 4-bit
	if (emmc_send_acmd(SD_ACMD6, 2, sd_state.rca, resp) < 0) {
		LOG_DEBUG("SD: ACMD6 (SET_BUS_WIDTH) failed\r\n");
		return -1;
	}
	EMMC_WRITE(EMMC_CONTROL0, EMMC_READ(EMMC_CONTROL0) | C0_4BIT_BUS);

	sd_state.initialized = 1;
	LOG_DEBUG("SD: init ok — type=%d rca=0x%x\r\n", sd_state.type, sd_state.rca);

	static block_dev_t sd_dev = { .read_block = sd_read_block };
	blkdev_register(&sd_dev);

	return 0;
}

int sd_read_block(unsigned int lba, void *buf)
{
	if (!sd_state.initialized)
		return -1;

	if (poll_clear(EMMC_STATUS, STATUS_DAT_INHIBIT, DATA_POLL_TIMEOUT) < 0) {
		LOG_DEBUG("SD: read DAT_INHIBIT timeout (lba=%u)\r\n", lba);
		return -1;
	}

	// SDHC uses block-number addressing; SD1/SD2 use byte addressing
	unsigned int arg = (sd_state.type == SD_TYPE_SDHC) ? lba : lba * 512;

	// 1 block of 512 bytes
	EMMC_WRITE(EMMC_BLKSIZECNT, (1u << 16) | 512u);

	unsigned int resp[4];
	if (emmc_send_cmd(SD_CMD17, arg, resp) < 0) {
		LOG_DEBUG("SD: CMD17 failed (lba=%u)\r\n", lba);
		return -1;
	}

	// Wait for read FIFO to be populated
	if (poll_set(EMMC_INTERRUPT, INT_READ_RDY | INT_ERROR, DATA_POLL_TIMEOUT) < 0) {
		LOG_DEBUG("SD: INT_READ_RDY timeout (lba=%u) INTERRUPT=0x%x STATUS=0x%x\r\n",
		          lba, EMMC_READ(EMMC_INTERRUPT), EMMC_READ(EMMC_STATUS));
		return -1;
	}
	if (EMMC_READ(EMMC_INTERRUPT) & INT_ERROR) {
		LOG_DEBUG("SD: read error interrupt (lba=%u)\r\n", lba);
		EMMC_WRITE(EMMC_INTERRUPT, INT_READ_RDY | INT_ERROR);
		return -1;
	}
	EMMC_WRITE(EMMC_INTERRUPT, INT_READ_RDY);

	// Drain 512 bytes (128 x 32-bit words) from the DATA FIFO
	unsigned int *p = (unsigned int *)buf;
	for (int i = 0; i < 512 / 4; i++)
		p[i] = EMMC_READ(EMMC_DATA);

	// Confirm transfer complete
	if (poll_set(EMMC_INTERRUPT, INT_DATA_DONE | INT_ERROR, DATA_POLL_TIMEOUT) < 0) {
		LOG_DEBUG("SD: INT_DATA_DONE timeout (lba=%u)\r\n", lba);
		return -1;
	}
	EMMC_WRITE(EMMC_INTERRUPT, INT_DATA_DONE | INT_ERROR);

	return 0;
}
