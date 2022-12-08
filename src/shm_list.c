
#include <stddef.h>
#include <stdio.h>
#include "shm_list.h"

/* Create and allocate a new shared memory list */
int shmlist_create(shmlist_t *sl, const char* segname, size_t elesz, size_t sz) {
    /* Create the vector */
    shmvector_t *v = malloc(sizeof(shmvector_t));
    shmvector_create(v, segname, elesz, sz);
    sl->v = v;

    /* Setup the list ptrs */
    sl->head = NULL;
    sl->next = sl->head;
    sl->prev = sl->head;

    /* The head always uses the first element of the vector */
    sl->idx = 0;
}

/* Release resources associated with this shared memory list */
int shmlist_destroy(shmlist_t *sl) {
    sl->head = NULL;
    sl->next = NULL;
    sl->prev = NULL;
    free(sl->v);
}

/**
 * Add a copy of ele to an available slot in sv
 */
int shmlist_add_tail(shmlist_t* sl, void* ele) {
    int rc = 0;

    /* Create the new list node */
    shmlist_t* ntail = malloc(sizeof(shmlist_t));
    ntail->next = sl->head;
    ntail->prev = sl->head->prev;
    ntail->v = sl->head->v;
    ntail->idx = shmvector_insert_quick(ntail->v, ele);
    if (ntail->idx < 0) {
        rc = 1;
        fprintf(stderr, "ERROR: Shared %s failed.\n", __FUNCTION__);
    }

    /* Insert the new tail into the list */
    sl->head->prev->next = ntail;
    sl->head->prev = ntail;

    return rc;
}

/**
 * Delete ele from list
 */
int shmlist_del(shmlist_t* sl) {

}

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

/* Rreturn a pointer to the next element */
int shmlist_next(shmlist_t *sl) {

}

/* Insert a copy of ele after this list ptr */
int shmlist_insert_after(shmlist_t *sl, void* ele) {

}

/* Insert a copy of ele before this list ptr */
int shmlist_insert_before(shmlist_t *sl, void* ele) {

}
