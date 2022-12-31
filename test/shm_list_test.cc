
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

int basic_char_cmp(void* lhs, void* rhs) {
    char* l = (char*)lhs;
    char* r = (char*)rhs;
    std::cerr << "lhs: " << l[0] << " rhs: " << r[0] << std::endl;
    if (l[0] == r[0]) {
        return 0;
    }
    return 1;
}

TEST(shmlist, extract_first_match_safe_basic) {
    shmlist_t sl;
    shmlist_create(&sl, "shmlist_extract_first_match_safe_basic", sizeof(char), 1024);

    // Prepopulate the list with 4 elements 
    char ele[5] = "abcd";
    shmlist_add_tail_safe(&sl, &ele[0]);
    shmlist_add_tail_safe(&sl, &ele[1]);
    shmlist_add_tail_safe(&sl, &ele[2]);
    shmlist_add_tail_safe(&sl, &ele[3]);

    /* Match the last element in the list */
    char* item3;
    int rc = shmlist_extract_first_match_safe(&sl, ele + 3, basic_char_cmp, (void**)&item3);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(3, shmlist_length(&sl));
    EXPECT_EQ(ele[0], ((char*)sl.list->next->data)[0]);
    EXPECT_EQ(ele[1], ((char*)sl.list->next->next->data)[0]);
    EXPECT_EQ(ele[2], ((char*)sl.list->next->next->next->data)[0]);
    EXPECT_EQ(ele[3], item3[0]);
    free(item3);

    /* Match the first element in the list */
    char *item0;
    rc = shmlist_extract_first_match_safe(&sl, ele + 0, basic_char_cmp, (void**)&item0);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(2, shmlist_length(&sl));
    EXPECT_EQ(ele[1], ((char*)sl.list->next->data)[0]);
    EXPECT_EQ(ele[2], ((char*)sl.list->next->next->data)[0]);
    EXPECT_EQ(ele[0], item0[0]);
    free(item0);
    
    /* Match the remaining two elements in the list */
    char *item1, *item2;
    rc = shmlist_extract_first_match_safe(&sl, ele + 1, basic_char_cmp, (void**)&item1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(1, shmlist_length(&sl));
    rc = shmlist_extract_first_match_safe(&sl, ele + 2, basic_char_cmp, (void**)&item2);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(0, shmlist_length(&sl));
    EXPECT_EQ(ele[1], item1[0]);
    EXPECT_EQ(ele[2], item2[0]);
    free(item1);
    free(item2);
    
    shmlist_destroy(&sl);

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
