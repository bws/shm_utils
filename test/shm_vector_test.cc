
#include <gtest/gtest.h>
#include "shm_vector.h"

/* Test empty creation */
TEST(shmvector, empty_creation) {
	int rc;
	shmvector_t sv;
	rc = shmvector_create(&sv, "/shmvector-empty_allocation", 1, 0);
	EXPECT_EQ(0, rc);
	EXPECT_NE((char*)NULL, sv.segname);
	EXPECT_NE(-1, sv.segd);
	EXPECT_NE((shmarray_t*)NULL, sv.shm);
	EXPECT_EQ(SHMMUTEX_LOCK_AVAILABLE, sv.shm->lock.val);
	EXPECT_EQ(0, sv.shm->capacity);
	EXPECT_EQ(1, sv.shm->esize);
	EXPECT_EQ(0, sv.shm->active_count);
	EXPECT_NE((void*)NULL, sv.shm->eles);
	EXPECT_NE((bool*)NULL, sv.shm->actives);
	shmvector_destroy(&sv);
}

/* Test basic destruction */
TEST(shmvector, basic_destroy) {
	int rc;
	shmvector_t sv;
	rc = shmvector_create(&sv, "/shmvector-basic_destroy", 1, 0);
	EXPECT_EQ(0, rc);
	EXPECT_NE((char*)NULL, sv.segname);
	EXPECT_NE(-1, sv.segd);
	EXPECT_NE((shmarray_t*)NULL, sv.shm);
	EXPECT_EQ(SHMMUTEX_LOCK_AVAILABLE, sv.shm->lock.val);
	EXPECT_EQ(0, sv.shm->capacity);
	EXPECT_EQ(1, sv.shm->esize);
	EXPECT_EQ(0, sv.shm->active_count);
	EXPECT_NE((void*)NULL, sv.shm->eles);
	EXPECT_NE((bool*)NULL, sv.shm->actives);
	shmvector_destroy(&sv);
}

/* Test normal allocation */
TEST(shmvector, basic_creation) {
	int rc;
	shmvector_t sv;
	rc = shmvector_create(&sv, "/shmvector-basic_creation", 24, 1024);
	EXPECT_EQ(0, rc);
	EXPECT_NE((char*)NULL, sv.segname);
	EXPECT_NE(-1, sv.segd);
	EXPECT_NE((shmarray_t*)NULL, sv.shm);
	EXPECT_EQ(1024, sv.shm->capacity);
	EXPECT_EQ(24, sv.shm->esize);
	EXPECT_EQ(0, sv.shm->active_count);
	EXPECT_NE((void*)NULL, sv.shm->eles);
	EXPECT_NE((bool*)NULL, sv.shm->actives);

	for (int i = 0; i < 1024; i++) {
		EXPECT_EQ(0, *( ((char*)sv.shm->eles) + i*24));
		EXPECT_EQ(false, sv.shm->actives[i]);
	}
	shmvector_destroy(&sv);
}

/* Allocate the same vector twice to confirm values are shared */
TEST(shmvector, shared_creation) {

	int rc1, rc2;
	shmvector_t sv1, sv2;
	rc1 = shmvector_create(&sv1, "/shmvector-shared_allocation", 24, 1024);
	EXPECT_EQ(0, rc1);
	EXPECT_NE((char*)NULL, sv1.segname);
	EXPECT_NE(-1, sv1.segd);
	EXPECT_NE((shmarray_t*)NULL, sv1.shm);
	EXPECT_EQ(1024, sv1.shm->capacity);
	EXPECT_EQ(24, sv1.shm->esize);
	EXPECT_EQ(0, sv1.shm->active_count);
	EXPECT_NE((void*)NULL, sv1.shm->eles);
	EXPECT_NE((bool*)NULL, sv1.shm->actives);

	/* Confirm the second shared vector has the same contents as the first */
	rc2 = shmvector_create(&sv2, "/shmvector-shared_allocation", 24, 1024);
	EXPECT_EQ(0, rc2);
	EXPECT_STREQ(sv1.segname, sv2.segname);
	EXPECT_NE(-1, sv2.segd);
	EXPECT_EQ(1, sv2.shm->lock.val);
	EXPECT_EQ(sv1.shm->capacity, sv2.shm->capacity);
	EXPECT_EQ(sv1.shm->esize, sv2.shm->esize);
	EXPECT_EQ(sv1.shm->active_count, sv2.shm->active_count);
	EXPECT_EQ(sv1.shm->eles, sv1.shm->eles);
	EXPECT_EQ(sv1.shm->actives, sv2.shm->actives);
	
	/* Destroy can be safely called multiple times */
	shmvector_destroy(&sv1);
	shmvector_destroy(&sv2);
}

/* Allocate the same vector twice to confirm push_back values are shared */
TEST(shmvector, simple_safe_push_back) {

	int rc1;
	double val = 1234.1234;
	shmvector_t sv1;
	rc1 = shmvector_create(&sv1, "/shmvector-simple_push_back", sizeof(double), 3);
	EXPECT_EQ(0, rc1);
	EXPECT_NE((char*)NULL, sv1.segname);
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(sizeof(double), sv1.shm->esize);
	EXPECT_EQ(0, sv1.shm->active_count);

	rc1 = shmvector_safe_push_back(&sv1, &val);
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(1, sv1.shm->active_count);
	EXPECT_EQ(val, ((double*)sv1.shm->eles)[0]);
	EXPECT_EQ(true, sv1.shm->actives[0]);
	EXPECT_EQ(false, sv1.shm->actives[1]);
	EXPECT_EQ(false, sv1.shm->actives[2]);

	val = 777.77;
	rc1 = shmvector_safe_push_back(&sv1, &val);
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(2, sv1.shm->active_count);
	EXPECT_EQ(val, ((double*)sv1.shm->eles)[1]);
	EXPECT_EQ(true, sv1.shm->actives[0]);
	EXPECT_EQ(true, sv1.shm->actives[1]);
	EXPECT_EQ(false, sv1.shm->actives[2]);

	val = 1.0;
	rc1 = shmvector_safe_push_back(&sv1, &val);
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(3, sv1.shm->active_count);
	EXPECT_EQ(val, ((double*)sv1.shm->eles)[2]);
	EXPECT_EQ(true, sv1.shm->actives[0]);
	EXPECT_EQ(true, sv1.shm->actives[1]);
	EXPECT_EQ(true, sv1.shm->actives[2]);

	shmvector_destroy(&sv1);

}

/* Allocate the same vector 3x to confirm push_back values are shared */
TEST(shmvector, shared_safe_push_back) {

	int rc1, rc2, rc3;
	shmvector_t sv1, sv2, sv3;

	/* Create a shared vector and add an element to it */
	double val1 = 1234.1234;
	rc1 = shmvector_create(&sv1, "/shmvector-shared_push_back", sizeof(double), 3);
	EXPECT_EQ(0, rc1);
	EXPECT_NE((char*)NULL, sv1.segname);
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(sizeof(double), sv1.shm->esize);
	EXPECT_EQ(0, sv1.shm->active_count);

	rc1 = shmvector_safe_push_back(&sv1, &val1);
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(1, sv1.shm->active_count);
	EXPECT_EQ(val1, ((double*)sv1.shm->eles)[0]);
	EXPECT_EQ(0.0, ((double*)sv1.shm->eles)[1]);
	EXPECT_EQ(0.0, ((double*)sv1.shm->eles)[2]);
	EXPECT_EQ(true, sv1.shm->actives[0]);
	EXPECT_EQ(false, sv1.shm->actives[1]);
	EXPECT_EQ(false, sv1.shm->actives[2]);

	/* Create a second shared vector and push an element onto that */
	double val2 = 777.77;
	rc2 = shmvector_create(&sv2, "/shmvector-shared_push_back", sizeof(double), 3);
	EXPECT_EQ(0, rc2);
	EXPECT_NE((char*)NULL, sv2.segname);
	EXPECT_EQ(3, sv2.shm->capacity);
	EXPECT_EQ(sizeof(double), sv2.shm->esize);
	EXPECT_EQ(1, sv2.shm->active_count);

	rc2 = shmvector_safe_push_back(&sv2, &val2);
	EXPECT_EQ(3, sv2.shm->capacity);
	EXPECT_EQ(2, sv2.shm->active_count);
	EXPECT_EQ(val1, ((double*)sv2.shm->eles)[0]);
	EXPECT_EQ(val2, ((double*)sv2.shm->eles)[1]);
	EXPECT_EQ(0.0, ((double*)sv2.shm->eles)[2]);
	EXPECT_EQ(true, sv2.shm->actives[0]);
	EXPECT_EQ(true, sv2.shm->actives[1]);
	EXPECT_EQ(false, sv2.shm->actives[2]);

	/* Create a 3rd vector and verify early values plus a new value */
	double val3 = 1.0;
	rc3 = shmvector_create(&sv3, "/shmvector-shared_push_back", sizeof(double), 3);
	EXPECT_EQ(0, rc3);
	EXPECT_NE((char*)NULL, sv3.segname);
	EXPECT_EQ(3, sv3.shm->capacity);
	EXPECT_EQ(sizeof(double), sv3.shm->esize);
	EXPECT_EQ(2, sv3.shm->active_count);

	rc1 = shmvector_safe_push_back(&sv3, &val3);
	EXPECT_EQ(3, sv3.shm->capacity);
	EXPECT_EQ(3, sv3.shm->active_count);
	EXPECT_EQ(val1, ((double*)sv3.shm->eles)[0]);
	EXPECT_EQ(val2, ((double*)sv3.shm->eles)[1]);
	EXPECT_EQ(val3, ((double*)sv3.shm->eles)[2]);
	EXPECT_EQ(true, sv3.shm->actives[0]);
	EXPECT_EQ(true, sv3.shm->actives[1]);
	EXPECT_EQ(true, sv3.shm->actives[2]);

	/* Confirm the first 2 instances have the same data as the third */
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(3, sv1.shm->active_count);
	EXPECT_EQ(val1, ((double*)sv1.shm->eles)[0]);
	EXPECT_EQ(val2, ((double*)sv1.shm->eles)[1]);
	EXPECT_EQ(val3, ((double*)sv1.shm->eles)[2]);
	EXPECT_EQ(true, sv1.shm->actives[0]);
	EXPECT_EQ(true, sv1.shm->actives[1]);
	EXPECT_EQ(true, sv1.shm->actives[2]);

	EXPECT_EQ(3, sv2.shm->capacity);
	EXPECT_EQ(3, sv2.shm->active_count);
	EXPECT_EQ(val1, ((double*)sv2.shm->eles)[0]);
	EXPECT_EQ(val2, ((double*)sv2.shm->eles)[1]);
	EXPECT_EQ(val3, ((double*)sv2.shm->eles)[2]);
	EXPECT_EQ(true, sv2.shm->actives[0]);
	EXPECT_EQ(true, sv2.shm->actives[1]);
	EXPECT_EQ(true, sv2.shm->actives[2]);

	/* Destroy all 3 shared instances */
	shmvector_destroy(&sv1);
	shmvector_destroy(&sv2);
	shmvector_destroy(&sv3);
}

/* Test insertion past the last inserted element */
TEST(shmvector, shared_insert_at_past) {
	int rc1;
	char ele;
	shmvector_t sv;
	shmvector_create(&sv, "/shmvector-shared_insert_at_past", sizeof(char), 10);

	ele = 'a';
	rc1 = shmvector_push_back(&sv, &ele);
	EXPECT_EQ(0, rc1);
	EXPECT_EQ('a', *((char*)shmvector_at(&sv, 0)));

	ele = 'b';
	rc1 = shmvector_insert_at(&sv, 9, &ele);
	EXPECT_EQ(9, rc1);
	EXPECT_EQ('b', *((char*)shmvector_at(&sv, 9)));

	ele = 'c';
	rc1 = shmvector_push_back(&sv, &ele);
	EXPECT_EQ(-1, rc1);
	
	shmvector_destroy(&sv);
}

/* Test insertion of an element that already exists */
TEST(shmvector, shared_insert_at_over) {
	int rc1;
	char ele;
	shmvector_t sv;
	shmvector_create(&sv, "/shmvector-shared_insert_at_past", sizeof(char), 10);

	ele = 'a';
	rc1 = shmvector_push_back(&sv, &ele);
	EXPECT_EQ(0, rc1);
	EXPECT_EQ('a', *((char*)shmvector_at(&sv, 0)));

	ele = 'b';
	rc1 = shmvector_insert_at(&sv, 0, &ele);
	EXPECT_EQ(0, rc1);
	EXPECT_EQ('b', *((char*)shmvector_at(&sv, 0)));

	ele = 'c';
	rc1 = shmvector_push_back(&sv, &ele);
	EXPECT_EQ(1, rc1);
	EXPECT_EQ('c', *((char*)shmvector_at(&sv, 1)));
	
	shmvector_destroy(&sv);
}

/* Allocate a vector and confirm insert_quick will push back */
TEST(shmvector, shared_insert_quick_not_full) {
	int rc1;
	char ele;
	shmvector_t sv;
	shmvector_create(&sv, "/shmvector-shared_insert_quick_not_full", sizeof(char), 10);

	ele = 'a';
	rc1 = shmvector_insert_quick(&sv, &ele);
	EXPECT_EQ(0, rc1);
	EXPECT_EQ('a', *((char*)shmvector_at(&sv, 0)));

	ele = 'b';
	rc1 = shmvector_insert_quick(&sv, &ele);
	EXPECT_EQ(1, rc1);
	EXPECT_EQ('b', *((char*)shmvector_at(&sv, 1)));

	shmvector_destroy(&sv);
}

/* Allocate a vector and confirm insert_quick will find an empty location */
TEST(shmvector, shared_insert_quick_is_full) {
	int rc1;
	char ele;
	shmvector_t sv;
	shmvector_create(&sv, "/shmvector-shared_insert_quick_is_full", sizeof(char), 4);

	// Fill the entire vector
	ele = 'a';
	rc1 = shmvector_push_back(&sv, &(ele));
	ele = 'b';
	rc1 = shmvector_push_back(&sv, &(ele));
	ele = 'c';
	rc1 = shmvector_push_back(&sv, &(ele));
	ele = 'd';
	rc1 = shmvector_push_back(&sv, &(ele));
	EXPECT_EQ('a', *((char*)shmvector_at(&sv, 0)));
	EXPECT_EQ('b', *((char*)shmvector_at(&sv, 1)));
	EXPECT_EQ('c', *((char*)shmvector_at(&sv, 2)));
	EXPECT_EQ('d', *((char*)shmvector_at(&sv, 3)));

	// Delete an element
	rc1 = shmvector_del(&sv, 2);
	EXPECT_EQ(0, rc1);

	// Perform a insert_quick into the available hole
	ele = 'z';
	rc1 = shmvector_insert_quick(&sv, &ele);
	EXPECT_EQ(2, rc1);

	shmvector_destroy(&sv);
}


