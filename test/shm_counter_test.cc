
#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include "shm_counter.h"
using namespace std;

static string shmdir = "/dev/shm";

/* Test simple creation */
TEST(shmcounter, set_create_simple) {
    const char* set1 = "/shmcounter_set_create_simple";
    const char* set2 = "/shmcounter_set_create_simple2";
    unlink(string(shmdir + string(set1)).c_str());
    unlink(string(shmdir + string(set2)).c_str());
    shmcounter_set_t sc1;
    int rc = shmcounter_set_create(&sc1, set1);
    EXPECT_EQ(0, rc);

    shmcounter_set_t sc1_copy;
    rc = shmcounter_set_create(&sc1_copy, set1);
    EXPECT_EQ(0, rc);

    shmcounter_set_t sc2;
    rc = shmcounter_set_create(&sc2, set2);
    EXPECT_EQ(0, rc);

    shmcounter_set_destroy(&sc1);
    shmcounter_set_destroy(&sc1_copy);
    shmcounter_set_destroy(&sc2);
}

/* Test basic set destruction */
TEST(shmcounter, set_destroy_basic) {

    const char* setname = "/shmcounter_set_destroy_basic";
    unlink(string(shmdir + string(setname)).c_str());

    // Create a counter and destroy
    shmcounter_set_t scs;
    int rc = shmcounter_set_create(&scs, setname);
    EXPECT_EQ(0, rc);
    rc = shmcounter_set_destroy(&scs);
    EXPECT_EQ(0, rc);
}

/* Test simple creation */
TEST(shmcounter, create_simple) {

    shmcounter_set_t scs;
    const char* setname = "/shmcounter_create_simple";
    unlink(string(shmdir + string(setname)).c_str());
    int rc = shmcounter_set_create(&scs, setname);
    EXPECT_EQ(0, rc);

    shmcounter_uid_t id1 = {.group = 1, .ctype = 2, .tag = 3, .lid = 4};
    shmcounter_t sc1;
    rc = shmcounter_create(&sc1, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 0));

    shmcounter_t sc1_copy;
    rc = shmcounter_create(&sc1_copy, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1_copy, 0));

    shmcounter_uid_t id2 = {.group = 12, .ctype = 13, .tag = 14};
    shmcounter_t sc2;
    rc = shmcounter_create(&sc2, &scs, id2);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isvalue(&sc2, 0));

    shmcounter_destroy(&sc1);
    shmcounter_destroy(&sc1_copy);
    shmcounter_destroy(&sc2);
    shmcounter_set_destroy(&scs);
}

/* Test basic destruction */
TEST(shmcounter, destroy_basic) {

    shmcounter_set_t scs;
    const char* setname = "/shmcounter_destroy_basic";
    unlink(string(shmdir + string(setname)).c_str());
    int rc = shmcounter_set_create(&scs, setname);
    EXPECT_EQ(0, rc);

    // Create a counter, increment it, and destroy
    shmcounter_uid_t id1 = {.group = 75, .ctype = 1, .tag = 0, .lid = 4};
    shmcounter_t sc1;
    shmcounter_create(&sc1, &scs, id1);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 0));
    shmcounter_inc_safe(&sc1, 1);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 1));
    shmcounter_destroy(&sc1);

    // Create a new counter with the same name and id, and ensure the counter is 0
    shmcounter_t sc2;
    shmcounter_create(&sc2, &scs, id1);
    EXPECT_EQ(true, shmcounter_isvalue(&sc2, 0));
    shmcounter_destroy(&sc2);

    shmcounter_set_destroy(&scs);
}

/* Test basic destruction */
TEST(shmcounter, destroy_reuse) {

    shmcounter_set_t scs;
    const char* setname = "/shmcounter_destroy_reuse";
    unlink(string(shmdir + string(setname)).c_str());
    int rc = shmcounter_set_create(&scs, setname);
    EXPECT_EQ(0, rc);

    // Create a counter, increment it, and destroy
    shmcounter_uid_t id1 = {.group = 75, .ctype = 1, .tag = 0, .lid = 4};
    shmcounter_t sc1;
    shmcounter_create(&sc1, &scs, id1);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 0));
    shmcounter_inc_safe(&sc1, 1);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 1));
    shmcounter_destroy(&sc1);

    // Create a new counter with the same name and id, and ensure the counter is 0
    shmcounter_t sc2;
    shmcounter_create(&sc2, &scs, id1);
    EXPECT_EQ(true, shmcounter_isvalue(&sc2, 0));

    // Ensure that only 1 slot in the counter set has been used
    EXPECT_EQ(1, scs.v->shm->active_count);

    shmcounter_destroy(&sc2);
    shmcounter_set_destroy(&scs);
}

/* Test increment */
TEST(shmcounter, inc_basic) {

    shmcounter_set_t scs;
    const char* setname = "/shmcounter_inc_basic";
    unlink(string(shmdir + string(setname)).c_str());
    int rc = shmcounter_set_create(&scs, setname);
    EXPECT_EQ(0, rc);

    shmcounter_uid_t id1 = {.group = 1, .ctype = 1, .tag = 1, .lid = 1};
    shmcounter_t sc1;
    rc = shmcounter_create(&sc1, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 0));

    shmcounter_t sc1_copy;
    rc = shmcounter_create(&sc1_copy, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1_copy, 0));

    shmcounter_inc_safe(&sc1, 1);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 1));
    EXPECT_EQ(true, shmcounter_isvalue(&sc1_copy, 1));

    shmcounter_inc_safe(&sc1_copy, 1);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 2));
    EXPECT_EQ(true, shmcounter_isvalue(&sc1_copy, 2));

    shmcounter_destroy(&sc1);
    shmcounter_destroy(&sc1_copy);
    shmcounter_set_destroy(&scs);
}

/* Test decrement */
TEST(shmcounter, dec_basic) {

    shmcounter_set_t scs;
    const char* setname = "/shmcounter_dec_basic";
    unlink(string(shmdir + string(setname)).c_str());
    int rc = shmcounter_set_create(&scs, setname);
    EXPECT_EQ(0, rc);

    shmcounter_uid_t id1 = {.group = 1, .ctype = 1, .tag = 1, .lid = 1};
    shmcounter_t sc1;
    rc = shmcounter_create(&sc1, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 0));

    shmcounter_t sc1_copy;
    rc = shmcounter_create(&sc1_copy, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1_copy, 0));

    shmcounter_dec_safe(&sc1, 1);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, -1));
    EXPECT_EQ(true, shmcounter_isvalue(&sc1_copy, -1));

    shmcounter_dec_safe(&sc1_copy, 1);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, -2));
    EXPECT_EQ(true, shmcounter_isvalue(&sc1_copy, -2));

    shmcounter_destroy(&sc1);
    shmcounter_destroy(&sc1_copy);
    shmcounter_set_destroy(&scs);
}

TEST(shmcounter, value_basic) {

    shmcounter_set_t scs;
    const char* setname = "/shmcounter_value_basic";
    unlink(string(shmdir + string(setname)).c_str());
    int rc = shmcounter_set_create(&scs, setname);
    EXPECT_EQ(0, rc);

    shmcounter_uid_t id1 = {.group = 1, .ctype = 1, .tag = 1, .lid = 1};
    shmcounter_t sc1;
    rc = shmcounter_create(&sc1, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(0, shmcounter_value(&sc1));

    shmcounter_t sc1_copy;
    rc = shmcounter_create(&sc1_copy, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(0, shmcounter_value(&sc1_copy));

    shmcounter_inc_safe(&sc1, 1);
    EXPECT_EQ(1, shmcounter_value(&sc1));
    EXPECT_EQ(1, shmcounter_value(&sc1_copy));

    shmcounter_inc_safe(&sc1_copy, 1);
    EXPECT_EQ(2, shmcounter_value(&sc1));
    EXPECT_EQ(2, shmcounter_value(&sc1_copy));

    shmcounter_destroy(&sc1);
    shmcounter_destroy(&sc1_copy);
    shmcounter_set_destroy(&scs);
}

TEST(shmcounter, isvalue_basic) {

    shmcounter_set_t scs;
    const char* setname = "/shmcounter_isvalue_basic";
    unlink(string(shmdir + string(setname)).c_str());
    int rc = shmcounter_set_create(&scs, setname);
    EXPECT_EQ(0, rc);

    shmcounter_uid_t id1 = {.group = 1, .ctype = 1, .tag = 1, .lid = 1};
    shmcounter_t sc1;
    rc = shmcounter_create(&sc1, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 0));

    shmcounter_t sc1_copy;
    rc = shmcounter_create(&sc1_copy, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1_copy, 0));

    shmcounter_inc_safe(&sc1, 1);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 1));
    EXPECT_EQ(true, shmcounter_isvalue(&sc1_copy, 1));

    shmcounter_inc_safe(&sc1_copy, 1);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 2));
    EXPECT_EQ(true, shmcounter_isvalue(&sc1_copy, 2));

    shmcounter_destroy(&sc1);
    shmcounter_destroy(&sc1_copy);
    shmcounter_set_destroy(&scs);
}

TEST(shmcounter, isequal_safe_basic) {

    shmcounter_set_t scs;
    const char* setname = "/shmcounter_isequal_safe_basic";
    unlink(string(shmdir + string(setname)).c_str());
    int rc = shmcounter_set_create(&scs, setname);
    EXPECT_EQ(0, rc);


    shmcounter_uid_t id1 = {.group = 1, .ctype = 1, .tag = 1, .lid = 1};
    shmcounter_t sc1;
    rc = shmcounter_create(&sc1, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 0));

    shmcounter_uid_t id2 = {.group = 1, .ctype = 1, .tag = 1, .lid = 2};
    shmcounter_t sc2;
    rc = shmcounter_create(&sc2, &scs, id2);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 0));

    // Increment only one counter and ensure inequality
    shmcounter_inc_safe(&sc1, 4);
    EXPECT_EQ(false, shmcounter_isequal_safe(&sc1, &sc2));

    // Increment only one counter and ensure inequality
    shmcounter_inc_safe(&sc2, 2);
    EXPECT_EQ(false, shmcounter_isequal_safe(&sc1, &sc2));

    // Increment only one counter and ensure equality
    shmcounter_inc_safe(&sc2, 2);
    EXPECT_EQ(true, shmcounter_isequal_safe(&sc1, &sc2));
    EXPECT_EQ(true, shmcounter_isequal_safe(&sc2, &sc1));

    // Cleanup
    shmcounter_destroy(&sc1);
    shmcounter_destroy(&sc2);
    shmcounter_set_destroy(&scs);
}

TEST(shmcounter, set_if_zero_safe_basic) {

    shmcounter_set_t scs;
    const char* setname = "/shmcounter_set_if_zero_safe_basic";
    unlink(string(shmdir + string(setname)).c_str());
    int rc = shmcounter_set_create(&scs, setname);
    EXPECT_EQ(0, rc);

    shmcounter_uid_t id1 = {.group = 1, .ctype = 1, .tag = 1, .lid = 1};
    shmcounter_t sc1;
    rc = shmcounter_create(&sc1, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 0));
    EXPECT_EQ(true, shmcounter_set_if_zero_safe(&sc1, 11));
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 11));

    shmcounter_t sc1_copy;
    rc = shmcounter_create(&sc1_copy, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 11));
    EXPECT_EQ(false, shmcounter_set_if_zero_safe(&sc1, 4));
    EXPECT_EQ(true, shmcounter_isvalue(&sc1, 11));

    // Cleanup
    shmcounter_destroy(&sc1);
    shmcounter_destroy(&sc1_copy);
    shmcounter_set_destroy(&scs);
}


