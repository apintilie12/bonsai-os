#ifndef _PM_H
#define _PM_H

#include "peripherals/base.h"

#define PM_BASE     (PBASE + 0x100000)
#define PM_RSTC     (PM_BASE + 0x1C)
#define PM_WDOG     (PM_BASE + 0x24)

#define PM_PASSWORD         0x5A000000
#define PM_RSTC_WRCFG_FULL  0x00000020

#endif /* _PM_H */
