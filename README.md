## Synchronization primitives
- `Algorithm Peterson's for 2 threads`
- `Algorithm Peterson's for N threads`
- `TicketLock`
- `SharedTicketLock (r/w lock)`
- `WaitDieLock (for N threads and K looks)`

### How to run
`make && ./(Peterson2|PetersonN|TicketLock|SharedTicketLock|WaitDieLock)`
### Algorithm Peterson's for N threads
#### how look structure
```c
struct lock {
    atomic_int depth[CNT_THREADS];
    atomic_int last [CNT_THREADS-1];
}
```
#### Lock for one thread of N (isclear function see in code)
```c
void lock_one(struct lock * lock, const int me) {
    const int depth = atomic_fetch_add(&lock->depth[me], 1) + 1;
    atomic_store(&lock->last[depth], me);

    while (!isclear(lock, me, depth) &&
           atomic_load(&lock->last[depth]) == me);
}
void unlock_one(struct lock * lock, const int me) {
    atomic_fetch_sub(&lock->depth[me], 1);
}
```
#### Lock for N threads
```c
void lock(struct lock * lock, const int me) {
    for (int i = 0; i < CNT_THREADS - 1; i++)
        lock_one(lock, me);
}
void unlock(struct lock * lock, const int me) {
    for (int i = 0; i < CNT_THREADS - 1; i++)
        unlock_one(lock, me);
}
```
### Ticker lock (SharedTickerLock similar to this, see in code)
#### How look structure 
```c
struct lock {
    atomic_ullong ticket;
    atomic_ullong queue;
} 
```
#### Lock for N threads 
```c
static void initlock(struct lock * lock) {
    lock->ticket = lock->queue = 0;
}
static void lock(struct lock * lock, const int me) {
    const unsigned long long ticket = atomic_fetch_add(&lock->ticket, 1);

    while (atomic_load(&lock->queue) != ticket);
}
static void unlock(struct lock * lock, const int me) {
    atomic_fetch_add(&lock->queue, 1);
}
```
### WaitDie lock
#### how look structures
```c
struct lock {
    atomic_bool is_locked;
    atomic_ullong timestamp;
};
struct context {
    int top;
    int cnt_captured;
    struct lock * locks[CNT_LOCKS];
    atomic_ullong timestamp;
}
```
#### Initialization for structures
```c
static void initcontext(struct context * ctx) {
    static atomic_ullong timestamp = 1;

    ctx->top = ctx->cnt_captured = 0;
    ctx->timestamp = atomic_fetch_add(&timestamp, 1);
}
static void addlock(struct context * ctx, struct lock * lock) {
    ctx->locks[ctx->top++] = lock;
}
```
#### Lock and unlock function
```c
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
```
