#ifndef HC_ONCE_H
#define HC_ONCE_H

/*
 * Portable one-time initialization.
 *
 * Windows: uses Interlocked operations on a volatile LONG — no <stdatomic.h>
 *          required, works on all MSVC versions targeting Windows Vista+.
 * Other:   uses C11 <stdatomic.h> with atomic_flag + atomic_int.
 *
 * Usage:
 *   static hc_once_t s_my_once = HC_ONCE_INIT;
 *   hc_call_once(&s_my_once, my_init_fn);
 *
 * The init function is called exactly once across all threads. Concurrent
 * callers spin (yielding via Sleep(0)/no-op) until initialization completes.
 */

#if defined(_WIN32)

#include <windows.h>

/* States encoded in a LONG: 0 = not started, 1 = in progress, 2 = done */
typedef volatile LONG hc_once_t;
#define HC_ONCE_INIT 0

static inline void hc_call_once(hc_once_t* state, void (*fn)(void)) {
    if (*state == 2) return; /* fast path: already done */
    if (InterlockedCompareExchange(state, 1, 0) == 0) {
        /* This thread won the race — run the initializer */
        fn();
        InterlockedExchange(state, 2);
    } else {
        /* Another thread is initializing — yield and spin */
        while (*state != 2)
            Sleep(0);
    }
}

#else /* POSIX / Android — C11 atomics */

#include <stdatomic.h>

typedef struct {
    atomic_flag begun; /* test-and-set: the winning thread clears it */
    atomic_int  done;  /* set to 1 after init completes              */
} hc_once_t;

#define HC_ONCE_INIT { ATOMIC_FLAG_INIT, 0 }

static inline void hc_call_once(hc_once_t* state, void (*fn)(void)) {
    if (atomic_load_explicit(&state->done, memory_order_acquire))
        return; /* fast path: already done */
    if (!atomic_flag_test_and_set_explicit(&state->begun, memory_order_acq_rel)) {
        /* This thread won the race — run the initializer */
        fn();
        atomic_store_explicit(&state->done, 1, memory_order_release);
    } else {
        /* Another thread is initializing — spin until done */
        while (!atomic_load_explicit(&state->done, memory_order_acquire))
            ;
    }
}

#endif /* _WIN32 */

#endif /* HC_ONCE_H */
