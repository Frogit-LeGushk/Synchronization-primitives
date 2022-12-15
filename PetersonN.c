#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>

#define CNT_THREADS 5

struct lock {
    atomic_int depth[CNT_THREADS];
    atomic_int last [CNT_THREADS-1];
} mylock;

size_t critical_section     = 0;
const size_t count_iterate  = (size_t)1 << 15;
const bool with_lock        = true;
const bool print_debug_log  = false;


static void initlock(struct lock * lock) {
    for (int i = 0; i < CNT_THREADS; i++)
        lock->depth[i] = -1;
    for (int i = 0; i < CNT_THREADS - 1; i++)
        lock->last[i] = 0;
}
static bool isclear(struct lock const * lock, const int me, const int depth) {
    for (int i = 0; i < CNT_THREADS; i++)
        if (i != me && atomic_load(&lock->depth[i]) >= depth)
            return false;
    return true;
}
static void lock_one(struct lock * lock, const int me) {
    const int depth = atomic_fetch_add(&lock->depth[me], 1) + 1;
    atomic_store(&lock->last[depth], me);

    while (!isclear(lock, me, depth) &&
           atomic_load(&lock->last[depth]) == me);
}
static void unlock_one(struct lock * lock, const int me) {
    atomic_fetch_sub(&lock->depth[me], 1);
}
static void lock(struct lock * lock, const int me) {
    for (int i = 0; i < CNT_THREADS - 1; i++)
        lock_one(lock, me);

    if (print_debug_log)
        printf("L[%d]", me);
}
static void unlock(struct lock * lock, const int me) {
    if (print_debug_log)
        printf("U[%d]\n", me);

    for (int i = 0; i < CNT_THREADS - 1; i++)
        unlock_one(lock, me);
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

    for (uint64_t i = 0; i < CNT_THREADS; i++)
        pthread_create(&threads_pool[i], NULL, &routine, (void *)i);
    for (uint64_t i = 0; i < CNT_THREADS; i++)
        pthread_join(threads_pool[i], NULL);

    printf("with lock: %s, cnt threads: %d\n", with_lock ? "true" : "false", CNT_THREADS);
    printf("critical_section=%zu\n", critical_section);
    printf("expected value  =%zu\n", CNT_THREADS * count_iterate);
    return 0;
}
