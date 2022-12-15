#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdlib.h>

#define CNT_THREADS     10
#define CNT_LOCKS       10

struct lock {
    atomic_bool is_locked;
    atomic_ullong timestamp;
} mylocks[CNT_LOCKS] = {0};

struct context {
    int top;
    int cnt_captured;
    struct lock * locks[CNT_LOCKS];
    atomic_ullong timestamp;
};

size_t critical_section     = 0;
const size_t count_iterate  = (size_t)1 << 13;
const bool with_lock        = true;
const bool print_debug_log  = false;


static void initcontext(struct context * ctx) {
    static atomic_ullong timestamp = 1;

    ctx->top = ctx->cnt_captured = 0;
    ctx->timestamp = atomic_fetch_add(&timestamp, 1);
}
static void addlock(struct context * ctx, struct lock * lock) {
    ctx->locks[ctx->top++] = lock;
}


static void unlock(struct context * ctx) {
    for (int i = 0; i < ctx->cnt_captured; i++) {
        struct lock * lock  = ctx->locks[i];
        atomic_store(&lock->is_locked, false);
    }
    ctx->cnt_captured = 0;
}
static bool lock(struct context * ctx) {
    atomic_bool expected = false;

    // try take all locks in context
    for (int i = 0; i < CNT_LOCKS; i++) {
        struct lock * lock  = ctx->locks[i];
        const unsigned long long ctx_ts = ctx->timestamp;

        // try take current lock
        while (!atomic_compare_exchange_strong(&lock->is_locked, &expected, true)) {
            atomic_store(&expected, false); // <- see dock why it is here

            const unsigned long long lock_ts = atomic_load(&lock->timestamp);

            // time to die...
            if (lock_ts < ctx_ts) {
                unlock(ctx);
                return false;
            }
        }

        ctx->cnt_captured++;
    }

    return true;
}


static void * routine(void * arg) {
    (void) arg;

    // initialize context
    struct context ctx;
    initcontext(&ctx);

    // initialize list of locks
    for (int i = 0; i < CNT_LOCKS; i++)
        addlock(&ctx, &mylocks[i]);

    // shuffle
    for (int i = 0; i < 2*CNT_LOCKS; i++) {
        int idx1 = i % CNT_LOCKS;
        int idx2 = rand() % ctx.top;

        struct lock * tmp = ctx.locks[idx1];
        ctx.locks[idx1] = ctx.locks[idx2];
        ctx.locks[idx2] = tmp;
    }

    // apply wait-die lock
    for (size_t i = 0; i < count_iterate; i++) {
        if (!lock(&ctx)) {
            // time to die...
            i--; continue;
        }
        critical_section++;
        unlock(&ctx);
    }

    return NULL;
}

int main() {
    pthread_t threads_pool[CNT_THREADS];

    printf("calculating, please wait...\n");

    for (int i = 0; i < CNT_THREADS; i++)
        pthread_create(&threads_pool[i], NULL, &routine, (void *)i);
    for (int i = 0; i < CNT_THREADS; i++)
        pthread_join(threads_pool[i], NULL);

    printf("with lock: %s, cnt threads: %d, cnt locks: %d\n", with_lock ? "true" : "false", CNT_THREADS, CNT_LOCKS);
    printf("critical_section=%zu\n", critical_section);
    printf("expected value  =%zu\n", CNT_THREADS * count_iterate);
    return 0;
}
