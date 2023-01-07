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

/** Public type for creating a shared memory doubly linked list */
typedef struct shmlist {
    /* Current list element. This is unsafe to use. */
    size_t cur_idx_unsafe;

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
 * @return 0 if a copy of ele was added to the list tail, otherwise non-zero
 */
int shmlist_add_tail_safe(shmlist_t* sl, void* ele);

/**
 * @return 0 if element was deleted from list, otherwise non-zero
 */
int shmlist_del_safe(shmlist_t* sl);

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
 * @param[in] sl List struct
 * @param[in] value value to pass to lhs of comparison function
 * @param[in] elecmp Element comparison function
 * @param[out] ele a locally allocated copy of match (caller must free this memory)
 */
int shmlist_extract_first_match_safe(shmlist_t *sl, void *value, shmlist_elecmp_fn elecmp, void **ele);

/**
 * Remove up to n matching elements from list and return local copies of the data
 * @return 0 if an element was matched and returned, non-zero if no match was found
 * @param[in] sl List struct
 * @param[in] match_max the maximum number of elements to return
 * @param[in] value value to pass to lhs of comparison function
 * @param[in] elecmp Element comparison function
 * @param[out] elecnt the number of matches returned
 * @param[out] ele an array of locally allocated matches (caller must free this memory)
 */
int shmlist_extract_n_matches_safe(shmlist_t *sl, size_t match_max, void *value, shmlist_elecmp_fn elecmp, 
								   size_t* elecnt, void **ele);

/**
 * @return the length of the list
 */
int shmlist_length(shmlist_t *sl);

/**
 * @return a pointer to the list head element
 */
shmlist_t* shmlist_head(shmlist_t *sl);

/**
 * @return a pointer to the list tail element
 */
shmlist_t* shmlist_tail(shmlist_t *sl);

/**
 * @return a pointer to the next element
 */
shmlist_t* shmlist_next(shmlist_t *sl);

/**
 * @return a pointer to the previous element
 */
shmlist_t* shmlist_prev(shmlist_t *sl);

/**
 * @return a pointer to the data at this list entry
 */
void* shmlist_get_data(shmlist_t *sl);

/**
 * @return a copy of ele after this list ptr
 */
int shmlist_insert_after_safe(shmlist_t *sl, void* ele);

/**
 * @return 0 if a copy of ele was inserted before this list ptr, otherwise non-zero
 */
int shmlist_insert_before_safe(shmlist_t *sl, void* ele);

#ifdef __cplusplus
}
#endif

#endif