#include "../include/cache.h"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define FILE_NAME "data.txt"

void create_test_file() {
    int fd = open(FILE_NAME, O_RDWR);    
    char buffer[BLOCK_SIZE];
    
    
    for (int i = 0; i < 20; i++) {
        snprintf(buffer, BLOCK_SIZE, "This is block %d\n", i);
        write(fd, buffer, BLOCK_SIZE);
    }
    
    close(fd);
}

void test_load_into_cache() {
    create_test_file();
    
    int fd = lab2_open(FILE_NAME);
    char buffer[BLOCK_SIZE];
    
    lab2_read(fd, buffer, BLOCK_SIZE); // Block 0
    CU_ASSERT_STRING_EQUAL(buffer, "This is block 0\n");
    
    lab2_read(fd, buffer, BLOCK_SIZE); // Block 1
    CU_ASSERT_STRING_EQUAL(buffer, "This is block 1\n");
    
    CU_ASSERT_TRUE(search_in_cache_mem(fd, 0) != NULL);
    CU_ASSERT_TRUE(search_in_cache_mem(fd,BLOCK_SIZE) != NULL);
    
    for (int i = 2; i < CACHE_SIZE; i++) {
        lab2_read(fd, buffer, BLOCK_SIZE);
        char expected_buffer[BLOCK_SIZE];
        snprintf(expected_buffer, BLOCK_SIZE, "This is block %d\n", i);
        CU_ASSERT_STRING_EQUAL(buffer, expected_buffer);
    }
    
    lab2_read(fd, buffer, BLOCK_SIZE); 
    char expected_buffer[CACHE_SIZE + 1];
    snprintf(expected_buffer, BLOCK_SIZE, "This is block %d\n", CACHE_SIZE);
    CU_ASSERT_STRING_EQUAL(buffer, expected_buffer);
    
    CU_ASSERT_TRUE(search_in_cache_mem(fd, 0) == NULL);
    
    lab2_close(fd);
}



void test_write_to_cache() {
    create_test_file();
    
    int fd = lab2_open(FILE_NAME);
    char buffer[BLOCK_SIZE];
    
    snprintf(buffer, BLOCK_SIZE, "Updated block 0\n");
    lseek(fd, 0, SEEK_SET);

    lab2_write(fd, buffer, BLOCK_SIZE);
    
    lseek(fd, 0, SEEK_SET);
    lab2_read(fd, buffer, BLOCK_SIZE);
    CU_ASSERT_STRING_EQUAL(buffer, "Updated block 0\n");
    
    lab2_close(fd);
}

int main() {
    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("Cache Tests", NULL, NULL);
    CU_add_test(suite, "Test LOAD into user cache", test_load_into_cache);
    CU_add_test(suite, "Test WRITE to user cache", test_write_to_cache);
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    
    return 0;
}

