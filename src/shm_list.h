/**
 * A linked list with its values stored in shared memory. 
 *
 * Sample usage:
 *   shmlist_t sl;
 *   shmlist_alloc(&sl, sizeof(int), 1024);
 *   int v1 = 64, v2 = 1024;
 *   shmlist_add_tail(&sl, &v1);
 *   shmlist_add_head(&sl, &v2);
 *   int* v3;
 *   // List removals receive a copy of the data
 *   v3 = shmlist_pop_front(&sl);
 *   free(v3);
 */
#ifndef SHM_LIST_H
#define SHM_LIST_H

#include <stdbool.h>
#include <stddef.h>
#include "shm_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct shmlist shmlist_t;

/** Public type for creating a shared memory doubly linked list */
typedef struct shmlist {
    /* List head */
    shmlist_t* head;

    /* Next */
    shmlist_t* next;

    /* Next */
    shmlist_t* prev;

	/* A shared memory vector to store data */
	shmvector_t* v;

	/* Index within the vector that stores data for this node */
	size_t idx;

} shmlist_t;


/**
	Create and allocate a new shared memory list
	@param sl Struct to fill in
	@param segname Name of the shared memory segment to use
	@param elesz Size of each list element
	@param sz Number of list elements to preallocate
*/
int shmlist_create(shmlist_t *sl, const char* segname, size_t elesz, size_t sz);

/**
 * Release resources associated with this shared memory list
*/
int shmlist_destroy(shmlist_t *sl);

/**
 * Add a copy of ele to an available slot in sv
 */
int shmlist_add_tail(shmlist_t* sl, void* ele);

/**
 * Delete ele from list
 */
int shmlist_del(shmlist_t* sl);

/**
 * Double the size of the shared vector
 */
int shmlist_for_each_safe(shmlist_t *sl);

/**
 * @return true if the list is empty
 */
int shmlist_is_empty(shmlist_t *sl);

/**
 * @return Remove head from list and return a local copy
 */
void* shmlist_extract_head(shmlist_t *sl);

/**
 * @return the length of the list
 */
int shmlist_length(shmlist_t *sl);

/**
 * @return a pointer to the list head element
 */
void* shmlist_head(shmlist_t *sl);

/**
 * @return a pointer to the next element
 */
int shmlist_next(shmlist_t *sl);

/**
 * Insert a copy of ele after this list ptr
 */
int shmlist_insert_after(shmlist_t *sl, void* ele);

/**
 * Insert a copy of ele before this list ptr
 */
int shmlist_insert_before(shmlist_t *sl, void* ele);

#ifdef __cplusplus
}
#endif

#endif