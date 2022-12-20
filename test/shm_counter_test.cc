
#include <gtest/gtest.h>
#include "shm_counter.h"

/* Test simple creation */
TEST(shmcounter, simple_creation) {

    shmcounter_uid_t id1 = {.group = 1, .ctype = 1, .tag = 1};
    shmcounter_t sc1;
    int rc = shmcounter_create(&sc1, "/shmcounter-simple_creation", id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isequal(&sc1, 0));

    shmcounter_t sc1_copy;
    rc = shmcounter_create(&sc1_copy, "/shmcounter-simple_creation", id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isequal(&sc1_copy, 0));

    shmcounter_uid_t id2 = {.group = 2, .ctype = 2, .tag = 2};
    shmcounter_t sc2;
    rc = shmcounter_create(&sc2, "shmcounter-simple_creation", id2);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isequal(&sc2, 0));

    shmcounter_destroy(&sc1);
    shmcounter_destroy(&sc1_copy);
    shmcounter_destroy(&sc2);
}

/* Test basic destruction */
TEST(shmcounter, basic_destroy) {

    shmcounter_uid_t id1 = {0};
    shmcounter_t sl1;
    shmcounter_create(&sl1, "shmcounter-basic_destroy", id1);
    shmcounter_destroy(&sl1);
    EXPECT_EQ(0, 0);
}

/* Test increment */
TEST(shmcounter, basic_inc) {
}

/* Test decrement */
TEST(shmcounter, basic_dec) {
}

TEST(shmcounter, basic_isequal) {

    shmcounter_uid_t id1 = {.group = 1, .ctype = 1, .tag = 1};
    shmcounter_t sc1;
    int rc = shmcounter_create(&sc1, "/shmcounter-basic_isequal", id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isequal(&sc1, 0));

    shmcounter_t sc1_copy;
    rc = shmcounter_create(&sc1_copy, "/shmcounter-basic_isequal", id1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(true, shmcounter_isequal(&sc1_copy, 0));

    shmcounter_inc_safe(&sc1, 1);
    EXPECT_EQ(true, shmcounter_isequal(&sc1, 1));
    EXPECT_EQ(true, shmcounter_isequal(&sc1_copy, 1));

    shmcounter_inc_safe(&sc1_copy, 1);
    EXPECT_EQ(true, shmcounter_isequal(&sc1, 2));
    EXPECT_EQ(true, shmcounter_isequal(&sc1_copy, 2));

}

