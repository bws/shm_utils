#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shm_mutex.h"
#include "shm_list.h"

/* Create a new list item for the list that contains existing */
static void shmlist_create_tail(shmlist_t *nitem, shmlist_t *existing, size_t idx) {
    nitem->list = existing->list;
    nitem->next = existing->list;
    nitem->prev = existing->list->prev;
    nitem->idx = idx;
    nitem->v = existing->v;
    nitem->data = nitem + 1;
}

/* Allocate and fill the buffer with the element from the list node */
static void* shmlist_malloc_copy(shmlist_t* node) {
    size_t elesz = node->v->shm->esize - sizeof(shmlist_t);
    void* ele = malloc(elesz);
    if (NULL != ele) {
        memcpy(ele, node->data, elesz);
    }
    return ele;
}

/* Allocate and fill the buffer with the element from the list node */
static void* shmlist_malloc_copy_data(shmlist_t* node) {
    size_t elesz = node->v->shm->esize - sizeof(shmlist_t);
    void* ele = malloc(elesz);
    fprintf(stderr, "Extracting data of size: %d\n", elesz);
    if (NULL != ele) {
        memcpy(ele, node->data, elesz);
    }
    return ele;
}
/* 
    Create and allocate a new shared memory list 
    Note: We keep an empty value at the beginning of the list
        that is not the user's list "head"
*/
int shmlist_create(shmlist_t *sl, const char* segname, size_t elesz, size_t sz) {
    /* Create the vector */
    shmvector_t *v = malloc(sizeof(shmvector_t));
    /* Vector size must be at least 1 to support empty lists */
    shmvector_create(v, segname, sizeof(shmlist_t) + elesz, sz + 1);
    sl->v = v;

    /* Critical section: initialize the head once */
    shmmutex_lock(&(sl->v->shm->lock));
    if (0 == shmvector_size(sl->v)) {
        /* Setup the list ptrs */
        sl->list = sl;
        sl->next = sl;
        sl->prev = sl;

        /* The head always uses the first element of the vector */
        int rc = shmvector_insert_at(sl->v, 0, sl);
        assert(0 == rc);
        sl->idx = 0;

        /* Set the data ptr to point to the end of this item */
        sl->data = (shmlist_t*)shmvector_at(sl->v, 0) + 1;
    }
    shmmutex_unlock(&(sl->v->shm->lock));
}

/* Release resources associated with this shared memory list */
int shmlist_destroy(shmlist_t *sl) {
    sl->list = NULL;
    sl->next = NULL;
    sl->prev = NULL;
    shmvector_destroy(sl->v);
    free(sl->v);
}

/**
 * Add a copy of ele to an available slot in sv
 */
int shmlist_add_tail_safe(shmlist_t* sl, void* ele) {
    int rc = 0;

    shmmutex_lock(&(sl->v->shm->lock));

    /* Create the new list node */
    int idx = shmvector_insert_quick(sl->v);
    if (idx < 0) {
        rc = 1;
        fprintf(stderr, "ERROR: Shared %s failed.\n", __FUNCTION__);
    }
    shmlist_t* ntail = shmvector_at(sl->v, idx);
    shmlist_create_tail(ntail, sl, idx);
    memcpy(ntail->data, ele, ntail->v->shm->esize);

    /* Insert the new tail into the list */
    sl->list->prev->next = ntail;
    sl->list->prev = ntail;

    shmmutex_unlock(&(sl->v->shm->lock));

    return rc;
}

/**
 * Delete this element from the list
 */
int shmlist_del_safe(shmlist_t* sl) {
    int rc = 0;
    shmmutex_lock(&(sl->v->shm->lock));
    /* Deleting the dummy node at the list beginning is a no-op */
    if (sl != sl->list) {
        /* Adjust adjacent list entries */
        sl->prev->next = sl->next;
        sl->next->prev = sl->prev;

        /* Deallocate the space */
        rc = shmvector_del(sl->v, sl->idx);
    } 
    shmmutex_unlock(&(sl->v->shm->lock));
    return rc;
}

/**
 * @return true if the list is empty
 */
int shmlist_is_empty(shmlist_t *sl) {
    return (sl->list == sl->list->next);
}

/** return Remove head from list and return a local copy  */
int shmlist_extract_head_safe(shmlist_t *sl, void** head_data) {
    shmmutex_lock(&(sl->v->shm->lock));
    int rc = 0;
    shmlist_t *phead = sl->list->next;
    if (sl->list == sl->list->next) {
        rc = 1;
    }
    else {
        /* Splice out phead */
        sl->list->next = phead->next;
        phead->next->prev = sl->list;

        /* Make a copy of phead data */
        *head_data = shmlist_malloc_copy(phead);
        
        /* Mark the phead memory as available for reuse */
        shmvector_del(phead->v, phead->idx);
    }
    shmmutex_unlock(&(sl->v->shm->lock));
    return rc;
}

/** Remove matching element from list and return a local copy of the data */
int shmlist_extract_first_match_safe(shmlist_t *sl, void* val, shmlist_elecmp_fn elecmp, void** match) {

    int rc = 1;
    shmmutex_lock(&(sl->v->shm->lock));
    shmlist_t *iter = sl->list->next;
    while (iter != sl->list) {
        if (0 == elecmp(val, iter->data)) {
            fprintf(stderr, "Found item\n");
            /* Splice out iter */
            iter->prev->next = iter->next;
            iter->next->prev = iter->prev;

            /* Make a copy of iter data */
            *match = shmlist_malloc_copy_data(iter);
        
            /* Mark the phead memory as available for reuse */
            shmvector_del(iter->v, iter->idx);
            rc = 0;
            break;
        }
        iter = iter->next;
    }
    shmmutex_unlock(&(sl->v->shm->lock));
    return rc;
}

/** return the length of the list  */
int shmlist_length(shmlist_t *sl) {
    /* Get the number of live elements in the vector minus the empty list head */
    return sl->v->shm->active_count - 1;
}

/** return a pointer to the list head element */
shmlist_t* shmlist_head(shmlist_t *sl) {
    return sl->list->next;
}

/* return a pointer to the next element */
shmlist_t* shmlist_next(shmlist_t *sl) {
    return sl->next;
}

/* Insert a copy of ele after this list ptr */
int shmlist_insert_after_safe(shmlist_t *sl, void* ele) {
    return -1;
}

/* Insert a copy of ele before this list ptr */
int shmlist_insert_before_safe(shmlist_t *sl, void* ele) {
    return -1;
}
