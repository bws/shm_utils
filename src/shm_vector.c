
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "shm_vector.h"

/** Allocate space in shared memory for an array of size N */
int shmvector_create(shmvector_t *sv, const char* segname, size_t elesz, size_t sz) {
	int rc = 0;
    assert(segname != 0);
    assert(elesz != 0);
    memset(sv, 0, sizeof(shmvector_t));
    sv->segname = segname;
    /* Open exclusive so segment is intialized once */
    sv->segd = shm_open(segname, O_RDWR|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
    if (-1 == sv->segd) {
        /* Open exclusive failed, so segment is initialized elsewhere */
        int rbytes = 0;
        size_t capacity = 0, elesize = 0, segsize = 0;

        /* Open the file and wait for the shmmutex to exist */
        sv->segd = shm_open(segname, O_RDWR, S_IRUSR|S_IWUSR);
        while (rbytes < sizeof(shmmutex_t)) {
            shmmutex_t tmp;
            rbytes = pread(sv->segd, &tmp, sizeof(shmmutex_t), offsetof(shmarray_t, lock));
        }

        /* MMap and lock the shared mutex */
        sv->shm = mmap(0, sizeof(shmmutex_t), PROT_READ|PROT_WRITE, MAP_SHARED, sv->segd, 0);
        if (sv->shm == MAP_FAILED) {
            fprintf(stderr, "ERROR: MMap to acquire lock failed\n");
            close(sv->segd);
            rc = 1;
        }

        /* Critical section - lock/unlock to ensure mutex creation is complete */
        shmmutex_lock(&(sv->shm->lock));
        shmmutex_unlock(&(sv->shm->lock));
        munmap(sv->shm, sizeof(shmmutex_t));

        /* Setup local pointers into shared memory space */
        rbytes = pread(sv->segd, &capacity, sizeof(size_t), offsetof(shmarray_t, capacity));
        rbytes = pread(sv->segd, &elesize, sizeof(size_t), offsetof(shmarray_t, esize));
        segsize = sizeof(shmarray_t) + (capacity * elesize) + (capacity * sizeof(bool));
        sv->shm = mmap(0, segsize, PROT_READ|PROT_WRITE, MAP_SHARED, sv->segd, 0);    
        if (sv->shm == MAP_FAILED) {
            fprintf(stderr, "ERROR: MMap to acquire shared array failed\n");
            close(sv->segd);
            rc = 1;
        }
    }
    else {
        /* Initialize the shared memory segement */
        size_t segsize = sizeof(shmarray_t) + (sz * elesz) + (sz * sizeof(bool));
        ftruncate(sv->segd, segsize);
        sv->shm = mmap(0, segsize, PROT_READ|PROT_WRITE, MAP_SHARED, sv->segd, 0);
        if (sv->shm != MAP_FAILED) {
            /* Initialize everything but the mutex */
            sv->shm->capacity = sz;
            sv->shm->esize = elesz;
            sv->shm->active_count = 0;
            sv->shm->next_back_idx = 0;
            sv->shm->eles = (void*)(sv->shm + 1);
            sv->shm->actives = (void*)sv->shm->eles + ((sz + 1) * elesz);

            /* Create the mutex as the last step to unblock other processes */
            shmmutex_create(&sv->shm->lock);
        } else {
            close(sv->segd);
            rc = 1;
        }
    }
	return rc;
}

int shmvector_destroy(shmvector_t *sv) {
    /* Disable the lock */
    shmmutex_destroy(&sv->shm->lock);

    /* Free resources */
    size_t segsize = sizeof(shmarray_t) + (sv->shm->capacity * sv->shm->esize) + 
        (sv->shm->capacity * sizeof(bool));
    shm_unlink(sv->segname);
    close(sv->segd);
    munmap(sv->shm, segsize);
}

/** Add an element to the array with thread-safety */
int shmvector_safe_push_back(shmvector_t* sv, void* ele) {
    int idx;
    shmmutex_lock(&sv->shm->lock);
    idx = shmvector_push_back(sv, ele);
    shmmutex_unlock(&sv->shm->lock);
	return idx;
}

/** Add an element to the array */
int shmvector_push_back(shmvector_t* sv, void* ele) {
	int idx = -1;
	if (sv->shm->next_back_idx < sv->shm->capacity) {
		void* buf_offset = sv->shm->eles + (sv->shm->esize * sv->shm->next_back_idx);
		buf_offset = memcpy(buf_offset, ele, sv->shm->esize);
        idx = sv->shm->next_back_idx;
		sv->shm->actives[sv->shm->next_back_idx] = true;
		sv->shm->next_back_idx++;
		sv->shm->active_count++;
	}
	return idx;
}

/** Insert an element to the array at position idx */
int shmvector_insert_at(shmvector_t* sv, size_t idx, void* ele) {
    int rc = -1;
	if (idx < sv->shm->capacity) {
		void* buf_offset = sv->shm->eles + (sv->shm->esize * idx);
		buf_offset = memcpy(buf_offset, ele, sv->shm->esize);
        /* Update the active count and last_idx if required */
        if (!sv->shm->actives[idx]) {
		    sv->shm->actives[idx] = true;
		    sv->shm->active_count++;
        }
        if (idx > sv->shm->next_back_idx) {
            sv->shm->next_back_idx = idx + 1;
        }
        rc = idx;
	}
	return rc;
}

/** Return the element at idx with thread-safety*/
void* shmvector_safe_at(shmvector_t* sv, size_t idx) {
	void *val;
    shmmutex_lock(&sv->shm->lock);
    val - shmvector_at(sv, idx);
    shmmutex_unlock(&sv->shm->lock);
	return val;
}

/** Return the element at idx */
void* shmvector_at(shmvector_t* sv, size_t idx) {
	void *val = 0;
	if (sv->shm->next_back_idx > idx && true == sv->shm->actives[idx]) {
        val = sv->shm->eles + (sv->shm->esize * idx);
	}
	return val;
}


/* Perform a push back if possible, otherwise search for an empty slot */
int shmvector_insert_quick(shmvector_t* sv, void* ele) {
    int idx = -1;
    /* If the vector has space find a location to insert this element */
    if (sv->shm->active_count < sv->shm->capacity) {
        idx = shmvector_push_back(sv, ele);
        if (-1 == idx) {
            /* Push_back failed so search for an entry not marked active */
            for (int i = 0; i < sv->shm->capacity; i++) {
                if (!(sv->shm->actives[i])) {
                    int rc = shmvector_insert_at(sv, i, ele);
                    idx = i;
                    break;
                }
            }
        }
    }
    return idx;
}

/* If the element at idx exists, mark it available */
int shmvector_del(shmvector_t* sv, size_t idx) {
    int rc = -1;
    if (sv->shm->actives[idx]) {
        sv->shm->actives[idx] = false;
        sv->shm->active_count --;
        rc = 0;
    }
    return rc;
}

/* Double the size of the shmarray and copy data as needed */
int shmvector_grow_array(shmvector_t *sv) {
	int rc = -1;
    size_t newsz = 1024;
    if (sv->shm->capacity > 0) {
	    newsz = sv->shm->capacity * 2;
    }
    /* Not yet implemented */
	return rc;
}
