#ifndef _TEST_COMMON_H
#define _TEST_COMMON_H

#include "lib/log.h"

#define PASS(name)        LOG_CORE("  [PASS] " name "\r\n")
#define FAIL(name)        LOG_CORE("  [FAIL] " name "\r\n")
#define CHECK(name, expr) do { if (expr) PASS(name); else FAIL(name); } while (0)

#endif
