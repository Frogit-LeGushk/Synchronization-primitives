#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>

#define CNT_THREADS 2

struct lock {
    atomic_bool flags[CNT_THREADS];
    atomic_bool last;
} mylock;

size_t critical_section     = 0;
const size_t count_iterate  = (size_t)1 << 20;
const bool with_lock        = true;
const bool print_debug_log  = false;

static void initlock(struct lock * lock) {
    lock->flags[0] = lock->flags[1] = lock->last = false;
}
static void lock(struct lock * lock, const bool me) {
    const bool other = 1 - me;

    // --> can be interrupt
    atomic_store(&lock->flags[me], true);
    // --> can be interrupt
    atomic_store(&lock->last, me);
    // --> can be interrupt
    while (atomic_load(&lock->flags[other]) &&
    // --> can be interrupt
           atomic_load(&lock->last) == me);

    if (print_debug_log)
        printf("L[%d]", me);
}
static void unlock(struct lock * lock, const bool me) {
    if (print_debug_log)
        printf("U[%d]\n", me);

    atomic_store(&lock->flags[me], false);
}


static void * routine(void * arg) {
    bool me = *(bool *)arg;

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
    pthread_t th1;
    pthread_t th2;

    bool my_tid1 = false;
    bool my_tid2 = 1 - my_tid1;

    initlock(&mylock);

    printf("calculating, please wait...\n");
    pthread_create(&th1, NULL, &routine, (void *)(&my_tid1));
    pthread_create(&th2, NULL, &routine, (void *)(&my_tid2));
    pthread_join(th1, NULL);
    pthread_join(th2, NULL);

    printf("with lock: %s, cnt threads: %d\n", with_lock ? "true" : "false", CNT_THREADS);
    printf("critical_section=%zu\n", critical_section);
    printf("expected value  =%zu\n", 2 * count_iterate);
    return 0;
}
