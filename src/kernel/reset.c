#include "kernel/reset.h"
#include "arch/utils.h"
#include "peripherals/pm.h"

void reboot(void) {
    put32(PM_WDOG, PM_PASSWORD | 1);
    put32(PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL);
    while (1) {}
}
