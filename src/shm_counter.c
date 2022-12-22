#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "shm_mutex.h"
#include "shm_vector.h"
#include "shm_counter.h"

/** @return 0 if uids are equal, 1 if unequal */
static int shmcounter_uidcmp(void* lhs, void* rhs) {
	shmcounter_data_t* l = (shmcounter_data_t*)(lhs);
	shmcounter_data_t* r = (shmcounter_data_t*)(rhs);
	//fprintf(stderr, "UID CMP LHS: %d %d %d RHS: %d %d %d\n",
	//		l->id.group, l->id.ctype, l->id.tag,
	//		r->id.group, r->id.ctype, r->id.tag);
	if ( (l->id.group == r->id.group) && 
		 (l->id.ctype == r->id.ctype) && 
		 (l->id.tag == r->id.tag))
		return 0;
	else
		return 1;
}

/** Create and allocate a new shared counter. Initialize it to 0. */
int shmcounter_create(shmcounter_t *sc, const char* counterset, shmcounter_uid_t cid) {

	int rc = 0;

	/* Create the vector's local storage */
    shmvector_t *v = malloc(sizeof(shmvector_t));
    sc->v = v;

    /* Setup the vector's shared storage */
    rc = shmvector_create(sc->v, counterset, sizeof(shmcounter_data_t), SHMCOUNTER_SET_SIZE);
	if (0 != rc) {
		fprintf(stderr, "ERROR: Failed creating shared storage for counter\n");
		free(v);
		return rc;
	}

	/* Rely on the fact that the vector's lock prevents deletion of the 
	   counter while we are performing arrival for the shared counter*/
	shmmutex_lock(&(sc->v->shm->lock));
	/* Search the vector for the supplied id, create it if required */
	int sz = shmvector_size(sc->v);
	fprintf(stderr, "Vector size is %d\n", sz);
	int idx = shmvector_find_first_of(sc->v, &d, shmcounter_uidcmp);
	fprintf(stderr, "Found matching counter at idx: %d\n", idx);
	sc->idx = idx;
	if (idx == sz) {
		shmcounter_data_t d = {0};
		int idx = shmvector_insert_at(sc->v, sc->idx, &d);
		if (idx != sc->idx) {
			fprintf(stderr, "ERROR: Could not insert new counter\n");
			rc = 1;
		}
	}

	/* Grab the counter's lock and increment the refcount */
	shmcounter_data_t *counter = shmvector_at(sc->v, sc->idx);
	shmmutex_lock(&(counter->lock));
	counter->refcount += 1;
	shmmutex_unlock(&(counter->lock));

	/* Unlock the vector */
	shmmutex_unlock(&(sc->v->shm->lock));
	return rc;
}

/** Release resources associated with this shared memory list */
int shmcounter_destroy(shmcounter_t *sc) {
	/* First take the lock on the vector in case deletion is required */
	shmmutex_lock(&(sc->v->shm->lock));

	/* Decrement the reference count on the counter, and delete
	   it from the vector if the reference count is 0 */
    shmcounter_data_t *scd = shmvector_at(sc->v, sc->idx);
	shmmutex_lock(&(scd->mutex));
	scd->refcount -= 1;
	if (0 == scd->refcount) {
		shmvector_del(sc->v, sc->idx);
	}
	/* Strictly speaking the item is deleted, but this unlock is still safe */
	shmmutex_unlock(&(scd->mutex));

	/* Now check if the vector is completely empty, if so delete it too */
	if (0 == shmvector_size(sc->v)) {
		/* Do an unsafe destroy on the mutex */
		sc->v->shm->lock = SHMMUTEX_LOCK_NOTREADY;
		
		// FIXME, how does this work with the lock?
		shmvector_destroy(sc->v);
		/* Release local resources */
		free(sc->v);
		sc->v = 0;
	} else {
		/* Release local resources */
		shmmutex_unlock(&(sc->v->shm->lock));
		free(sc->v);
		sc->v = 0;
	}
	return 0;
}

/** Increment the counter */
void shmcounter_inc_safe(shmcounter_t* sc, int val) {
	shmcounter_data_t *d = shmvector_at(sc->v, sc->idx);
	shmmutex_lock(&(sc->v->shm->lock));
	d->count += val;
	shmmutex_unlock(&(sc->v->shm->lock));
}

/** Decrement the counter */
void shmcounter_dec_safe(shmcounter_t* sc, int val) {
	shmcounter_data_t *d = shmvector_at(sc->v, sc->idx);
	shmmutex_lock(&(sc->v->shm->lock));
	d->count -= val;
	shmmutex_unlock(&(sc->v->shm->lock));
}

/** Compare the value of the counter */
bool shmcounter_isequal(shmcounter_t* sc, int val) {
	fprintf(stderr, "Checking value for idx: %d\n", sc->idx);
	shmcounter_data_t *d = shmvector_at(sc->v, sc->idx);
	return (val == d->count);
}