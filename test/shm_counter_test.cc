
#include <gtest/gtest.h>
#include "shm_counter.h"

/* Test simple creation */
TEST(shmcounter, set_create_simple) {

    shmcounter_set_t sc1;
    int rc = shmcounter_set_create(&sc1, "/shmcounter_set_create_simple");
    EXPECT_EQ(0, rc);

    shmcounter_set_t sc1_copy;
    rc = shmcounter_set_create(&sc1_copy, "/shmcounter_set_create_simple");
    EXPECT_EQ(0, rc);

    shmcounter_set_t sc2;
    rc = shmcounter_set_create(&sc2, "/shmcounter_set_create_simple2");
    EXPECT_EQ(0, rc);

    shmcounter_set_destroy(&sc1);
    shmcounter_set_destroy(&sc1_copy);
    shmcounter_set_destroy(&sc2);
}

/* Test basic set destruction */
TEST(shmcounter, set_destroy_basic) {

    // Create a counter and destroy
    shmcounter_set_t scs;
    int rc = shmcounter_set_create(&scs, "/shmcounter-basic_destroy");
    EXPECT_EQ(0, rc);
    rc = shmcounter_set_destroy(&scs);
    EXPECT_EQ(0, rc);
}

/* Test simple creation */
TEST(shmcounter, create_simple) {

    shmcounter_set_t scs;
    int rc = shmcounter_set_create(&scs, "/shmcounter_create_simple");
    EXPECT_EQ(0, rc);

    shmcounter_uid_t id1 = {.group = 1, .ctype = 2, .tag = 3};
    shmcounter_t sc1;
    rc = shmcounter_create(&sc1, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isequal(&sc1, 0));

    shmcounter_t sc1_copy;
    rc = shmcounter_create(&sc1_copy, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isequal(&sc1_copy, 0));

    shmcounter_uid_t id2 = {.group = 12, .ctype = 13, .tag = 14};
    shmcounter_t sc2;
    rc = shmcounter_create(&sc2, &scs, id2);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isequal(&sc2, 0));

    shmcounter_destroy(&sc1);
    shmcounter_destroy(&sc1_copy);
    shmcounter_destroy(&sc2);
    shmcounter_set_destroy(&scs);
}

/* Test basic destruction */
TEST(shmcounter, destroy_basic) {

    shmcounter_set_t scs;
    int rc = shmcounter_set_create(&scs, "/shmcounter_destroy_basic");
    EXPECT_EQ(0, rc);

    // Create a counter, increment it, and destroy
    shmcounter_uid_t id1 = {.group = 75, .ctype = 1, .tag = 0};
    shmcounter_t sc1;
    shmcounter_create(&sc1, &scs, id1);
    EXPECT_EQ(true, shmcounter_isequal(&sc1, 0));
    shmcounter_inc_safe(&sc1, 1);
    EXPECT_EQ(true, shmcounter_isequal(&sc1, 1));
    shmcounter_destroy(&sc1);

    // Create a new counter with the same name and id, and ensure the counter is 0
    shmcounter_t sc2;
    shmcounter_create(&sc2, &scs, id1);
    EXPECT_EQ(true, shmcounter_isequal(&sc2, 0));
    shmcounter_destroy(&sc2);

    shmcounter_set_destroy(&scs);
}

/* Test increment */
TEST(shmcounter, inc_basic) {
}

/* Test decrement */
TEST(shmcounter, dec_basic) {
}

TEST(shmcounter, isequal_basic) {

    shmcounter_set_t scs;
    int rc = shmcounter_set_create(&scs, "/shmcounter_isequal_basic");
    EXPECT_EQ(0, rc);

    shmcounter_uid_t id1 = {.group = 1, .ctype = 1, .tag = 1};
    shmcounter_t sc1;
    rc = shmcounter_create(&sc1, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isequal(&sc1, 0));

    shmcounter_t sc1_copy;
    rc = shmcounter_create(&sc1_copy, &scs, id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isequal(&sc1_copy, 0));

    shmcounter_inc_safe(&sc1, 1);
    EXPECT_EQ(true, shmcounter_isequal(&sc1, 1));
    EXPECT_EQ(true, shmcounter_isequal(&sc1_copy, 1));

    shmcounter_inc_safe(&sc1_copy, 1);
    EXPECT_EQ(true, shmcounter_isequal(&sc1, 2));
    EXPECT_EQ(true, shmcounter_isequal(&sc1_copy, 2));

    shmcounter_destroy(&sc1);
    shmcounter_destroy(&sc1_copy);
    shmcounter_set_destroy(&scs);
}

