#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <sys/time.h>
#include "shm_mutex.h"

/** 
 * Wrapper for the futex system call
 * 
 * @param uaddr
 * @param op the futex operation to perform (WAIT, WAKE, etc)
 * @param val value that is shared amongst locking processes
 * @param timeout timeout
 * @param uaddr2 ???
 * @param val3 ???
*/
static int futex(uint32_t *uaddr, int op, uint32_t val,
                 const struct timespec *timeout, uint32_t *uaddr2, uint32_t val3) {
    return syscall(SYS_futex, uaddr, op, val, timeout, uaddr2, val3);
}

/* Create the mutex variable and make it available. Called from 1 process only. */
int shmmutex_create(shmmutex_t *sm) {
    sm->val = SHMMUTEX_LOCK_AVAILABLE;
    return 0;
}

/* Destroy the mutex. This can be safely called by all processes */
int shmmutex_destroy(shmmutex_t *sm) {
    /* Ensure no one is holding the lock and then set it to NOTREADY */
    int rc = 0;
    long s;

    /* All destroyers race to be the first to safely set lock to NOTRREADY */
    while (sm->val != SHMMUTEX_LOCK_NOTREADY) {
        /* If the futex is available, set it to notready */
        const uint32_t avail = SHMMUTEX_LOCK_AVAILABLE;
        if (atomic_compare_exchange_strong(&(sm->val), &avail, SHMMUTEX_LOCK_NOTREADY))
            break;      /* Yes */

        /* Futex is not available and it is ready; wait. */
        s = futex(&(sm->val), FUTEX_WAIT, SHMMUTEX_LOCK_TAKEN, NULL, NULL, 0);
        if (s == -1 && errno != EAGAIN) {
            fprintf(stderr, "ERROR: Failure while trying to lock\n");
            rc = 1;
        }
    }
    return 0;
}

/* Destroy a locked mutex. */
int shmmutex_destroy_if_locked(shmmutex_t *sm) {
    /* Ensure the lock is held and then set it to NOTREADY */
    int rc = 0;
    long s;

    /* If the futex is taken, set it to notready */
    const uint32_t taken = SHMMUTEX_LOCK_TAKEN;
    if (atomic_compare_exchange_strong(&(sm->val), &taken, SHMMUTEX_LOCK_NOTREADY)) {
        /* We destroyed the futex, notify processes that may be sleep-waiting */
        s = futex(&sm->val, FUTEX_WAKE, SHMMUTEX_LOCK_NOTREADY, NULL, NULL, 0);
        if (s  == -1) {
            fprintf(stderr, "ERROR: Failure waking after destroying lock\n");
            rc = 1;
        }
    }
    else {
        /* The lock was not in the taken state, return an error */
        rc = 1;
    }
    return rc;
}

int shmmutex_lock(shmmutex_t *sm) {

     /* atomic_compare_exchange_strong(ptr, oldval, newval)
       atomically performs the equivalent of:

        if (*ptr == *oldval)
            *ptr = newval;

        It returns true if the test yielded true and *ptr was updated. 
     */
    int rc = 0;
    long s;
    while (1) {
        /* If the futex is available, take it */
        const uint32_t avail = SHMMUTEX_LOCK_AVAILABLE;
        if (atomic_compare_exchange_strong(&(sm->val), &avail, SHMMUTEX_LOCK_TAKEN))
            break;      /* Yes */

        /* Futex is not available; wait. */
        s = futex(&(sm->val), FUTEX_WAIT, SHMMUTEX_LOCK_TAKEN, NULL, NULL, 0);
        if (s == -1 && errno != EAGAIN) {
            fprintf(stderr, "ERROR: Failure while trying to lock\n");
            rc = 1;
        }
    }
    return rc;
}

int shmmutex_unlock(shmmutex_t *sm) {
    int rc = 0;
    long s;
    const uint32_t taken = SHMMUTEX_LOCK_TAKEN;
    if (atomic_compare_exchange_strong(&(sm->val), &taken, SHMMUTEX_LOCK_AVAILABLE)) {
        /* We released the futex, notify processes that may be sleep-waiting */
        s = futex(&sm->val, FUTEX_WAKE, SHMMUTEX_LOCK_AVAILABLE, NULL, NULL, 0);
        if (s  == -1) {
            fprintf(stderr, "ERROR: Failure while release lock\n");
            rc = 1;
        }
    }
    return rc;
}
