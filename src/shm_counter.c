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

	/* Search the vector for the supplied id, create it if required */
	shmcounter_data_t d = {.id = cid, .count = 0};
	shmmutex_lock(&(sc->v->shm->lock));
	int sz = shmvector_size(sc->v);
	fprintf(stderr, "Vector size is %d\n", sz);
	int idx = shmvector_find_first_of(sc->v, &d, shmcounter_uidcmp);
	fprintf(stderr, "Found matching counter at idx: %d\n", idx);
	sc->idx = idx;
	if (idx == sz) {
		int idx = shmvector_insert_at(sc->v, sc->idx, &d);
		if (idx != sc->idx) {
			fprintf(stderr, "ERROR: Could not insert new counter\n");
			rc = 1;
		}
	}
	shmmutex_unlock(&(sc->v->shm->lock));
	return rc;
}

/** Release resources associated with this shared memory list */
int shmcounter_destroy(shmcounter_t *sc) {
	shmvector_destroy(sc->v);
	free(sc->v);
	sc->v = 0;
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