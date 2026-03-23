#include "tests/test_bitmap.h"
#include "tests/test_common.h"
#include "lib/bitmap.h"

// 128-bit bitmap (2 words) for most tests
#define TEST_BITS 128
static unsigned long storage[BITMAP_WORDS(TEST_BITS)];
static bitmap_t bm;

static void reset(void) {
    bitmap_init(&bm, storage, TEST_BITS);
}

// ----------------------------------------------------------------------------
// bitmap_init
// ----------------------------------------------------------------------------
static void test_init(void) {
    LOG_CORE("-- bitmap_init --\r\n");
    reset();
    CHECK("num_bits set",    bm.num_bits == TEST_BITS);
    CHECK("words pointer",   bm.words == storage);
    CHECK("bit 0 clear",     bitmap_test(&bm, 0) == 0);
    CHECK("bit 63 clear",    bitmap_test(&bm, 63) == 0);
    CHECK("bit 64 clear",    bitmap_test(&bm, 64) == 0);
    CHECK("bit 127 clear",   bitmap_test(&bm, 127) == 0);
}

// ----------------------------------------------------------------------------
// bitmap_set / bitmap_test
// ----------------------------------------------------------------------------
static void test_set_and_test(void) {
    LOG_CORE("-- bitmap_set / bitmap_test --\r\n");
    reset();

    bitmap_set(&bm, 0);
    CHECK("set bit 0",       bitmap_test(&bm, 0) == 1);
    CHECK("bit 1 unaffected",bitmap_test(&bm, 1) == 0);

    bitmap_set(&bm, 63);
    CHECK("set bit 63 (last of word 0)",  bitmap_test(&bm, 63) == 1);
    CHECK("bit 62 unaffected",            bitmap_test(&bm, 62) == 0);

    bitmap_set(&bm, 64);
    CHECK("set bit 64 (first of word 1)", bitmap_test(&bm, 64) == 1);
    CHECK("bit 65 unaffected",            bitmap_test(&bm, 65) == 0);

    bitmap_set(&bm, 127);
    CHECK("set bit 127 (last bit)",       bitmap_test(&bm, 127) == 1);
    CHECK("bit 126 unaffected",           bitmap_test(&bm, 126) == 0);

    // other bits from earlier are still set
    CHECK("bit 0 still set",  bitmap_test(&bm, 0)  == 1);
    CHECK("bit 63 still set", bitmap_test(&bm, 63) == 1);
    CHECK("bit 64 still set", bitmap_test(&bm, 64) == 1);
}

// ----------------------------------------------------------------------------
// bitmap_clear
// ----------------------------------------------------------------------------
static void test_clear(void) {
    LOG_CORE("-- bitmap_clear --\r\n");
    reset();

    bitmap_set(&bm, 5);
    bitmap_set(&bm, 6);
    bitmap_set(&bm, 7);

    bitmap_clear(&bm, 6);
    CHECK("cleared bit 6",    bitmap_test(&bm, 6) == 0);
    CHECK("bit 5 unaffected", bitmap_test(&bm, 5) == 1);
    CHECK("bit 7 unaffected", bitmap_test(&bm, 7) == 1);

    // clearing an already-clear bit is a no-op
    bitmap_clear(&bm, 10);
    CHECK("clear already-clear bit", bitmap_test(&bm, 10) == 0);

    // clear across word boundary
    bitmap_set(&bm, 63);
    bitmap_set(&bm, 64);
    bitmap_clear(&bm, 63);
    CHECK("clear bit 63",            bitmap_test(&bm, 63) == 0);
    CHECK("bit 64 unaffected",       bitmap_test(&bm, 64) == 1);
}

// ----------------------------------------------------------------------------
// bitmap_set_range / bitmap_clear_range
// ----------------------------------------------------------------------------
static void test_set_range(void) {
    LOG_CORE("-- bitmap_set_range --\r\n");
    reset();

    bitmap_set_range(&bm, 4, 6);   // bits 4..9

    CHECK("bit 3 not set",  bitmap_test(&bm, 3) == 0);
    CHECK("bit 4 set",      bitmap_test(&bm, 4) == 1);
    CHECK("bit 7 set",      bitmap_test(&bm, 7) == 1);
    CHECK("bit 9 set",      bitmap_test(&bm, 9) == 1);
    CHECK("bit 10 not set", bitmap_test(&bm, 10) == 0);

    // range spanning word boundary
    reset();
    bitmap_set_range(&bm, 62, 4);  // bits 62..65

    CHECK("bit 61 not set", bitmap_test(&bm, 61) == 0);
    CHECK("bit 62 set",     bitmap_test(&bm, 62) == 1);
    CHECK("bit 63 set",     bitmap_test(&bm, 63) == 1);
    CHECK("bit 64 set",     bitmap_test(&bm, 64) == 1);
    CHECK("bit 65 set",     bitmap_test(&bm, 65) == 1);
    CHECK("bit 66 not set", bitmap_test(&bm, 66) == 0);
}

static void test_clear_range(void) {
    LOG_CORE("-- bitmap_clear_range --\r\n");
    reset();

    // set everything, then clear a window
    bitmap_set_range(&bm, 0, TEST_BITS);
    bitmap_clear_range(&bm, 10, 5);  // bits 10..14

    CHECK("bit 9 still set",  bitmap_test(&bm, 9)  == 1);
    CHECK("bit 10 cleared",   bitmap_test(&bm, 10) == 0);
    CHECK("bit 12 cleared",   bitmap_test(&bm, 12) == 0);
    CHECK("bit 14 cleared",   bitmap_test(&bm, 14) == 0);
    CHECK("bit 15 still set", bitmap_test(&bm, 15) == 1);
}

// ----------------------------------------------------------------------------
// bitmap_find_clear_run
// ----------------------------------------------------------------------------
static void test_find_clear_run_all_free(void) {
    LOG_CORE("-- bitmap_find_clear_run all free --\r\n");
    reset();

    CHECK("find run of 1",        bitmap_find_clear_run(&bm, 1)        == 0);
    CHECK("find run of 64",       bitmap_find_clear_run(&bm, 64)       == 0);
    CHECK("find run of 128",      bitmap_find_clear_run(&bm, 128)      == 0);
    CHECK("find run of TEST_BITS",bitmap_find_clear_run(&bm, TEST_BITS) == 0);
}

static void test_find_clear_run_all_used(void) {
    LOG_CORE("-- bitmap_find_clear_run all used --\r\n");
    reset();
    bitmap_set_range(&bm, 0, TEST_BITS);

    CHECK("no run of 1",   bitmap_find_clear_run(&bm, 1)   == -1);
    CHECK("no run of 128", bitmap_find_clear_run(&bm, 128) == -1);
}

static void test_find_clear_run_fragmented(void) {
    LOG_CORE("-- bitmap_find_clear_run fragmented --\r\n");
    reset();

    // set bits 0..9 used, bits 10..14 free (run of 5), bits 15..127 used
    bitmap_set_range(&bm, 0, 10);
    bitmap_set_range(&bm, 15, TEST_BITS - 15);

    CHECK("find run of 3 starts at 10", bitmap_find_clear_run(&bm, 3) == 10);
    CHECK("find run of 5 starts at 10", bitmap_find_clear_run(&bm, 5) == 10);
    CHECK("run of 6 not found",         bitmap_find_clear_run(&bm, 6) == -1);
}

static void test_find_clear_run_second_gap(void) {
    LOG_CORE("-- bitmap_find_clear_run picks first fitting gap --\r\n");
    reset();

    // gap of 2 at bits 4..5, gap of 5 at bits 20..24
    bitmap_set_range(&bm, 0, 4);
    bitmap_set_range(&bm, 6, 14);
    bitmap_set_range(&bm, 25, TEST_BITS - 25);

    CHECK("run of 2 returns first gap (4)",  bitmap_find_clear_run(&bm, 2) == 4);
    CHECK("run of 5 skips small gap (20)",   bitmap_find_clear_run(&bm, 5) == 20);
    CHECK("run of 6 not found",              bitmap_find_clear_run(&bm, 6) == -1);
}

static void test_find_clear_run_at_end(void) {
    LOG_CORE("-- bitmap_find_clear_run run at end --\r\n");
    reset();

    // only last 4 bits free
    bitmap_set_range(&bm, 0, TEST_BITS - 4);

    CHECK("run of 4 at end",  bitmap_find_clear_run(&bm, 4) == TEST_BITS - 4);
    CHECK("run of 5 not found", bitmap_find_clear_run(&bm, 5) == -1);
}

static void test_find_clear_run_single_bit(void) {
    LOG_CORE("-- bitmap_find_clear_run single clear bit --\r\n");
    reset();

    bitmap_set_range(&bm, 0, TEST_BITS);
    bitmap_clear(&bm, 50);

    CHECK("finds isolated clear bit", bitmap_find_clear_run(&bm, 1) == 50);
    CHECK("run of 2 not found",       bitmap_find_clear_run(&bm, 2) == -1);
}

// ----------------------------------------------------------------------------
// Entry point
// ----------------------------------------------------------------------------
void test_bitmap(void) {
    LOG_CORE("=== bitmap tests ===\r\n");

    test_init();
    test_set_and_test();
    test_clear();
    test_set_range();
    test_clear_range();
    test_find_clear_run_all_free();
    test_find_clear_run_all_used();
    test_find_clear_run_fragmented();
    test_find_clear_run_second_gap();
    test_find_clear_run_at_end();
    test_find_clear_run_single_bit();

    LOG_CORE("=== bitmap tests done ===\r\n");
}
