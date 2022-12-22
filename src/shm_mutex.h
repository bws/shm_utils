#ifndef SHM_MUTEX_H
#define SHM_MUTEX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/** Mutex lock states */
#define SHMMUTEX_LOCK_NOTREADY 0
#define SHMMUTEX_LOCK_AVAILABLE 1
#define SHMMUTEX_LOCK_TAKEN 2

/** 
 * Public type used to perform mutual exclusion between processes. This type
 * must be stored in memory that can be read and written from all processes
 * participating in the concurrency control (i.e. put it in SHM) 
 * 
 * Mutex states/values:
 * 0 - not ready, no one can acquire the lock
 * 1 - lock is available
 * 2 - lock is taken
 * 
 * Note that 0 is defined this way because shared memory is zero-filled
 * before use. Nested lock calls are not supported.
 * 
 */
typedef struct shmmutex {
    /* This futex can have the values 0, 1, 2. */
    uint32_t val;
} shmmutex_t;

/**
 * Create the mutex. There are 3 important preconditions before calling
 * this function:
 *   1. The data structure must be stored in memory visible from
 *      every process that uses the mutex 
 *   2. The memory must be zero-filled before calling create
 *   3. The create call must only be called by a single process
 * 
 *  This mutex is specifically designed to work for zero-filled shared
 *  memory segments acquired using shm_open+ftruncate+mmap
 * 
 * @param sm pointer to the mutex data
 * @return 0 on success
 */
int shmmutex_create(shmmutex_t *sm);

/**
 * Destroy the mutex. The shared mutex cannot be used by anyone after this call.
 * 
 * @param sm pointer to the mutex data
 * @return 0 on success
 */
int shmmutex_destroy(shmmutex_t *sm);

/** 
 * On success a single process holds the lock 
 * 
 * @param sm pointer to the mutex data
 * @return 0 on success, non-zero on failure
 * 
*/
int shmmutex_lock(shmmutex_t *sm);

/** 
 * On success a process holding the lock releases it 
 * 
 * @param sm pointer to the mutex data
 * @return 0 on success, non-zero on failure
 * 
*/
int shmmutex_unlock(shmmutex_t *sm);

#ifdef __cplusplus
}
#endif

#endif