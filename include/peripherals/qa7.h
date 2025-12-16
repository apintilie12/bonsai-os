#ifndef _P_QA7_H
#define _P_QA7_H

#include "mm.h"

#define QA7_PHYS_BASE       0x40000000
#define QA7_BASE            (VA_START + QA7_PHYS_BASE)

/* Core Timer Interrupt Control (for Generic Timers) */
#define CORE0_TIMER_IRQCNTL (QA7_BASE + 0x40)
#define CORE1_TIMER_IRQCNTL (QA7_BASE + 0x44)
#define CORE2_TIMER_IRQCNTL (QA7_BASE + 0x48)
#define CORE3_TIMER_IRQCNTL (QA7_BASE + 0x4C)

/* Core Mailboxes (for IPIs) */
/// Core 0
#define CORE0_MBOX_INT_CNTL     (QA7_BASE + 0x50)

#define CORE0_MBOX0_SET         (QA7_BASE + 0x80)
#define CORE0_MBOX1_SET         (QA7_BASE + 0x84)
#define CORE0_MBOX2_SET         (QA7_BASE + 0x88)
#define CORE0_MBOX3_SET         (QA7_BASE + 0x8C)

#define CORE0_MBOX0_RDCLR       (QA7_BASE + 0xC0)
#define CORE0_MBOX1_RDCLR       (QA7_BASE + 0xC4)
#define CORE0_MBOX2_RDCLR       (QA7_BASE + 0xC8)
#define CORE0_MBOX3_RDCLR       (QA7_BASE + 0xCC)

/// Core 1
#define CORE1_MBOX_INT_CNTL     (QA7_BASE + 0x54)

#define CORE1_MBOX0_SET         (QA7_BASE + 0x90)
#define CORE1_MBOX1_SET         (QA7_BASE + 0x94)
#define CORE1_MBOX2_SET         (QA7_BASE + 0x98)
#define CORE1_MBOX3_SET         (QA7_BASE + 0x9C)

#define CORE1_MBOX0_RDCLR       (QA7_BASE + 0xD0)
#define CORE1_MBOX1_RDCLR       (QA7_BASE + 0xD4)
#define CORE1_MBOX2_RDCLR       (QA7_BASE + 0xD8)
#define CORE1_MBOX3_RDCLR       (QA7_BASE + 0xDC)

/// Core 2
#define CORE2_MBOX_INT_CNTL     (QA7_BASE + 0x58)

#define CORE2_MBOX0_SET         (QA7_BASE + 0xA0)
#define CORE2_MBOX1_SET         (QA7_BASE + 0xA4)
#define CORE2_MBOX2_SET         (QA7_BASE + 0xA8)
#define CORE2_MBOX3_SET         (QA7_BASE + 0xAC)

#define CORE2_MBOX0_RDCLR       (QA7_BASE + 0xE0)
#define CORE2_MBOX1_RDCLR       (QA7_BASE + 0xE4)
#define CORE2_MBOX2_RDCLR       (QA7_BASE + 0xE8)
#define CORE2_MBOX3_RDCLR       (QA7_BASE + 0xEC)

/// Core 3
#define CORE3_MBOX_INT_CNTL     (QA7_BASE + 0x5C)

#define CORE3_MBOX0_SET         (QA7_BASE + 0xB0)
#define CORE3_MBOX1_SET         (QA7_BASE + 0xB4)
#define CORE3_MBOX2_SET         (QA7_BASE + 0xB8)
#define CORE3_MBOX3_SET         (QA7_BASE + 0xBC)

#define CORE3_MBOX0_RDCLR       (QA7_BASE + 0xF0)
#define CORE3_MBOX1_RDCLR       (QA7_BASE + 0xF4)
#define CORE3_MBOX2_RDCLR       (QA7_BASE + 0xF8)
#define CORE3_MBOX3_RDCLR       (QA7_BASE + 0xFC)

#endif /* _P_QA7_H */