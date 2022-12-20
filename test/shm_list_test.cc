
#include <gtest/gtest.h>
#include "shm_list.h"

/* Test empty creation */
TEST(shmlist, empty_creation) {
    shmlist_t sl1;
    shmlist_create(&sl1, "shmlist-empty_creation", sizeof(int), 0);
    EXPECT_EQ(0, shmlist_length(&sl1));

    shmlist_t sl2;
    shmlist_create(&sl2, "shmlist-empty_creation", sizeof(int), 0);
    EXPECT_EQ(0, shmlist_length(&sl2));

    shmlist_destroy(&sl1);
    shmlist_destroy(&sl2);
}

/* Test basic destruction */
TEST(shmlist, basic_destroy) {
    shmlist_t sl1;
    shmlist_create(&sl1, "shmlist-basic_destroy", sizeof(int), 1024);
    shmlist_destroy(&sl1);
    EXPECT_EQ(0, 0);
}

/* Test add_tail */
TEST(shmlist, basic_add_tail_safe) {
    shmlist_t sl1;
    shmlist_create(&sl1, "shmlist-basic_destroy", sizeof(char), 1024);

    char ele[5] = "abcd";
    shmlist_add_tail_safe(&sl1, &ele[0]);
    EXPECT_EQ(1, shmlist_length(&sl1));
    EXPECT_EQ(ele[0], ((char*)sl1.list->prev->data)[0]);

    shmlist_add_tail_safe(&sl1, &ele[1]);
    EXPECT_EQ(2, shmlist_length(&sl1));
    EXPECT_EQ(ele[1], ((char*)sl1.list->prev->data)[0]);

    shmlist_add_tail_safe(&sl1, &ele[2]);
    EXPECT_EQ(3, shmlist_length(&sl1));
    EXPECT_EQ(ele[2], ((char*)sl1.list->prev->data)[0]);

    shmlist_add_tail_safe(&sl1, &ele[3]);
    EXPECT_EQ(4, shmlist_length(&sl1));
    EXPECT_EQ(ele[3], ((char*)sl1.list->prev->data)[0]);

    shmlist_destroy(&sl1);
}

/* Test del */
TEST(shmlist, basic_del_safe) {
    shmlist_t sl1;
    shmlist_create(&sl1, "shmlist-basic_destroy", sizeof(char), 1024);

    // Prepopulate the list with 4 elements */
    char ele[5] = "abcd";
    shmlist_add_tail_safe(&sl1, &ele[0]);
    shmlist_add_tail_safe(&sl1, &ele[1]);
    shmlist_add_tail_safe(&sl1, &ele[2]);
    shmlist_add_tail_safe(&sl1, &ele[3]);

    // Test delete a regular data node */
    EXPECT_EQ(4, shmlist_length(&sl1));
    shmlist_del_safe(sl1.next);
    EXPECT_EQ(3, shmlist_length(&sl1));
    EXPECT_EQ(ele[1], ((char*)sl1.list->next->data)[0]);
    EXPECT_EQ(ele[2], ((char*)sl1.list->next->next->data)[0]);
    EXPECT_EQ(ele[3], ((char*)sl1.list->next->next->next->data)[0]);

    // Test deleting the list head using the list node */
    shmlist_del_safe(sl1.list);


    shmlist_destroy(&sl1);
    EXPECT_EQ(0, 0);
}

TEST(shmlist, basic_shmlist_for_each_safe) {
    shmlist_t sl1;
    shmlist_create(&sl1, "shmlist-basic_shmlist_for_each_safe", sizeof(char), 1024);

    // Prepopulate the list with 4 elements 
    char ele[5] = "abcd";
    shmlist_add_tail_safe(&sl1, &ele[0]);
    shmlist_add_tail_safe(&sl1, &ele[1]);
    shmlist_add_tail_safe(&sl1, &ele[2]);
    shmlist_add_tail_safe(&sl1, &ele[3]);

    // Loop over the elements
    //int i = 0;
    //char* iter, iter2;
    //shmlist_for_each_safe(&sl, iter, iter2, char) {
    //    EXPECT_EQ(ele[i], iter[0]);
    //    i++;
   // }

    shmlist_destroy(&sl1);

}

TEST(shmlist, basic_shmlist_is_empty) {
    shmlist_t sl1;
    shmlist_create(&sl1, "shmlist-basic_is_empty", sizeof(char), 1);

    // Test a new empty list
    EXPECT_EQ(1, shmlist_is_empty(&sl1));

    // Add 1 element and test
    char ele = 'a';
    shmlist_add_tail_safe(&sl1, &ele);
    EXPECT_EQ(0, shmlist_is_empty(&sl1));

    // Delete 1 element and test 
    shmlist_del_safe(sl1.next);
    EXPECT_EQ(1, shmlist_is_empty(&sl1));

    shmlist_destroy(&sl1);
}

TEST(shmlist, basic_shmlist_extract_head_safe) {
    shmlist_t sl1;
    shmlist_create(&sl1, "shmlist-basic_destroy", sizeof(char), 1024);

    // Prepopulate the list with 4 elements 
    char ele[5] = "abcd";
    shmlist_add_tail_safe(&sl1, &ele[0]);
    shmlist_add_tail_safe(&sl1, &ele[1]);
    shmlist_add_tail_safe(&sl1, &ele[2]);
    shmlist_add_tail_safe(&sl1, &ele[3]);

    shmlist_destroy(&sl1);

}

TEST(shmlist, basic_shmlist_length) {

}

TEST(shmlist, basic_shmlist_head) {
    
}

TEST(shmlist, basic_shmlist_next) {

    // Tail next should be null

}

TEST(shmlist, shmlist_insert_after_safe) {

}

TEST(shmlist, basic_shmlist_insert_before_safe) {

}
