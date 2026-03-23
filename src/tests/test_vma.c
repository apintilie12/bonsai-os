#include "tests/test_vma.h"
#include "tests/test_common.h"
#include "mm/mm.h"
#include "kernel/sched.h"
#include "lib/errno.h"

static MM_STRUCT fresh(void) {
    MM_STRUCT mm;
    // zero the whole struct manually since we have no memset accessible here
    unsigned char *p = (unsigned char *)&mm;
    for (unsigned long i = 0; i < sizeof(MM_STRUCT); i++) p[i] = 0;
    return mm;
}

// ----------------------------------------------------------------------------
// vma_add
// ----------------------------------------------------------------------------
static void test_vma_add_basic(void) {
    LOG_CORE("-- vma_add basic --\r\n");
    MM_STRUCT mm = fresh();

    int r = vma_add(&mm, 0x1000, 0x3000, VMA_READ | VMA_WRITE | VMA_ANON);
    CHECK("returns E_OK",         r == E_OK);
    CHECK("count incremented",    mm.vma_count == 1);
    CHECK("virt_start stored",    mm.vmas[0].virt_start == 0x1000);
    CHECK("virt_end stored",      mm.vmas[0].virt_end   == 0x3000);
    CHECK("flags stored",         mm.vmas[0].flags == (VMA_READ | VMA_WRITE | VMA_ANON));
}

static void test_vma_add_multiple(void) {
    LOG_CORE("-- vma_add multiple non-overlapping --\r\n");
    MM_STRUCT mm = fresh();

    CHECK("add first",   vma_add(&mm, 0x1000, 0x2000, VMA_READ) == E_OK);
    CHECK("add second",  vma_add(&mm, 0x3000, 0x4000, VMA_READ) == E_OK);
    CHECK("add third",   vma_add(&mm, 0x5000, 0x6000, VMA_READ) == E_OK);
    CHECK("count == 3",  mm.vma_count == 3);
}

static void test_vma_add_fill_capacity(void) {
    LOG_CORE("-- vma_add fill to MAX_VMA_COUNT --\r\n");
    MM_STRUCT mm = fresh();

    for (int i = 0; i < MAX_VMA_COUNT; i++) {
        unsigned long start = (unsigned long)(i + 1) * 0x2000;
        unsigned long end   = start + 0x1000;
        CHECK("add succeeds", vma_add(&mm, start, end, VMA_READ) == E_OK);
    }
    CHECK("count == MAX", mm.vma_count == MAX_VMA_COUNT);

    int r = vma_add(&mm, 0xFF000, 0x100000, VMA_READ);
    CHECK("overflow returns E_NOMEM", r == E_NOMEM);
    CHECK("count unchanged",          mm.vma_count == MAX_VMA_COUNT);
}

static void test_vma_add_overlap(void) {
    LOG_CORE("-- vma_add overlap detection --\r\n");
    MM_STRUCT mm = fresh();

    vma_add(&mm, 0x3000, 0x6000, VMA_READ);

    CHECK("exact same range",
          vma_add(&mm, 0x3000, 0x6000, VMA_READ) == E_INVAL);

    CHECK("overlap at front (new starts inside existing)",
          vma_add(&mm, 0x1000, 0x4000, VMA_READ) == E_INVAL);

    CHECK("overlap at back (new ends inside existing)",
          vma_add(&mm, 0x5000, 0x8000, VMA_READ) == E_INVAL);

    CHECK("new fully contains existing",
          vma_add(&mm, 0x2000, 0x7000, VMA_READ) == E_INVAL);

    CHECK("new fully inside existing",
          vma_add(&mm, 0x4000, 0x5000, VMA_READ) == E_INVAL);

    CHECK("count unchanged after all failed adds", mm.vma_count == 1);
}

static void test_vma_add_adjacent(void) {
    LOG_CORE("-- vma_add adjacent (touching, not overlapping) --\r\n");
    MM_STRUCT mm = fresh();

    vma_add(&mm, 0x2000, 0x4000, VMA_READ);

    CHECK("region ending where existing starts",
          vma_add(&mm, 0x1000, 0x2000, VMA_READ) == E_OK);

    CHECK("region starting where existing ends",
          vma_add(&mm, 0x4000, 0x5000, VMA_READ) == E_OK);

    CHECK("count == 3", mm.vma_count == 3);
}

// ----------------------------------------------------------------------------
// vma_find
// ----------------------------------------------------------------------------
static void test_vma_find_empty(void) {
    LOG_CORE("-- vma_find empty MM --\r\n");
    MM_STRUCT mm = fresh();
    CHECK("returns NULL", vma_find(&mm, 0x1000) == 0);
}

static void test_vma_find_boundaries(void) {
    LOG_CORE("-- vma_find boundary addresses --\r\n");
    MM_STRUCT mm = fresh();
    vma_add(&mm, 0x2000, 0x5000, VMA_READ | VMA_WRITE);

    CHECK("addr == virt_start (inclusive)",
          vma_find(&mm, 0x2000) != 0);

    CHECK("addr in middle",
          vma_find(&mm, 0x3000) != 0);

    CHECK("addr == virt_end - PAGE_SIZE (last page)",
          vma_find(&mm, 0x4000) != 0);

    CHECK("addr == virt_end (exclusive)",
          vma_find(&mm, 0x5000) == 0);

    CHECK("addr below virt_start",
          vma_find(&mm, 0x1000) == 0);

    CHECK("addr above virt_end",
          vma_find(&mm, 0x6000) == 0);
}

static void test_vma_find_correct_entry(void) {
    LOG_CORE("-- vma_find returns correct entry --\r\n");
    MM_STRUCT mm = fresh();
    vma_add(&mm, 0x1000, 0x2000, VMA_READ);
    vma_add(&mm, 0x4000, 0x6000, VMA_READ | VMA_WRITE);
    vma_add(&mm, 0x8000, 0x9000, VMA_EXEC);

    VM_AREA *v = vma_find(&mm, 0x5000);
    CHECK("finds second VMA",     v != 0);
    CHECK("correct virt_start",   v && v->virt_start == 0x4000);
    CHECK("correct virt_end",     v && v->virt_end   == 0x6000);
    CHECK("correct flags",        v && v->flags == (VMA_READ | VMA_WRITE));
}

static void test_vma_find_gap(void) {
    LOG_CORE("-- vma_find address in gap between VMAs --\r\n");
    MM_STRUCT mm = fresh();
    vma_add(&mm, 0x1000, 0x2000, VMA_READ);
    vma_add(&mm, 0x4000, 0x5000, VMA_READ);

    CHECK("gap address returns NULL", vma_find(&mm, 0x3000) == 0);
}

// ----------------------------------------------------------------------------
// vma_remove
// ----------------------------------------------------------------------------
static void test_vma_remove_only(void) {
    LOG_CORE("-- vma_remove only entry --\r\n");
    MM_STRUCT mm = fresh();
    vma_add(&mm, 0x1000, 0x2000, VMA_READ);

    vma_remove(&mm, 0x1000);
    CHECK("count == 0",       mm.vma_count == 0);
    CHECK("slot zeroed",      mm.vmas[0].virt_start == 0 &&
                              mm.vmas[0].virt_end   == 0 &&
                              mm.vmas[0].flags      == 0);
    CHECK("find returns NULL", vma_find(&mm, 0x1000) == 0);
}

static void test_vma_remove_first(void) {
    LOG_CORE("-- vma_remove first of several --\r\n");
    MM_STRUCT mm = fresh();
    vma_add(&mm, 0x1000, 0x2000, VMA_READ);
    vma_add(&mm, 0x3000, 0x4000, VMA_WRITE);
    vma_add(&mm, 0x5000, 0x6000, VMA_EXEC);

    vma_remove(&mm, 0x1000);
    CHECK("count == 2",            mm.vma_count == 2);
    CHECK("first slot is old [1]", mm.vmas[0].virt_start == 0x3000);
    CHECK("second slot is old [2]",mm.vmas[1].virt_start == 0x5000);
    CHECK("removed not findable",  vma_find(&mm, 0x1000) == 0);
}

static void test_vma_remove_last(void) {
    LOG_CORE("-- vma_remove last of several --\r\n");
    MM_STRUCT mm = fresh();
    vma_add(&mm, 0x1000, 0x2000, VMA_READ);
    vma_add(&mm, 0x3000, 0x4000, VMA_WRITE);
    vma_add(&mm, 0x5000, 0x6000, VMA_EXEC);

    vma_remove(&mm, 0x5000);
    CHECK("count == 2",           mm.vma_count == 2);
    CHECK("first two unchanged",  mm.vmas[0].virt_start == 0x1000 &&
                                  mm.vmas[1].virt_start == 0x3000);
    CHECK("last slot zeroed",     mm.vmas[2].virt_start == 0 &&
                                  mm.vmas[2].virt_end   == 0);
    CHECK("removed not findable", vma_find(&mm, 0x5000) == 0);
}

static void test_vma_remove_middle(void) {
    LOG_CORE("-- vma_remove middle entry --\r\n");
    MM_STRUCT mm = fresh();
    vma_add(&mm, 0x1000, 0x2000, VMA_READ);
    vma_add(&mm, 0x3000, 0x4000, VMA_WRITE);
    vma_add(&mm, 0x5000, 0x6000, VMA_EXEC);

    vma_remove(&mm, 0x3000);
    CHECK("count == 2",            mm.vma_count == 2);
    CHECK("first slot unchanged",  mm.vmas[0].virt_start == 0x1000);
    CHECK("second slot is old [2]",mm.vmas[1].virt_start == 0x5000);
    CHECK("removed not findable",  vma_find(&mm, 0x3000) == 0);
}

static void test_vma_remove_nonexistent(void) {
    LOG_CORE("-- vma_remove non-existent address --\r\n");
    MM_STRUCT mm = fresh();
    vma_add(&mm, 0x1000, 0x2000, VMA_READ);
    vma_add(&mm, 0x3000, 0x4000, VMA_WRITE);

    vma_remove(&mm, 0x9000);
    CHECK("count unchanged", mm.vma_count == 2);
    CHECK("first intact",    mm.vmas[0].virt_start == 0x1000);
    CHECK("second intact",   mm.vmas[1].virt_start == 0x3000);
}

static void test_vma_remove_empty(void) {
    LOG_CORE("-- vma_remove from empty MM --\r\n");
    MM_STRUCT mm = fresh();
    vma_remove(&mm, 0x1000);
    CHECK("count still 0", mm.vma_count == 0);
}

// ----------------------------------------------------------------------------
// Entry point
// ----------------------------------------------------------------------------
void test_vma(void) {
    LOG_CORE("=== vma tests ===\r\n");

    test_vma_add_basic();
    test_vma_add_multiple();
    test_vma_add_fill_capacity();
    test_vma_add_overlap();
    test_vma_add_adjacent();

    test_vma_find_empty();
    test_vma_find_boundaries();
    test_vma_find_correct_entry();
    test_vma_find_gap();

    test_vma_remove_only();
    test_vma_remove_first();
    test_vma_remove_last();
    test_vma_remove_middle();
    test_vma_remove_nonexistent();
    test_vma_remove_empty();

    LOG_CORE("=== vma tests done ===\r\n");
}
