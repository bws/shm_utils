/**
 * A vector in shared memory. 
 *
 * E.g.
 * shmvector_t sv;
 * shmvector_alloc(&sv, sizeof(int), 1024);
 *
 * Using access functions adds the ability to grow the array dynamically
 * and reuse space. Direct access to memory removes that capability.
 *
 */
#ifndef SHM_VECTOR_H
#define SHM_VECTOR_H

#include <stdbool.h>
#include "shm_mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Functor used to compare elements for find algorithms 
 * @return 0 on equality, 1 on non-equality
 */
typedef int (*shmvector_elecmp_fn)(void* lhs, void* rhs);

/* Private type for creating an array with holes in shared memory */
typedef struct shmarray shmarray_t;

/** Public type for creating a shared memory vector */
typedef struct shmvector {
	/* Name of the shared memory segment to store data within */
	const char* segname;

	/* File descriptor for the shared memory segment */
	int segd;

	/* An array of items stored in shared memory */
	shmarray_t* shm;
} shmvector_t;

/* Private type for creating an array with holes in shared memory */
typedef struct shmarray {
	/* Mutual exclusion lock */
	shmmutex_t lock;
	
	/* Number of total buffers allocated */
	size_t capacity;

	/* Size of each array element */
	size_t esize;

	/* Index to use for the next push_back */
	size_t next_back_idx;

	/* Number of allocated buffers in use */
	size_t active_count;

	/* Array of shared memory array buffers */
	void* eles;

	/* Array of booleans indicating whether a buffer is active */
	bool* actives;
} shmarray_t;

/**
	Create and allocate a new shared memory vector
	@param sv Struct to fill in
	@param segname Name of the shared memory segment to use
	@param elesz Size of each vector element
	@param sz Number of vector elements to allocate
*/
int shmvector_create(shmvector_t *sv, const char* segname, size_t elesz, size_t sz);

/**
 * Release resources associated with this shared memory vector
*/
int shmvector_destroy(shmvector_t *sv);

/**
 * @return the number of active elements in the vector
 */
size_t shmvector_size(shmvector_t *sv);

/**
 * @return the index of the element compares equal,
 *         or size+1 if the element does not exist
 */
size_t shmvector_find_first_of(shmvector_t *sv, void *data, shmvector_elecmp_fn elecmp);

/**
 * Concurrent safe push_back() function
 * 
 * @return the index to which ele is copied
*/
int shmvector_safe_push_back(shmvector_t* sv, void* ele);

/**
 * @return the index to which ele is copied
 */
int shmvector_push_back(shmvector_t* sv, void* ele);

/**
 * @return the index to which ele is copied
 */
int shmvector_insert_at(shmvector_t* sv, size_t idx, void* ele);

/**
 * Concurrent safe at() function
 * 
 * @return the element at idx or NULL if no such element exists
*/
void* shmvector_safe_at(shmvector_t* sv, size_t idx);

/**
 * @return the element at idx or NULL if no such element exists
*/
void* shmvector_at(shmvector_t* sv, size_t idx);

/**
 * Allocate an element at the first location found by a simple search
 * @return a pointer to the allocated element
*/
int shmvector_insert_quick(shmvector_t* sv);

/**
 * Delete element at index idx
 * @return 0 on success, non-zero on failure
*/
int shmvector_del(shmvector_t* sv, size_t idx);


/**
 * Double the size of the shared vector
*/
//int shmvector_grow_array(shmvector_t *sv);

#ifdef __cplusplus
}
#endif

#endif