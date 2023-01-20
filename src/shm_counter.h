/**
 * A shared counter that supports atomic increment/decrement. 
 *
 * Sample usage:
 *   shmcounter_t sc;
 *   shmcounter_create(&sc);
 *   shmcounter_inc(&sc, 1);
 *   shmcounter_dec(&sl, 3);
 *   if (shmcounter_isequal(&sc, 0))
 *     fprintf(stdout, "Counter is 0\n");
 *   shmcounter_destroy(&sc);
 */
#ifndef SHM_COUNTER_H
#define SHM_COUNTER_H

#include <stdbool.h>
#include <stddef.h>
#include "shm_mutex.h"
#include "shm_vector.h"
#ifdef __cplusplus
extern "C" {
#endif

/** Number of counters allowed within a set */
#define SHMCOUNTER_SET_SIZE 2048

/** Reserved values that cannot be used within the UID */
#define SHMCOUNTER_RESERVED_GROUP 0xDEADBEEF
#define SHMCOUNTER_RESERVED_CTYPE 0xDEADBEEF
#define SHMCOUNTER_RESERVED_TAG 0xDEADBEEF

/** A counter id with sufficient width to uniquely match MPI collectives */
typedef struct shmcounter_uid {
    /** Group this counter belongs to */
    uint64_t group;
    /** CType this counter belongs to */
    uint64_t ctype;
    /** Tag this counter belongs to */
    uint64_t tag;
    /** Local id */
    uint64_t lid;
} shmcounter_uid_t;

/** The data stored within the shared storage for this counter */
typedef struct shmcounter_data {
    /* Mutex for this counter */
    shmmutex_t mutex;

    /* Unique id for this counter */
    shmcounter_uid_t id;

    /* Number of active reader/writers for this counter */
    size_t refcount;

    /* Value of this counter*/
    int count;
} shmcounter_data_t;

/** Public type for creating a set of shared counters */
typedef struct shmcounter_set {
	/* A shared memory vector to store data */
	shmvector_t* v;

} shmcounter_set_t;

/** Public type for creating a shared counter */
typedef struct shmcounter {
    /* The counter set this counter belongs to */
    shmcounter_set_t *set;
    
    /* Index of the vector where the counter value is stored */
    size_t idx;

} shmcounter_t;


/**
	Create and allocate a new shared counter. Initialize it to 0.
	@param sc Struct to fill in
*/
int shmcounter_set_create(shmcounter_set_t *scs, const char* counterset);

/**
 * Release resources associated with this shared memory counter.
 * @param sc Counter struct
*/
int shmcounter_set_destroy(shmcounter_set_t *scs);

/**
	Create and allocate a new shared counter. Initialize it to 0.
	@param sc Struct to fill in
*/
int shmcounter_create(shmcounter_t *sc, shmcounter_set_t *scs, shmcounter_uid_t cid);

/**
 * Release resources associated with this shared memory counter.
 * @param sc Counter struct
*/
int shmcounter_destroy(shmcounter_t *sc);

/**
 * Increment the counter
 * @param sc Counter struct
 * @param val Amount to increment counter
 */
void shmcounter_inc_safe(shmcounter_t* sc, int val);

/**
 * Decrement the counter
 * @param sc Counter struct
 * @param val Amount to increment counter
 */
void shmcounter_dec_safe(shmcounter_t* sc, int val);

/**
 * @return true if the value was 0 and the value is now val. Otherwise false.
 * @param sc Counter struct
 * @param val Value to set the counter to
 */
bool shmcounter_set_if_zero_safe(shmcounter_t* sc, int val);

/** 
 * @return the value of the counter 
 */
int shmcounter_value(shmcounter_t* sc);

/**
 * Compare the value of the counter
 * @param sc Counter struct
 * @param val Amount to compare counter to
 * @return true if the counter equals val, otherwise false
 */
bool shmcounter_isvalue(shmcounter_t* sc, int val);

/**
 * Compare two counters for equality with mutex safety
 * @param lhs Counter struct
 * @param rhs Counter struct
 * @return true if the counters have the same value, otherwise false
 * 
 * @note Currently assumes both counters are from the same counter set.
 */
bool shmcounter_isequal_safe(shmcounter_t* lhs, shmcounter_t* rhs);

#ifdef __cplusplus
}
#endif

#endif