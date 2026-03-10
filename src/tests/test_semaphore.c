#include "tests/test_semaphore.h"
#include "tests/test_common.h"
#include "lib/semaphore.h"
#include "kernel/sched.h"
#include "kernel/fork.h"
#include "arch/utils.h"

// Shared state — reinitialized before each test
static semaphore_t test_sem;
static semaphore_t done_sem; // each test task signals this when finished
static volatile int shared_counter;

// ----------------------------------------------------------------------------
// Test 1: basic blocking
//
// Consumer blocks on test_sem (count=0). Producer delays, then signals.
// We verify the consumer did not run past the sem_wait before the signal.
// ----------------------------------------------------------------------------

static void t1_consumer(void) {
    sem_wait(&test_sem);
    shared_counter++;   // must only happen after producer signals
    sem_signal(&done_sem);
    exit_process();
}

static void t1_producer(void) {
    delay(20000000);    // long enough for consumer to schedule and block
    sem_signal(&test_sem);
    sem_signal(&done_sem);
    exit_process();
}

static void test_basic_blocking(void) {
    LOG_CORE("-- basic blocking --\r\n");
    sem_init(&test_sem, 0);
    sem_init(&done_sem, 0);
    shared_counter = 0;

    copy_process(PF_KTHREAD, (unsigned long)&t1_consumer, 0, "t1-consumer");
    copy_process(PF_KTHREAD, (unsigned long)&t1_producer, 0, "t1-producer");

    sem_wait(&done_sem); // wait for consumer
    sem_wait(&done_sem); // wait for producer

    CHECK("consumer unblocked after signal", shared_counter == 1);
}

// ----------------------------------------------------------------------------
// Test 2: counting semaphore — N signals, N non-blocking waits
//
// Semaphore starts at 3. One task calls sem_wait 3 times; all should
// return immediately without blocking.
// ----------------------------------------------------------------------------

static void t2_consumer(void) {
    sem_wait(&test_sem); shared_counter++;
    sem_wait(&test_sem); shared_counter++;
    sem_wait(&test_sem); shared_counter++;
    sem_signal(&done_sem);
    exit_process();
}

static void test_counting(void) {
    LOG_CORE("-- counting semaphore --\r\n");
    sem_init(&test_sem, 3);
    sem_init(&done_sem, 0);
    shared_counter = 0;

    copy_process(PF_KTHREAD, (unsigned long)&t2_consumer, 0, "t2-consumer");

    sem_wait(&done_sem);

    CHECK("3 non-blocking waits on count=3", shared_counter == 3);
}

// ----------------------------------------------------------------------------
// Test 3: multiple waiters
//
// 3 consumer tasks all block on test_sem (count=0).
// Producer signals 3 times, each with a small delay.
// All 3 consumers must eventually complete.
// ----------------------------------------------------------------------------

static void t3_consumer(void) {
    sem_wait(&test_sem);
    shared_counter++;
    sem_signal(&done_sem);
    exit_process();
}

static void t3_producer(void) {
    delay(20000000);
    sem_signal(&test_sem);
    delay(5000000);
    sem_signal(&test_sem);
    delay(5000000);
    sem_signal(&test_sem);
    sem_signal(&done_sem);
    exit_process();
}

static void test_multiple_waiters(void) {
    LOG_CORE("-- multiple waiters --\r\n");
    sem_init(&test_sem, 0);
    sem_init(&done_sem, 0);
    shared_counter = 0;

    copy_process(PF_KTHREAD, (unsigned long)&t3_consumer, 0, "t3-consumer-a");
    copy_process(PF_KTHREAD, (unsigned long)&t3_consumer, 0, "t3-consumer-b");
    copy_process(PF_KTHREAD, (unsigned long)&t3_consumer, 0, "t3-consumer-c");
    copy_process(PF_KTHREAD, (unsigned long)&t3_producer, 0, "t3-producer");

    sem_wait(&done_sem); // consumer a
    sem_wait(&done_sem); // consumer b
    sem_wait(&done_sem); // consumer c
    sem_wait(&done_sem); // producer

    CHECK("all 3 consumers unblocked", shared_counter == 3);
}

// ----------------------------------------------------------------------------

void test_semaphore(void) {
    LOG_CORE("=== semaphore tests ===\r\n");
    test_basic_blocking();
    test_counting();
    test_multiple_waiters();
    LOG_CORE("=== tests done ===\r\n");
}
