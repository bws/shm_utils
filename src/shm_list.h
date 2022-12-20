/**
 * A circular linked list with its values stored in shared memory. 
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

/** 
 * Functor used to compare elements for find algorithms 
 * @return 0 on equality, 1 on non-equality
 */
typedef int (*shmlist_elecmp_fn)(void* lhs, void* rhs);

typedef struct shmlist shmlist_t;

/** Public type for creating a shared memory doubly linked list */
typedef struct shmlist {
    /* A dummy item just before the list head */
    shmlist_t* list;

    /* Next */
    shmlist_t* next;

    /* Previous */
    shmlist_t* prev;

	/* Index within the vector that stores data for this node */
	size_t idx;

	/* Pointer to just the element */
	void* data;

	/* A shared memory vector to store data */
	shmvector_t* v;
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
int shmlist_add_tail_safe(shmlist_t* sl, void* ele);

/**
 * Delete ele from list
 */
int shmlist_del_safe(shmlist_t* sl);

/**
 * Iterate over members of the list, the user may invalidate the current entry.
 */
#define shmlist_for_each_safe(sl, iter1, iter2) \
    for (iter1 = ((sl == sl->list) ? sl->next : sl), iter2 = iter1->next; \
         iter1 != sl->list; \
         iter1 = iter2, iter2 = iter2->next)

/**
 * @return true if the list is empty
 */
int shmlist_is_empty(shmlist_t *sl);

/**
 * @return Remove head from list and return a local copy of the data
 * @param sl List struct
 * @param head a locally allocated copy of head (caller must free this memory)
 */
int shmlist_extract_head_safe(shmlist_t *sl, void** head);

/**
 * Remove matching element from list and return a local copy of the data
 * @return 0 if an element was matched and returned, non-zero if no match was found
 * @param sl List struct
 * @param match a locally allocated copy of match (caller must free this memory)
 * @param elecmp Element comparison function
 */
int shmlist_extract_first_match_safe(shmlist_t *sl, void** match, shmlist_elecmp_fn elecmp);

/**
 * @return the length of the list
 */
int shmlist_length(shmlist_t *sl);

/**
 * @return a pointer to the list head element
 */
shmlist_t* shmlist_head(shmlist_t *sl);

/**
 * @return a pointer to the next element
 */
shmlist_t* shmlist_next(shmlist_t *sl);

/**
 * Insert a copy of ele after this list ptr
 */
int shmlist_insert_after_safe(shmlist_t *sl, void* ele);

/**
 * Insert a copy of ele before this list ptr
 */
int shmlist_insert_before_safe(shmlist_t *sl, void* ele);

#ifdef __cplusplus
}
#endif

#endif