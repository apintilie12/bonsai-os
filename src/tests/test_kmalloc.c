#include "tests/test_kmalloc.h"
#include "tests/test_common.h"
#include "mm/kmalloc.h"
#include "stddef.h"

// ----------------------------------------------------------------------------
// Edge cases
// ----------------------------------------------------------------------------
static void test_null_cases(void) {
    LOG_CORE("-- null cases --\r\n");
    CHECK("kmalloc(0) returns NULL", kmalloc(0) == NULL);
    // kfree(NULL) must not crash
    kfree(NULL);
    CHECK("kfree(NULL) is safe", 1);
}

// ----------------------------------------------------------------------------
// Basic allocation and writability
// ----------------------------------------------------------------------------
static void test_basic_alloc(void) {
    LOG_CORE("-- basic alloc --\r\n");

    unsigned char *p = (unsigned char *)kmalloc(1);
    CHECK("kmalloc(1) non-NULL", p != NULL);
    if (p) {
        p[0] = 0xAB;
        CHECK("kmalloc(1) writable", p[0] == 0xAB);
        kfree(p);
    }

    p = (unsigned char *)kmalloc(32);
    CHECK("kmalloc(32) non-NULL", p != NULL);
    if (p) {
        p[0] = 0x01; p[31] = 0xFF;
        CHECK("kmalloc(32) writable", p[0] == 0x01 && p[31] == 0xFF);
        kfree(p);
    }

    p = (unsigned char *)kmalloc(500);
    CHECK("kmalloc(500) non-NULL", p != NULL);
    if (p) {
        p[0] = 0x55; p[499] = 0xAA;
        CHECK("kmalloc(500) writable", p[0] == 0x55 && p[499] == 0xAA);
        kfree(p);
    }
}

// ----------------------------------------------------------------------------
// Alignment — every returned pointer must be KMALLOC_BLOCK_SIZE aligned
// ----------------------------------------------------------------------------
static void test_alignment(void) {
    LOG_CORE("-- alignment --\r\n");
    unsigned long mask = KMALLOC_BLOCK_SIZE - 1;

    void *p1 = kmalloc(1);
    void *p2 = kmalloc(16);
    void *p3 = kmalloc(31);
    void *p4 = kmalloc(32);
    void *p5 = kmalloc(100);

    CHECK("kmalloc(1) aligned",   p1 && ((unsigned long)p1 & mask) == 0);
    CHECK("kmalloc(16) aligned",  p2 && ((unsigned long)p2 & mask) == 0);
    CHECK("kmalloc(31) aligned",  p3 && ((unsigned long)p3 & mask) == 0);
    CHECK("kmalloc(32) aligned",  p4 && ((unsigned long)p4 & mask) == 0);
    CHECK("kmalloc(100) aligned", p5 && ((unsigned long)p5 & mask) == 0);

    kfree(p1); kfree(p2); kfree(p3); kfree(p4); kfree(p5);
}

// ----------------------------------------------------------------------------
// No overlap — allocate N buffers, fill each with a distinct pattern,
// then verify none have been corrupted by their neighbours.
// ----------------------------------------------------------------------------
static void test_no_overlap(void) {
    LOG_CORE("-- no overlap --\r\n");
#define N_ALLOCS 8
    unsigned char *ptrs[N_ALLOCS];
    int sizes[N_ALLOCS] = { 1, 16, 30, 32, 64, 100, 200, 28 };

    for (int i = 0; i < N_ALLOCS; i++) {
        ptrs[i] = (unsigned char *)kmalloc(sizes[i]);
        CHECK("alloc succeeds", ptrs[i] != NULL);
        if (ptrs[i]) {
            for (int j = 0; j < sizes[i]; j++)
                ptrs[i][j] = (unsigned char)(i + 1);
        }
    }

    int ok = 1;
    for (int i = 0; i < N_ALLOCS; i++) {
        if (!ptrs[i]) continue;
        for (int j = 0; j < sizes[i]; j++) {
            if (ptrs[i][j] != (unsigned char)(i + 1)) {
                ok = 0;
                break;
            }
        }
    }
    CHECK("patterns intact (no overlap)", ok);

    for (int i = 0; i < N_ALLOCS; i++) kfree(ptrs[i]);
#undef N_ALLOCS
}

// ----------------------------------------------------------------------------
// Free and reuse — first-fit means the same address is returned after free
// ----------------------------------------------------------------------------
static void test_free_reuse(void) {
    LOG_CORE("-- free and reuse --\r\n");

    void *a = kmalloc(64);
    CHECK("first alloc non-NULL", a != NULL);
    kfree(a);

    void *b = kmalloc(64);
    CHECK("second alloc non-NULL", b != NULL);
    CHECK("same address reused",   a == b);
    kfree(b);
}

// ----------------------------------------------------------------------------
// Large allocation (> PAGE_SIZE) bypasses the pool
// ----------------------------------------------------------------------------
static void test_large_alloc(void) {
    LOG_CORE("-- large alloc --\r\n");

    unsigned char *p = (unsigned char *)kmalloc(PAGE_SIZE + 1);
    CHECK("large alloc non-NULL", p != NULL);
    if (p) {
        p[0]          = 0xDE;
        p[PAGE_SIZE]  = 0xAD;
        CHECK("large alloc writable", p[0] == 0xDE && p[PAGE_SIZE] == 0xAD);
        kfree(p);
    }
}

// ----------------------------------------------------------------------------
// Entry point
// ----------------------------------------------------------------------------
void test_kmalloc(void) {
    LOG_CORE("=== kmalloc tests ===\r\n");
    test_null_cases();
    test_basic_alloc();
    test_alignment();
    test_no_overlap();
    test_free_reuse();
    test_large_alloc();
    LOG_CORE("=== kmalloc tests done ===\r\n");
}
