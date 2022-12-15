#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>

#define CNT_THREADS 5

struct lock {
    atomic_ullong ticket;
    atomic_ullong queue;
} mylock;

size_t critical_section     = 0;
const size_t count_iterate  = (size_t)1 << 15;
const bool with_lock        = true;
const bool print_debug_log  = false;


static void initlock(struct lock * lock) {
    lock->ticket = lock->queue = 0;
}
static void lock(struct lock * lock, const int me) {
    const unsigned long long ticket = atomic_fetch_add(&lock->ticket, 1);

    while (atomic_load(&lock->queue) != ticket);

    if (print_debug_log)
        printf("L[%d]", me);
}
static void unlock(struct lock * lock, const int me) {
    if (print_debug_log)
        printf("U[%d]\n", me);

    atomic_fetch_add(&lock->queue, 1);
}


static void * routine(void * arg) {
    const int me = (int)arg;

    for (size_t i = 0; i < count_iterate; i++) {
        if (with_lock) {
            lock(&mylock, me);
            critical_section++;
            unlock(&mylock, me);
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
