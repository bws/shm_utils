
#include <gtest/gtest.h>
#include <string>
#include "shm_list.h"
using namespace std;

static string shmdir = "/dev/shm";

/* Test empty creation */
TEST(shmlist, create_empty) {
    const char* listname = "/shmlist_create_empty";
    unlink(string(shmdir + string(listname)).c_str());
    shmlist_t sl1;
    shmlist_create(&sl1, listname, sizeof(int), 0);
    EXPECT_EQ(0, shmlist_length(&sl1));

    shmlist_t sl2;
    shmlist_create(&sl2, listname, sizeof(int), 0);
    EXPECT_EQ(0, shmlist_length(&sl2));

    shmlist_destroy(&sl1);
    shmlist_destroy(&sl2);
}

/* Test creation with large types */
TEST(shmlist, create_large) {
    const char* listname = "/shmlist_create_large";
    unlink(string(shmdir + string(listname)).c_str());
    shmlist_t sl1;
    shmlist_create(&sl1, listname, 192, 2048);
    EXPECT_EQ(0, shmlist_length(&sl1));

    shmlist_t sl2;
    shmlist_create(&sl2, listname, 192, 2048);
    EXPECT_EQ(0, shmlist_length(&sl2));

    shmlist_destroy(&sl1);
    shmlist_destroy(&sl2);
}

/* Test basic destruction */
TEST(shmlist, destroy_basic) {
    const char* listname = "/shmlist_destroy_basic";
    unlink(string(shmdir + string(listname)).c_str());

    shmlist_t sl1;
    shmlist_create(&sl1, listname, sizeof(int), 1024);
    shmlist_destroy(&sl1);
    EXPECT_EQ(0, 0);
}

/* Test add_tail */
TEST(shmlist, add_tail_safe_basic) {
    const char* listname = "/shmlist_add_tail_safe_basic";
    unlink(string(shmdir + string(listname)).c_str());

    shmlist_t sl1;
    shmlist_create(&sl1, listname, sizeof(char), 1024);
    std::cerr << "Curr: " << sl1.cur_idx_unsafe << std::endl;

    char ele[5] = "abcd";
    int rc = shmlist_add_tail_safe(&sl1, &ele[0]);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(1, shmlist_length(&sl1));
    char e0 = ((char*)shmlist_get_data(shmlist_tail(&sl1)))[0];
    EXPECT_EQ(ele[0], e0);
    EXPECT_EQ(1, sl1.cur_idx_unsafe);
    
    rc = shmlist_add_tail_safe(&sl1, &ele[1]);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(2, shmlist_length(&sl1));
    char e1 = ((char*)shmlist_get_data(shmlist_tail(&sl1)))[0];
    EXPECT_EQ(ele[1], e1);
    EXPECT_EQ(2, sl1.cur_idx_unsafe);

    rc = shmlist_add_tail_safe(&sl1, &ele[2]);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(3, shmlist_length(&sl1));
    char e2 = ((char*)shmlist_get_data(shmlist_tail(&sl1)))[0];
    EXPECT_EQ(ele[2], e2);
    EXPECT_EQ(3, sl1.cur_idx_unsafe);

    rc = shmlist_add_tail_safe(&sl1, &ele[3]);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(4, shmlist_length(&sl1));
    char e3 = ((char*)shmlist_get_data(shmlist_tail(&sl1)))[0];
    EXPECT_EQ(ele[3], e3);
    EXPECT_EQ(4, sl1.cur_idx_unsafe);

    shmlist_destroy(&sl1);
}

/* Test del */
TEST(shmlist, del_safe_basic) {
    const char* listname = "/shmlist_del_safe_basic";
    unlink(string(shmdir + string(listname)).c_str());

    shmlist_t sl1;
    shmlist_create(&sl1, listname, sizeof(char), 1024);

    // Prepopulate the list with 4 elements
    char ele[5] = "abcd";
    shmlist_add_tail_safe(&sl1, &ele[0]);
    shmlist_add_tail_safe(&sl1, &ele[1]);
    shmlist_add_tail_safe(&sl1, &ele[2]);
    shmlist_add_tail_safe(&sl1, &ele[3]);

    // Test delete a regular data node
    EXPECT_EQ(4, shmlist_length(&sl1));
    int rc = shmlist_del_safe(shmlist_head(&sl1));
    EXPECT_EQ(0, rc);
    EXPECT_EQ(3, shmlist_length(&sl1));
    char e1 = ((char*)shmlist_get_data(shmlist_head(&sl1)))[0];
    EXPECT_EQ(ele[1], e1);
    char e2 = ((char*)shmlist_get_data(shmlist_next(&sl1)))[0];
    EXPECT_EQ(ele[2], e2);
    char e3 = ((char*)shmlist_get_data(shmlist_next(&sl1)))[0];
    EXPECT_EQ(ele[3], e3);

    // Test deleting the list dummy node -- a no-op
    sl1.cur_idx_unsafe = 0;
    rc = shmlist_del_safe(&sl1);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(3, shmlist_length(&sl1));

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
    const char* listname = "/shmlist_extract_first_match_safe_basic";
    unlink(string(shmdir + string(listname)).c_str());

    shmlist_t sl;
    shmlist_create(&sl, listname, sizeof(char), 1024);

    // Prepopulate the list with 4 elements 
    char ele[5] = "abcd";
    shmlist_add_tail_safe(&sl, &ele[0]);
    shmlist_add_tail_safe(&sl, &ele[1]);
    shmlist_add_tail_safe(&sl, &ele[2]);
    shmlist_add_tail_safe(&sl, &ele[3]);
    char e0 = ((char*)shmlist_get_data(shmlist_head(&sl)))[0];
    char e1 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    char e2 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    char e3 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    EXPECT_EQ(ele[0], e0);
    EXPECT_EQ(ele[1], e1);
    EXPECT_EQ(ele[2], e2);
    EXPECT_EQ(ele[3], e3);

    // Match the last element in the list
    char* item3;
    int rc = shmlist_extract_first_match_safe(&sl, ele + 3, basic_char_cmp, (void**)&item3);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(ele[3], item3[0]);
    free(item3);

    // Now confirm list state is missing only the last element
    EXPECT_EQ(3, shmlist_length(&sl));
    e0 = ((char*)shmlist_get_data(shmlist_head(&sl)))[0];
    EXPECT_EQ(ele[0], e0);
    e1 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    EXPECT_EQ(ele[1], e1);
    e2 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    EXPECT_EQ(ele[2], e2);

    // Match the first element in the list
    char *item0;
    rc = shmlist_extract_first_match_safe(&sl, ele + 0, basic_char_cmp, (void**)&item0);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(ele[0], item0[0]);
    free(item0);
    // Confirm list state is correct
    EXPECT_EQ(2, shmlist_length(&sl));
    e1 = ((char*)shmlist_get_data(shmlist_head(&sl)))[0];
    EXPECT_EQ(ele[1], e1);
    e2 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    EXPECT_EQ(ele[2], e2);

    // Match the remaining two elements in the list
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

TEST(shmlist, extract_n_matches_safe_basic) {
    const char* listname = "/shmlist_extract_n_matches_safe_basic";
    unlink(string(shmdir + string(listname)).c_str());

    shmlist_t sl;
    shmlist_create(&sl, listname, sizeof(char), 1024);

    // Prepopulate the list with 8 elements 
    char ele[9] = "abababcd";
    shmlist_add_tail_safe(&sl, &ele[0]);
    shmlist_add_tail_safe(&sl, &ele[1]);
    shmlist_add_tail_safe(&sl, &ele[2]);
    shmlist_add_tail_safe(&sl, &ele[3]);
    shmlist_add_tail_safe(&sl, &ele[4]);
    shmlist_add_tail_safe(&sl, &ele[5]);
    shmlist_add_tail_safe(&sl, &ele[6]);
    shmlist_add_tail_safe(&sl, &ele[7]);
    char e0 = ((char*)shmlist_get_data(shmlist_head(&sl)))[0];
    char e1 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    char e2 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    char e3 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    char e4 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    char e5 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    char e6 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    char e7 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    EXPECT_EQ(ele[0], e0);
    EXPECT_EQ(ele[1], e1);
    EXPECT_EQ(ele[2], e2);
    EXPECT_EQ(ele[3], e3);
    EXPECT_EQ(ele[4], e4);
    EXPECT_EQ(ele[5], e5);
    EXPECT_EQ(ele[6], e6);
    EXPECT_EQ(ele[7], e7);

    // Match the first 'a' element in the list
    char a = 'a';
    size_t max_match = 1;
    size_t match_count;
    char* items;
    int rc = shmlist_extract_n_matches_safe(&sl, max_match, &a, basic_char_cmp, &match_count, (void**)&items);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(7, shmlist_length(&sl));
    EXPECT_EQ(1, match_count);
    EXPECT_EQ('a', items[0]);
    free(items);

    // Match the all 'b' elements in the list
    char b = 'b';
    max_match = 8;
    items = NULL;
    rc = shmlist_extract_n_matches_safe(&sl, max_match, &b, basic_char_cmp, &match_count, (void**)&items);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(4, shmlist_length(&sl));
    EXPECT_EQ(3, match_count);
    EXPECT_EQ('b', items[0]);
    EXPECT_EQ('b', items[1]);
    EXPECT_EQ('b', items[2]);
    free(items);

    // Now confirm list state is "aacd"
    EXPECT_EQ(4, shmlist_length(&sl));
    e0 = ((char*)shmlist_get_data(shmlist_head(&sl)))[0];
    EXPECT_EQ('a', e0);
    e1 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    EXPECT_EQ('a', e1);
    e2 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    EXPECT_EQ('c', e2);
    e3 = ((char*)shmlist_get_data(shmlist_next(&sl)))[0];
    EXPECT_EQ('d', e3);

    shmlist_destroy(&sl);
}

TEST(shmlist, shmlist_is_empty_basic) {
    const char* listname = "/shmlist_is_empty_basic";
    unlink(string(shmdir + string(listname)).c_str());

    shmlist_t sl1;
    shmlist_create(&sl1, listname, sizeof(char), 1);

    // Test a new empty list
    EXPECT_EQ(1, shmlist_is_empty(&sl1));

    // Add 1 element and test
    char ele = 'a';
    int rc = shmlist_add_tail_safe(&sl1, &ele);
    EXPECT_EQ(0, rc);
    EXPECT_EQ(1, sl1.cur_idx_unsafe);
    EXPECT_EQ(0, shmlist_is_empty(&sl1));

    // Delete 1 element and test 
    shmlist_del_safe(shmlist_head(&sl1));
    EXPECT_EQ(1, shmlist_is_empty(&sl1));

    shmlist_destroy(&sl1);
}

TEST(shmlist, basic_shmlist_extract_head_safe) {
}

TEST(shmlist, basic_shmlist_length) {
}

TEST(shmlist, basic_shmlist_head) {
}

TEST(shmlist, basic_shmlist_next) {
}

TEST(shmlist, shmlist_insert_after_safe) {
}

TEST(shmlist, basic_shmlist_insert_before_safe) {
}
