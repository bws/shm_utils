#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shm_mutex.h"
#include "shm_vector.h"
#include "shm_counter.h"

/** @return 0 if uids are equal, 1 if unequal */
static int shmcounter_uidcmp(void* lhs, void* rhs) {
	shmcounter_uid_t* l = (shmcounter_uid_t*)(lhs);
	shmcounter_data_t* r = (shmcounter_data_t*)(rhs);
	if ( (l->group == r->id.group) && 
		 (l->ctype == r->id.ctype) && 
		 (l->tag == r->id.tag) &&
		 (l->lid == r->id.lid)) {
		return 0;
	}
	else {
		return 1;
	}
}

/** Create and allocate a new shared counter set. Counters initialized to 0 once. */
int shmcounter_set_create(shmcounter_set_t *scs, const char* counterset) {

	int rc = 0;

	/* Create the vector's local storage */
    shmvector_t *v = malloc(sizeof(shmvector_t));
	memset(v, 0, sizeof(shmvector_t));

    /* Setup the vector's shared storage */
    rc = shmvector_create(v, counterset, sizeof(shmcounter_data_t), SHMCOUNTER_SET_SIZE);
	if (0 != rc) {
		fprintf(stderr, "ERROR: Failed creating shared storage for counter\n");
		free(v);
		return rc;
	}
	/* Local initialization */
    scs->v = v;
	return rc;
}

/** Release resources associated with this shared memory list */
int shmcounter_set_destroy(shmcounter_set_t *scs) {
	/* If the vector is completely empty, delete it */
	shmvector_destroy_safe(scs->v);
	/* Release local resources */
	free(scs->v);
	scs->v = 0;
	return 0;
}

/** Create and allocate a new shared counter. Initialize it to 0. */
int shmcounter_create(shmcounter_t *sc, shmcounter_set_t *scs, shmcounter_uid_t cid) {

	int rc = 0;

	/* Rely on vector's lock to prevent deletion of the 
	   counter while we are performing arrival for the shared counter*/
	shmmutex_lock(&(scs->v->shm->lock));
	/* Search the vector for the supplied id, create it if required */
	int sz = shmvector_size(scs->v);
	int idx = shmvector_find_first_of(scs->v, &cid, &shmcounter_uidcmp);
	if (idx == sz) {
		shmcounter_data_t d = {.mutex = 0, .id = cid, .count = 0, .refcount = 0};
		size_t newidx = shmvector_insert_quick(scs->v);
		if (newidx < 0) {
			fprintf(stderr, "ERROR: Could not insert new counter\n");
			rc = 1;
		}
		newidx = shmvector_insert_at(scs->v, newidx, &d);
		idx = newidx;
	}

	/* Initialize the counter mutex and increment refcount */
	if (0 == rc) {
		shmcounter_data_t *cd = shmvector_at(scs->v, idx);
		cd->refcount++;
		shmmutex_create(&(cd->mutex));
	}

	/* Unlock the vector */
	shmmutex_unlock(&(scs->v->shm->lock));

	/* Local initialization */
	if (0 == rc) {
		sc->idx = idx;
		sc->set = scs;

	}
	return rc;
}

/** Release resources associated with this shared memory list */
int shmcounter_destroy(shmcounter_t *sc) {
	/* Vector critical section if we need to perform deletion */
	shmmutex_lock(&(sc->set->v->shm->lock));

	/* If this is the last reference, delete the counter */
	shmcounter_data_t *cd = shmvector_at(sc->set->v, sc->idx);
	shmmutex_lock(&(cd->mutex));
	if (1 == cd->refcount) {
		/* No need to unlock, just set the data to 0 and delete idx */
		memset(cd, 0, sizeof(shmcounter_data_t));
		shmvector_del(sc->set->v, sc->idx);
	} else {
		cd->refcount--;
		shmmutex_unlock(&(cd->mutex));
	}
	shmmutex_unlock(&(sc->set->v->shm->lock));
	return 0;
}

/** Increment the counter */
void shmcounter_inc_safe(shmcounter_t* sc, int val) {
	/* Lock the vector to prevent deletion */
	shmmutex_lock(&(sc->set->v->shm->lock));

	/* Lock the counter */
	shmcounter_data_t *d = shmvector_at(sc->set->v, sc->idx);
	shmmutex_lock(&(d->mutex));
	d->count += val;
	shmmutex_unlock(&(d->mutex));

	/* End vector critical section */
	shmmutex_unlock(&(sc->set->v->shm->lock));
}

/** Decrement the counter */
void shmcounter_dec_safe(shmcounter_t* sc, int val) {
	/* Lock the vector to prevent deletion */
	shmmutex_lock(&(sc->set->v->shm->lock));

	/* Lock the counter */
	shmcounter_data_t *d = shmvector_at(sc->set->v, sc->idx);
	shmmutex_lock(&(d->mutex));
	d->count -= val;
	shmmutex_unlock(&(d->mutex));

	/* End vector critical section */
	shmmutex_unlock(&(sc->set->v->shm->lock));
}

/** Set the counter to value if the counter is 0. Return true if the value was updated. */
bool shmcounter_set_if_zero_safe(shmcounter_t* sc, int val) {
	bool value_set = false;
	/* Lock the vector to prevent deletion */
	shmmutex_lock(&(sc->set->v->shm->lock));

	/* Lock the counter */
	shmcounter_data_t *d = shmvector_at(sc->set->v, sc->idx);
	shmmutex_lock(&(d->mutex));
	if (0 == d->count) {
		d->count = val;
		value_set = true;
	}
	shmmutex_unlock(&(d->mutex));

	/* End vector critical section */
	shmmutex_unlock(&(sc->set->v->shm->lock));
	return value_set;
}

/** return the value of the counter */
int shmcounter_value(shmcounter_t* sc) {
	shmcounter_data_t *d = shmvector_at(sc->set->v, sc->idx);
	return (d->count);
}

/** Compare the value of the counter */
bool shmcounter_isvalue(shmcounter_t* sc, int val) {
	shmcounter_data_t *d = shmvector_at(sc->set->v, sc->idx);
	return (val == d->count);
}

/** Compare the value of the counter */
bool shmcounter_isequal_safe(shmcounter_t* lhs, shmcounter_t* rhs) {
	int lcount, rcount;
	shmcounter_data_t *l, *r;

	/* Short circuit on self comparison (which isn't lock safe) */
	if (lhs->idx == rhs->idx)
		return true;

	/* Lock the vector to prevent deletion */
	shmmutex_lock(&(lhs->set->v->shm->lock));

	/* Lock the lhs counter */
	l = shmvector_at(lhs->set->v, lhs->idx);
	shmmutex_lock(&(l->mutex));
	lcount = l->count;

	/* Lock the rhs counter */
	r = shmvector_at(rhs->set->v, rhs->idx);
	shmmutex_lock(&(r->mutex));
	rcount = r->count;

	/* Unlock the counters */
	shmmutex_unlock(&(r->mutex));
	shmmutex_unlock(&(l->mutex));

	/* End vector critical section */
	shmmutex_unlock(&(lhs->set->v->shm->lock));

	return (lcount == rcount);
}
