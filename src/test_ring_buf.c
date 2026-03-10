#include "test_ring_buf.h"
#include "ring_buf.h"
#include "log.h"

#define PASS(name) LOG_CORE("  [PASS] " name "\r\n")
#define FAIL(name) LOG_CORE("  [FAIL] " name "\r\n")
#define CHECK(name, expr) do { if (expr) PASS(name); else FAIL(name); } while (0)

// --- char buffer (capacity 4, holds 3 elements) ---
static char char_storage[4];
static ring_buf_t char_buf = RING_BUF_INIT(char_storage, 4, sizeof(char));

// --- struct buffer ---
typedef struct {
    int id;
    char tag;
} test_item_t;

static test_item_t struct_storage[4];
static ring_buf_t struct_buf = RING_BUF_INIT(struct_storage, 4, sizeof(test_item_t));

static void test_char_buf(void) {
    LOG_CORE("-- char buffer --\r\n");
    char out;

    // Fresh state
    CHECK("empty on init",        ring_buf_is_empty(&char_buf));
    CHECK("not full on init",    !ring_buf_is_full(&char_buf));
    CHECK("count is 0 on init",   ring_buf_count(&char_buf) == 0);

    // Pop from empty
    CHECK("pop empty returns 0",  ring_buf_pop(&char_buf, &out) == 0);

    // Push one element
    char a = 'A';
    CHECK("push returns 1",       ring_buf_push(&char_buf, &a) == 1);
    CHECK("not empty after push", !ring_buf_is_empty(&char_buf));
    CHECK("count is 1",           ring_buf_count(&char_buf) == 1);

    // Pop recovers value
    CHECK("pop returns 1",        ring_buf_pop(&char_buf, &out) == 1);
    CHECK("pop gives correct val", out == 'A');
    CHECK("empty after pop",      ring_buf_is_empty(&char_buf));

    // Fill to capacity (4-slot buffer holds 3 elements)
    char b = 'B', c = 'C', d = 'D';
    ring_buf_push(&char_buf, &b);
    ring_buf_push(&char_buf, &c);
    ring_buf_push(&char_buf, &d);
    CHECK("full after 3 pushes",  ring_buf_is_full(&char_buf));
    CHECK("count is 3",           ring_buf_count(&char_buf) == 3);

    // Drop when full
    char e = 'E';
    CHECK("push when full drops", ring_buf_push(&char_buf, &e) == 0);
    CHECK("still full after drop", ring_buf_is_full(&char_buf));

    // Drain and verify order (FIFO)
    char got[3];
    ring_buf_pop(&char_buf, &got[0]);
    ring_buf_pop(&char_buf, &got[1]);
    ring_buf_pop(&char_buf, &got[2]);
    CHECK("FIFO order preserved",  got[0] == 'B' && got[1] == 'C' && got[2] == 'D');
    CHECK("empty after drain",     ring_buf_is_empty(&char_buf));

    // Wrap-around: push/pop across the end of the backing array
    char x = 'X', y = 'Y', z = 'Z';
    ring_buf_push(&char_buf, &x);
    ring_buf_push(&char_buf, &y);
    ring_buf_pop(&char_buf, &out);  // consume X, head/tail now offset
    ring_buf_push(&char_buf, &z);
    ring_buf_pop(&char_buf, &out);
    CHECK("wrap-around pop 1", out == 'Y');
    ring_buf_pop(&char_buf, &out);
    CHECK("wrap-around pop 2", out == 'Z');
    CHECK("empty after wrap",  ring_buf_is_empty(&char_buf));
}

static void test_struct_buf(void) {
    LOG_CORE("-- struct buffer --\r\n");
    test_item_t out;

    CHECK("empty on init", ring_buf_is_empty(&struct_buf));

    test_item_t item1 = { .id = 42,  .tag = 'A' };
    test_item_t item2 = { .id = 100, .tag = 'Z' };

    ring_buf_push(&struct_buf, &item1);
    ring_buf_push(&struct_buf, &item2);
    CHECK("count is 2", ring_buf_count(&struct_buf) == 2);

    ring_buf_pop(&struct_buf, &out);
    CHECK("struct pop id",  out.id  == 42);
    CHECK("struct pop tag", out.tag == 'A');

    ring_buf_pop(&struct_buf, &out);
    CHECK("struct pop id 2",  out.id  == 100);
    CHECK("struct pop tag 2", out.tag == 'Z');

    CHECK("empty after struct drain", ring_buf_is_empty(&struct_buf));
}

void test_ring_buf(void) {
    LOG_CORE("=== ring_buf tests ===\r\n");
    test_char_buf();
    test_struct_buf();
    LOG_CORE("=== tests done ===\r\n");
}
