#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>

#define CNT_THREADS 5

struct lock {
    atomic_ullong ticket;
    atomic_ullong queue_read;
    atomic_ullong queue_write;
} mylock;

size_t critical_section     = 0;
const size_t count_iterate  = (size_t)1 << 15;
const bool with_lock        = true;
const bool print_debug_log  = false;


static void initlock(struct lock * lock) {
    lock->ticket = lock->queue_read = lock->queue_write = 0;
}
static void read_lock(struct lock * lock) {
    const unsigned long long ticket = atomic_fetch_add(&lock->ticket, 1);

    while (atomic_load(&lock->queue_read) != ticket);
    atomic_fetch_add(&lock->queue_read, 1);
}
static void write_lock(struct lock * lock) {
    const unsigned long long ticket = atomic_fetch_add(&lock->ticket, 1);

    while (atomic_load(&lock->queue_write) != ticket);
}


static void read_unlock(struct lock * lock) {
    atomic_fetch_add(&lock->queue_write, 1);
}
static void write_unlock(struct lock * lock) {
    atomic_fetch_add(&lock->queue_write, 1);
    atomic_fetch_add(&lock->queue_read, 1);
}


static void * routine(void * arg) {
    (void) arg;

    for (size_t i = 0; i < 2*count_iterate; i++) {
        if (with_lock) {
            if (i % 2 == 0) {
                write_lock(&mylock);
                critical_section++;
                write_unlock(&mylock);
            } else {
                size_t cur1,cur2;
                read_lock(&mylock);

                cur1 = critical_section;
                for (int j = 0; j < 100; j++) {
                    cur2 = critical_section;
                    if (cur1 != cur2)
                        printf("Race condition detected\n");
                }

                read_unlock(&mylock);
            }
        } else {
            critical_section++;
        }
    }

    return NULL;
}

int main() {
    initlock(&mylock);
    pthread_t threads_pool[CNT_THREADS];

    printf("calculating, please wait...\n");

    for (int i = 0; i < CNT_THREADS; i++)
        pthread_create(&threads_pool[i], NULL, &routine, (void *)i);
    for (int i = 0; i < CNT_THREADS; i++)
        pthread_join(threads_pool[i], NULL);

    printf("with lock: %s, cnt threads: %d\n", with_lock ? "true" : "false", CNT_THREADS);
    printf("critical_section=%zu\n", critical_section);
    printf("expected value  =%zu\n", CNT_THREADS * count_iterate);
    return 0;
}
