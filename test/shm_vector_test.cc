
#include <gtest/gtest.h>
#include <string>
#include "shm_vector.h"
using namespace std;

static const string shmdir = "/dev/shm";

/* Test empty creation */
TEST(shmvector, create_empty) {
    const char* vecname = "/shmvector_create_empty";
    unlink(string(shmdir + string(vecname)).c_str());

	int rc;
	shmvector_t sv;
	rc = shmvector_create(&sv, vecname, 1, 0);
	EXPECT_EQ(0, rc);
	EXPECT_NE((char*)NULL, sv.segname);
	EXPECT_NE(-1, sv.segd);
	EXPECT_NE((shmarray_t*)NULL, sv.shm);
	EXPECT_EQ(SHMMUTEX_LOCK_AVAILABLE, sv.shm->lock.val);
	EXPECT_EQ(0, sv.shm->capacity);
	EXPECT_EQ(1, sv.shm->esize);
	EXPECT_EQ(0, sv.shm->active_count);
	EXPECT_EQ(sizeof(shmarray_t), sv.shm->eles_offset);
	EXPECT_EQ(sizeof(shmarray_t), sv.shm->actives_offset);
	shmvector_destroy(&sv);
}

/* Test basic destruction */
TEST(shmvector, basic_destroy) {
    const char* vecname = "/shmvector_destroy_basic";
    unlink(string(shmdir + string(vecname)).c_str());

	int rc;
	shmvector_t sv;
	rc = shmvector_create(&sv, vecname, 1, 0);
	EXPECT_EQ(0, rc);
	EXPECT_NE((char*)NULL, sv.segname);
	EXPECT_NE(-1, sv.segd);
	EXPECT_NE((shmarray_t*)NULL, sv.shm);
	EXPECT_EQ(SHMMUTEX_LOCK_AVAILABLE, sv.shm->lock.val);
	EXPECT_EQ(0, sv.shm->capacity);
	EXPECT_EQ(1, sv.shm->esize);
	EXPECT_EQ(0, sv.shm->active_count);
	EXPECT_EQ(sizeof(shmarray_t), sv.shm->eles_offset);
	EXPECT_EQ(sizeof(shmarray_t), sv.shm->actives_offset);
	shmvector_destroy(&sv);
}

/* Test normal allocation */
TEST(shmvector, create_basic) {
    const char* vecname = "/shmvector_create_basic";
    unlink(string(shmdir + string(vecname)).c_str());

	int rc;
	shmvector_t sv;
	rc = shmvector_create(&sv, vecname, 24, 1024);
	EXPECT_EQ(0, rc);
	EXPECT_NE((char*)NULL, sv.segname);
	EXPECT_NE(-1, sv.segd);
	EXPECT_NE((shmarray_t*)NULL, sv.shm);
	EXPECT_EQ(1024, sv.shm->capacity);
	EXPECT_EQ(24, sv.shm->esize);
	EXPECT_EQ(0, sv.shm->active_count);
	EXPECT_EQ(sizeof(shmarray_t), sv.shm->eles_offset);
	EXPECT_EQ(sizeof(shmarray_t) + 24*1024, sv.shm->actives_offset);

	for (int i = 0; i < 1024; i++) {
		EXPECT_EQ(0, *( ((char*)sv.shm + sv.shm->eles_offset) + i*24));
		EXPECT_EQ(false, ((bool*)((char*)sv.shm + sv.shm->actives_offset))[i]);
	}
	shmvector_destroy(&sv);
}

/* Allocate the same vector twice to confirm values are shared */
TEST(shmvector, create_shared) {
    const char* vecname = "/shmvector_create_shared";
    unlink(string(shmdir + string(vecname)).c_str());

	int rc1, rc2;
	shmvector_t sv1, sv2;
	rc1 = shmvector_create(&sv1, vecname, 24, 1024);
	EXPECT_EQ(0, rc1);
	EXPECT_NE((char*)NULL, sv1.segname);
	EXPECT_NE(-1, sv1.segd);
	EXPECT_NE((shmarray_t*)NULL, sv1.shm);
	EXPECT_EQ(1024, sv1.shm->capacity);
	EXPECT_EQ(24, sv1.shm->esize);
	EXPECT_EQ(0, sv1.shm->active_count);
	EXPECT_EQ(sizeof(shmarray_t), sv1.shm->eles_offset);
	EXPECT_EQ(sizeof(shmarray_t) + 24*1024, sv1.shm->actives_offset);

	/* Confirm the second shared vector has the same contents as the first */
	rc2 = shmvector_create(&sv2, vecname, 24, 1024);
	EXPECT_EQ(0, rc2);
	EXPECT_STREQ(sv1.segname, sv2.segname);
	EXPECT_NE(-1, sv2.segd);
	EXPECT_EQ(1, sv2.shm->lock.val);
	EXPECT_EQ(sv1.shm->capacity, sv2.shm->capacity);
	EXPECT_EQ(sv1.shm->esize, sv2.shm->esize);
	EXPECT_EQ(sv1.shm->active_count, sv2.shm->active_count);
	EXPECT_EQ(sv1.shm->eles_offset, sv2.shm->eles_offset);
	EXPECT_EQ(sv1.shm->actives_offset, sv2.shm->actives_offset);
	
	/* Destroy can be safely called multiple times */
	shmvector_destroy(&sv1);
	shmvector_destroy(&sv2);
}

/* Allocate the same vector twice to confirm push_back values are shared */
TEST(shmvector, push_back_safe_simple) {
    const char* vecname = "/shmvector_push_back_safe_simple";
    unlink(string(shmdir + string(vecname)).c_str());

	int rc1;
	double val = 1234.1234;
	shmvector_t sv1;
	rc1 = shmvector_create(&sv1, vecname, sizeof(double), 3);
	double* eles = (double*)((char*)sv1.shm + sv1.shm->eles_offset);
	bool* actives = (bool*)((char*)sv1.shm + sv1.shm->actives_offset);
	EXPECT_EQ(0, rc1);
	EXPECT_NE((char*)NULL, sv1.segname);
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(sizeof(double), sv1.shm->esize);
	EXPECT_EQ(0, sv1.shm->active_count);

	rc1 = shmvector_safe_push_back(&sv1, &val);
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(1, sv1.shm->active_count);
	EXPECT_EQ(val, eles[0]);
	EXPECT_EQ(true, actives[0]);
	EXPECT_EQ(false, actives[1]);
	EXPECT_EQ(false, actives[2]);

	val = 777.77;
	rc1 = shmvector_safe_push_back(&sv1, &val);
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(2, sv1.shm->active_count);
	EXPECT_EQ(val, eles[1]);
	EXPECT_EQ(true, actives[0]);
	EXPECT_EQ(true, actives[1]);
	EXPECT_EQ(false, actives[2]);

	val = 1.0;
	rc1 = shmvector_safe_push_back(&sv1, &val);
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(3, sv1.shm->active_count);
	EXPECT_EQ(val, eles[2]);
	EXPECT_EQ(true, actives[0]);
	EXPECT_EQ(true, actives[1]);
	EXPECT_EQ(true, actives[2]);

	shmvector_destroy(&sv1);

}

/* Allocate the same vector 3x to confirm push_back values are shared */
TEST(shmvector, push_back_safe_shared) {
    const char* vecname = "/shmvector_push_back_safe_shared";
    unlink(string(shmdir + string(vecname)).c_str());

	int rc1, rc2, rc3;
	shmvector_t sv1, sv2, sv3;

	/* Create a shared vector and add an element to it */
	double val1 = 1234.1234;
	rc1 = shmvector_create(&sv1, vecname, sizeof(double), 3);
	double* eles = (double*)((char*)sv1.shm + sv1.shm->eles_offset);
	bool* actives = (bool*)((char*)sv1.shm + sv1.shm->actives_offset);
	EXPECT_EQ(0, rc1);
	EXPECT_NE((char*)NULL, sv1.segname);
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(sizeof(double), sv1.shm->esize);
	EXPECT_EQ(0, sv1.shm->active_count);

	rc1 = shmvector_safe_push_back(&sv1, &val1);
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(1, sv1.shm->active_count);
	EXPECT_EQ(val1, eles[0]);
	EXPECT_EQ(0.0, eles[1]);
	EXPECT_EQ(0.0, eles[2]);
	EXPECT_EQ(true, actives[0]);
	EXPECT_EQ(false, actives[1]);
	EXPECT_EQ(false, actives[2]);

	/* Create a second shared vector and push an element onto that */
	double val2 = 777.77;
	rc2 = shmvector_create(&sv2, vecname, sizeof(double), 3);
	EXPECT_EQ(0, rc2);
	EXPECT_NE((char*)NULL, sv2.segname);
	EXPECT_EQ(3, sv2.shm->capacity);
	EXPECT_EQ(sizeof(double), sv2.shm->esize);
	EXPECT_EQ(1, sv2.shm->active_count);

	rc2 = shmvector_safe_push_back(&sv2, &val2);
	EXPECT_EQ(3, sv2.shm->capacity);
	EXPECT_EQ(2, sv2.shm->active_count);
	EXPECT_EQ(val1, eles[0]);
	EXPECT_EQ(val2, eles[1]);
	EXPECT_EQ(0.0, eles[2]);
	EXPECT_EQ(true, actives[0]);
	EXPECT_EQ(true, actives[1]);
	EXPECT_EQ(false, actives[2]);

	/* Create a 3rd vector and verify early values plus a new value */
	double val3 = 1.0;
	rc3 = shmvector_create(&sv3, vecname, sizeof(double), 3);
	EXPECT_EQ(0, rc3);
	EXPECT_NE((char*)NULL, sv3.segname);
	EXPECT_EQ(3, sv3.shm->capacity);
	EXPECT_EQ(sizeof(double), sv3.shm->esize);
	EXPECT_EQ(2, sv3.shm->active_count);

	rc1 = shmvector_safe_push_back(&sv3, &val3);
	EXPECT_EQ(3, sv3.shm->capacity);
	EXPECT_EQ(3, sv3.shm->active_count);
	EXPECT_EQ(val1, eles[0]);
	EXPECT_EQ(val2, eles[1]);
	EXPECT_EQ(val3, eles[2]);
	EXPECT_EQ(true, actives[0]);
	EXPECT_EQ(true, actives[1]);
	EXPECT_EQ(true, actives[2]);

	/* Confirm the first 2 instances have the same data as the third */
	EXPECT_EQ(3, sv1.shm->capacity);
	EXPECT_EQ(3, sv1.shm->active_count);
	EXPECT_EQ(val1, eles[0]);
	EXPECT_EQ(val2, eles[1]);
	EXPECT_EQ(val3, eles[2]);
	EXPECT_EQ(true, actives[0]);
	EXPECT_EQ(true, actives[1]);
	EXPECT_EQ(true, actives[2]);

	EXPECT_EQ(3, sv2.shm->capacity);
	EXPECT_EQ(3, sv2.shm->active_count);
	EXPECT_EQ(val1, eles[0]);
	EXPECT_EQ(val2, eles[1]);
	EXPECT_EQ(val3, eles[2]);
	EXPECT_EQ(true, actives[0]);
	EXPECT_EQ(true, actives[1]);
	EXPECT_EQ(true, actives[2]);

	/* Destroy all 3 shared instances */
	shmvector_destroy(&sv1);
	shmvector_destroy(&sv2);
	shmvector_destroy(&sv3);
}

/* Test insertion past the last inserted element */
TEST(shmvector, insert_at_shared_past) {
    const char* vecname = "/shmvector_insert_at_past_shared";
    unlink(string(shmdir + string(vecname)).c_str());

	int rc1;
	char ele;
	shmvector_t sv;
	shmvector_create(&sv, vecname, sizeof(char), 10);

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
TEST(shmvector, insert_at_over_shared) {
    const char* vecname = "/shmvector_insert_at_over_shared";
    unlink(string(shmdir + string(vecname)).c_str());

	int rc1;
	char ele;
	shmvector_t sv;
	shmvector_create(&sv, vecname, sizeof(char), 10);

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
    const char* vecname = "/shmvector_insert_quick_not_full";
    unlink(string(shmdir + string(vecname)).c_str());

	int rc1;
	char ele;
	shmvector_t sv;
	shmvector_create(&sv, vecname, sizeof(char), 10);

	int idx = shmvector_insert_quick(&sv);
	EXPECT_EQ(0, idx);
	EXPECT_EQ(1, shmvector_size(&sv));

	idx = shmvector_insert_quick(&sv);
	EXPECT_EQ(1, idx);
	EXPECT_EQ(2, shmvector_size(&sv));

	shmvector_destroy(&sv);
}

/* Allocate a vector and confirm insert_quick will find an empty location */
TEST(shmvector, shared_insert_quick_is_full) {
    const char* vecname = "/shmvector_insert_quick_is_full_shared";
    unlink(string(shmdir + string(vecname)).c_str());

	int rc1;
	char ele;
	shmvector_t sv;
	shmvector_create(&sv, vecname, sizeof(char), 4);

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
	rc1 = shmvector_insert_quick(&sv);
	EXPECT_EQ(2, rc1);

	shmvector_destroy(&sv);
}

static int test_charcmp(void* l, void* r) {
	char lhs = ((char*)(l))[0];
	char rhs = ((char*)(r))[0];
	fprintf(stderr, "Comparing lhs: %c rhs: %c\n", lhs, rhs);
	if (lhs == rhs)
		return 0;
	else
		return 1;
}

TEST(shmvector, find_first_of_basic) {
    const char* vecname = "/shmvector_find_first_of_basic";
    unlink(string(shmdir + string(vecname)).c_str());

	int rc1;
	shmvector_t sv;
	shmvector_create(&sv, vecname, sizeof(char), 4);

	// Fill the entire vector
	char ele0 = 'a';
	rc1 = shmvector_push_back(&sv, &(ele0));
	char ele1 = 'b';
	rc1 = shmvector_push_back(&sv, &(ele1));
	char ele2 = 'c';
	rc1 = shmvector_push_back(&sv, &(ele2));
	char ele3 = 'd';
	rc1 = shmvector_push_back(&sv, &(ele3));

	// Use find to find existing elements
	EXPECT_EQ(0, shmvector_find_first_of(&sv, &ele0, test_charcmp));
	EXPECT_EQ(1, shmvector_find_first_of(&sv, &ele1, test_charcmp));
	EXPECT_EQ(2, shmvector_find_first_of(&sv, &ele2, test_charcmp));
	EXPECT_EQ(3, shmvector_find_first_of(&sv, &ele3, test_charcmp));

	// A non-existant element should come back as -1
	char nonele = 'f';
	EXPECT_EQ(-1, shmvector_find_first_of(&sv, &nonele, test_charcmp));

	shmvector_destroy(&sv);
}

static int test_dblcmp(void* l, void* r) {
	double* lhs = ((double*)(l));
	double* rhs = ((double*)(r));
	fprintf(stderr, "Comparing lhs: %f rhs: %f\n", *lhs, *rhs);
	if (*lhs == *rhs) {
		fprintf(stderr, "They're equal\n");
		return 0;
	}
	else {
		fprintf(stderr, "They're NOT equal\n");
		return 1;
	}
}

TEST(shmvector, find_first_of_double) {
    const char* vecname = "/shmvector_find_first_of_double";
    unlink(string(shmdir + string(vecname)).c_str());

	int rc1;
	shmvector_t sv;
	shmvector_create(&sv, vecname, sizeof(double), 8);

	// Fill the entire vector
	double ele0 = 0.123;
	rc1 = shmvector_push_back(&sv, &(ele0));
	double ele1 = 2.345;
	rc1 = shmvector_push_back(&sv, &(ele1));
	double ele2 = 34.567;
	rc1 = shmvector_push_back(&sv, &(ele2));
	double ele3 = 456.789;
	rc1 = shmvector_push_back(&sv, &(ele3));

	// Use find to find existing elements
	EXPECT_EQ(0, shmvector_find_first_of(&sv, &ele0, test_dblcmp));
	EXPECT_EQ(1, shmvector_find_first_of(&sv, &ele1, test_dblcmp));
	EXPECT_EQ(2, shmvector_find_first_of(&sv, &ele2, test_dblcmp));
	EXPECT_EQ(3, shmvector_find_first_of(&sv, &ele3, test_dblcmp));

	// A non-existant element should come back as last_idx + 1
	double nonele = 999.999;
	EXPECT_EQ(-1, shmvector_find_first_of(&sv, &nonele, test_dblcmp));

	// Now delete all elements and search for them
	shmvector_del(&sv, 0);
	shmvector_del(&sv, 1);
	shmvector_del(&sv, 2);
	shmvector_del(&sv, 3);
	EXPECT_EQ(-1, shmvector_find_first_of(&sv, &ele0, test_dblcmp));
	EXPECT_EQ(-1, shmvector_find_first_of(&sv, &ele1, test_dblcmp));
	EXPECT_EQ(-1, shmvector_find_first_of(&sv, &ele2, test_dblcmp));
	EXPECT_EQ(-1, shmvector_find_first_of(&sv, &ele3, test_dblcmp));

	shmvector_destroy(&sv);
}




