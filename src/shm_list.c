#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shm_mutex.h"
#include "shm_list.h"

/** The data stored in the list shared vector */
typedef struct shmlist_element {
	/* Index within the vector that stores data for this node */
	size_t idx;

    /* Next */
    size_t next_idx;

    /* Previous */
    size_t prev_idx;

	/* Pointer to just the data element -- may not be valid */
	void* data_unsafe;
} shmlist_ele_t;

/* Return the data within vector element */
static void* shmlist_ele_get_data(void* v_ele) {
    shmlist_ele_t *ele = v_ele;
    ele->data_unsafe = v_ele + sizeof(shmlist_ele_t);
    return (v_ele + sizeof(shmlist_ele_t));
}

static inline size_t shmlist_get_next_idx(shmlist_t *sl, size_t idx) {
    shmlist_ele_t *li = shmvector_at(sl->v, idx);
    assert(li);
    return li->next_idx;
}
    
static inline size_t shmlist_get_prev_idx(shmlist_t *sl, size_t idx) {
    shmlist_ele_t *li = shmvector_at(sl->v, idx);
    assert(li);
    return li->prev_idx;
}
    
static inline size_t shmlist_set_next_idx(shmlist_t *sl, size_t idx, size_t val) {
    shmlist_ele_t *li = shmvector_at(sl->v, idx);
    assert(li);
    li->next_idx = val;
}
    
static inline size_t shmlist_set_prev_idx(shmlist_t *sl, size_t idx, size_t val) {
    shmlist_ele_t *li = shmvector_at(sl->v, idx);
    assert(li);
    li->prev_idx = val;
}

/* Create a new list item for the list that contains existing */
static void shmlist_create_tail(shmlist_t *sl, size_t idx, void *ntail, void *ele_data) {
    shmlist_ele_t * t_ele = ntail;
    t_ele->idx = idx;
    t_ele->next_idx = 0;
    t_ele->prev_idx = shmlist_get_prev_idx(sl, 0);
    t_ele->data_unsafe = t_ele + 1;
    memcpy(t_ele + 1, ele_data, sl->v->shm->esize - sizeof(shmlist_ele_t));
}

/* Fill a buffer with the element from the list node */
static void* shmlist_copy_data(shmlist_t* sl, void* ele, shmlist_ele_t* node) {
    if (NULL != ele) {
        size_t elesz = sl->v->shm->esize - sizeof(shmlist_ele_t);
        memcpy(ele, shmlist_ele_get_data(node), elesz);
    }
    return ele;
}

/* Allocate and fill the buffer with the element from the list node */
static void* shmlist_malloc_copy_data(shmlist_t* sl, shmlist_ele_t* node) {
    size_t elesz = sl->v->shm->esize - sizeof(shmlist_ele_t);
    void* ele = malloc(elesz);
    if (NULL != ele) {
        memcpy(ele, shmlist_ele_get_data(node), elesz);
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
    shmvector_create(v, segname, sizeof(shmlist_ele_t) + elesz, sz + 1);
    sl->v = v;

    /* Critical section: initialize the head once */
    int rc = shmmutex_lock(&(sl->v->shm->lock));
    if (rc != 0) {
        fprintf(stderr, "Mutex lock failed: %s\n", __func__);
        abort();
    }
    if (0 == shmvector_size(sl->v)) {
        /* The dummy head always uses the first element of the vector */
        shmlist_ele_t* dummy = calloc(1, sizeof(shmlist_ele_t) + elesz);
        int rc = shmvector_insert_at(sl->v, 0, dummy);
        assert(0 == rc);
        free(dummy);
    }

    /* Set the pointer to the dummy idx */
    sl->cur_idx_unsafe = 0;

    shmmutex_unlock(&(sl->v->shm->lock));
    return 0;
}

/* Release resources associated with this shared memory list */
int shmlist_destroy(shmlist_t *sl) {
    shmvector_destroy_safe(sl->v);
    free(sl->v);
}

/**
 * Add a copy of ele to the tail of the list
 */
int shmlist_add_tail_safe(shmlist_t* sl, void* ele_data) {
    int rc = 0;

    shmmutex_lock(&(sl->v->shm->lock));

    /* Create the new list node */
    int tidx = shmvector_insert_quick(sl->v);
    if (tidx <= 0) {
        rc = 1;
        fprintf(stderr, "ERROR: Shared %s failed.\n", __FUNCTION__);
        return rc;
    }
    void* ntail = shmvector_at(sl->v, tidx);
    shmlist_create_tail(sl, tidx, ntail, ele_data);

    /* Insert the new tail into the list */
    size_t prevtail_idx = shmlist_get_prev_idx(sl, 0);
    shmlist_set_next_idx(sl, prevtail_idx, tidx);
    shmlist_set_prev_idx(sl, 0, tidx);

    /* List now points at the new tail */
    sl->cur_idx_unsafe = tidx;

    shmmutex_unlock(&(sl->v->shm->lock));

    return rc;
}

/**
 * Delete this element from the list. This function is difficult to use correctly.
 */
int shmlist_del_safe(shmlist_t* sl) {
    int rc = 0;
    shmmutex_lock(&(sl->v->shm->lock));
    /* Deleting the dummy node at the list beginning is a no-op */
    if (sl->cur_idx_unsafe != 0) {
        
        /* Adjust adjacent list entries */
        size_t adj_prev_idx = shmlist_get_prev_idx(sl, sl->cur_idx_unsafe);
        size_t adj_next_idx = shmlist_get_next_idx(sl, sl->cur_idx_unsafe);
        shmlist_set_next_idx(sl, adj_prev_idx, adj_next_idx);
        shmlist_set_prev_idx(sl, adj_next_idx, adj_prev_idx);

        /* Deallocate the space */
        rc = shmvector_del(sl->v, sl->cur_idx_unsafe);

        /* Update the current list entry to be next */
        sl->cur_idx_unsafe = adj_next_idx;
    }
 
    shmmutex_unlock(&(sl->v->shm->lock));
    return rc;
}

/**
 * @return true if the list is empty
 */
int shmlist_is_empty(shmlist_t *sl) {
    size_t hnext = shmlist_get_next_idx(sl, 0);
    return (0 == hnext);
}

/** return Remove head from list and return a local copy  */
int shmlist_extract_head_safe(shmlist_t *sl, void** head_data) {
    shmmutex_lock(&(sl->v->shm->lock));
    int rc = 0;
    if (shmlist_is_empty(sl)) {
        rc = 1;
    }
    else {
        /* Retrieve the head */
        size_t hidx = shmlist_get_next_idx(sl, 0);
        shmlist_ele_t *phead = shmvector_at(sl->v, hidx);

        /* Splice out the head */
        shmlist_set_next_idx(sl, 0, phead->next_idx);
        shmlist_set_prev_idx(sl, phead->next_idx, phead->prev_idx);

        /* Make a local copy of phead data */
        *head_data = shmlist_malloc_copy_data(sl, phead);
        
        /* Mark the phead memory as available for reuse */
        shmvector_del(sl->v, phead->idx);
    }
    shmmutex_unlock(&(sl->v->shm->lock));
    return rc;
}

/** Remove matching element from list and return a local copy of the data */
int shmlist_extract_first_match_safe(shmlist_t *sl, void* cmpvalue, shmlist_elecmp_fn elecmp, void** match) {

    int rc = 1;
    shmmutex_lock(&(sl->v->shm->lock));
    size_t iter = shmlist_head(sl)->cur_idx_unsafe;
    while (iter != 0) {
        shmlist_ele_t *item = shmvector_at(sl->v, iter);
        void* idata = shmlist_ele_get_data(item);
        if (0 == elecmp(cmpvalue, idata)) {
            /* Splice out iter */
            shmlist_set_next_idx(sl, item->prev_idx, item->next_idx);
            shmlist_set_prev_idx(sl, item->next_idx, item->prev_idx);

            /* Make a local copy of iter data */
            *match = shmlist_malloc_copy_data(sl, item);
        
            /* Mark the phead memory as available for reuse */
            shmvector_del(sl->v, iter);
            rc = 0;
            break;
        }
        iter = shmlist_get_next_idx(sl, iter);
    }
    shmmutex_unlock(&(sl->v->shm->lock));
    return rc;
}

/** return an array of matches up to the supplied max */
int shmlist_extract_n_matches_safe(shmlist_t *sl, size_t match_max, void *cmpvalue, shmlist_elecmp_fn elecmp, 
								   size_t* elecnt, void **ele) {
    int rc = 1;

    /* Create an array to hold matching indexes */
    size_t *idx_matches = calloc(match_max, sizeof(size_t));
    size_t match_cnt = 0;

    shmmutex_lock(&(sl->v->shm->lock));
    size_t iter = shmlist_head(sl)->cur_idx_unsafe;
    while (iter != 0 && match_cnt < match_max) {
        shmlist_ele_t *item = shmvector_at(sl->v, iter);
        void* idata = shmlist_ele_get_data(item);
        if (0 == elecmp(cmpvalue, idata)) {
            /* Save this matching index for later extraction */
            idx_matches[match_cnt] = iter;
            match_cnt++;
        }
        iter = shmlist_get_next_idx(sl, iter);
    }

    /* Extract the matches if any exist*/
    *elecnt = match_cnt;
    if (*elecnt > 0) {
        *ele = calloc(*elecnt, sl->v->shm->esize - sizeof(shmlist_ele_t));
        for (int i = 0; i < *elecnt; i++) {
            shmlist_ele_t *item = shmvector_at(sl->v, idx_matches[i]);
            shmlist_copy_data(sl, (*ele) + i, item);

            /* Splice out the matched item */
            shmlist_set_next_idx(sl, item->prev_idx, item->next_idx);
            shmlist_set_prev_idx(sl, item->next_idx, item->prev_idx);

            /* Mark the shared item as available for reuse */
            shmvector_del(sl->v, item->idx);
        }
        rc = 0;
    }
    shmmutex_unlock(&(sl->v->shm->lock));

    /* Free local resources */
    free(idx_matches);

    return rc;

}

/** return the length of the list  */
int shmlist_length(shmlist_t *sl) {
    /* Get the number of live elements in the vector minus the empty list head */
    return sl->v->shm->active_count - 1;
}

/** return a pointer to the list head element */
shmlist_t* shmlist_head(shmlist_t *sl) {
    size_t hidx = shmlist_get_next_idx(sl, 0);
    sl->cur_idx_unsafe = hidx;
    return sl;
}

/** return a pointer to the list tail element */
shmlist_t* shmlist_tail(shmlist_t *sl) {
    size_t tidx = shmlist_get_prev_idx(sl, 0);
    sl->cur_idx_unsafe = tidx;
    return sl;
}

/* return a pointer to the next element */
shmlist_t* shmlist_next(shmlist_t *sl) {
    size_t nidx = shmlist_get_next_idx(sl, sl->cur_idx_unsafe);
    sl->cur_idx_unsafe = nidx;
    return sl;
}

/* return a pointer to the previous element */
shmlist_t* shmlist_prev(shmlist_t *sl) {
    size_t pidx = shmlist_get_prev_idx(sl, sl->cur_idx_unsafe);
    sl->cur_idx_unsafe = pidx;
    return sl;
}

/* return a pointer to the list inside the data */
void* shmlist_get_data(shmlist_t *sl) {
    void *v_ele = shmvector_at(sl->v, sl->cur_idx_unsafe);
    void *ele_data = shmlist_ele_get_data(v_ele);
    return ele_data;
}
/* Insert a copy of ele after this list ptr */
int shmlist_insert_after_safe(shmlist_t *sl, void* ele) {
    return -1;
}

/* Insert a copy of ele before this list ptr */
int shmlist_insert_before_safe(shmlist_t *sl, void* ele) {
    return -1;
}
