/*
    cache.c - this is open API to communicate with files.
*/

#include "../include/cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>

#define BLOCK_SIZE 4096 

/* Cache pages array. */ 
static cache_write_t cache[CACHE_SIZE];

/* 
    Procedure get cache_page and calling write() 
    If write operation was successfull, setting valid flag to FALSE.
 */
static void do_write_and_set_invalid(cache_write_t * cache_page) {
    cache_page->valid = 0;
}

/* Checkout valid flag of cache_page. */
static bool find_not_valid_page(cache_write_t * cache_page){
    return cache_page->valid;
}

/*
    The procedure initializes cache_page fields, allocate memory for cache_page->data.
*/
static void init_cache_page(cache_write_t * cache_page, int fd, ino_t inode, off_t offset){
    cache_page->valid = 1;
    cache_page->fd = fd;
    cache_page->inode = inode;
    cache_page->offset = offset;
    cache_page->data = malloc(BLOCK_SIZE);
    
    if (cache_page->data == NULL)
    {
        perror("Allocate memory was failed");
        exit(EXIT_FAILURE);
    }
    
    cache_page->referenced = 1;   
}

/* 
    The procedure set offset from beginning of file.
*/
static void read_from_fd_to_buf(cache_write_t * cache_page, int fd, off_t offset) {
    lab2_lseek(fd, offset, SEEK_SET);
    if (read(fd,cache_page->data, BLOCK_SIZE) == -1) {
        perror("Read operation was failed!");
        exit(EXIT_FAILURE);
    }
}

/*
    Important procedure because it's throw out page from cache. 
    First of all, we try to find not valid page by find_not_valid_page(),
    if page was found we checking referenced flag. If flag is 0 lseek ret-
    urn offset of current page and will be setting valid flag to 0.
    In other case, will be setting referenced field 0.
*/
static void eject_page() {
    for (size_t i = 0; ; i = (i + 1) % CACHE_SIZE) {
        cache_write_t * cache_page = &cache[i];
        if (find_not_valid_page(cache_page)) {
            if (!cache_page->referenced) {
                lab2_lseek(cache_page->fd, cache_page->offset, SEEK_SET);
                do_write_and_set_invalid(cache_page);
                return;
            } else {
                cache_page->referenced = 0;
            }
        } else {
            cache_page->valid=0;
        }
    }
}

/*
    This function get inode and offset. Searching in user cache memory 
    if inode and offset equals and page is valid then return address where 
    located cache page or NULL.
*/
cache_write_t * search_in_cache_mem(ino_t inode, off_t offset) {
    for (size_t i = 0; i < CACHE_SIZE; i++) {
        if (find_not_valid_page(&cache[i]) && 
            cache[i].inode == inode && 
            cache[i].offset == offset) 
        {
            return &cache[i];
        }
    }
    return NULL;
}

/*
    This function load page to cache memory. 
*/
static cache_write_t * load_into_cache_mem(ino_t inode, int fd, off_t offset) {
    cache_write_t  * cache_page;
    for (size_t i = 0; i < CACHE_SIZE; i++) {
        cache_page = &cache[i];
        if (!find_not_valid_page(cache_page)) {
            init_cache_page(cache_page, fd, inode, offset);
            read_from_fd_to_buf(cache_page, fd, offset);
            return cache_page;
        }
    }

    eject_page();
    return load_into_cache_mem(inode, fd, offset);
}

/*
    Function get filename and include libc open(). 
    File opening with READ and WRITE accesses. If function return -1
    user have to choose print or skip logs. In other case, return file 
    descriptor (fd).
*/
int lab2_open(const char *path) {
    int fd = open(path, O_RDWR  );
    if (fd < 0) {
        char * message_from_user;
        printf("\nOutput Yes or No:");
        scanf("%s", message_from_user);
        if (message_from_user == "Yes\n")
        {
            perror("Failed to open file!");
            exit(EXIT_FAILURE);
        } 
        return -1;
    }
    return fd;
}

/*
    Function get file descriptor (fd) and in for cycle find this page
    and free memory.  
*/
int lab2_close(int fd) {
    struct stat f_stat;
    fstat(fd, &f_stat);
    ino_t inode = f_stat.st_ino;

    for (size_t i = 0; i < CACHE_SIZE; i++) {
        if (find_not_valid_page(&cache[i]) && cache[i].fd == fd) {
            lab2_lseek(fd, cache[i].offset, SEEK_SET);
            do_write_and_set_invalid(&cache[i]);
            free(cache[i].data);
            cache[i].valid = 0;        
        }
    }
    return close(fd);
}

/*
    Reading data from file. 
*/
ssize_t lab2_read(int fd, void *buf, size_t count) {
    struct stat f_stat;
    fstat(fd, &f_stat);
    if (fstat(fd, &f_stat) == -1) {
        return -1; 
    }
    off_t offset = lab2_lseek(fd, 0, SEEK_CUR);
    if (offset == -1) {
        return -1; 
    }

    cache_write_t * entry = search_in_cache_mem(f_stat.st_ino, offset);
    size_t remaining_bytes = f_stat.st_size - offset;
    size_t bytes_to_read = (remaining_bytes < count) ? remaining_bytes : count;

    if (!entry)
    {   
        entry = load_into_cache_mem(f_stat.st_ino,fd, offset);
    } 
    memcpy(buf, entry->data, bytes_to_read);
    entry->referenced = 1;

    return bytes_to_read;
}

/*
    Function try to search in cache memory this page. if entry not NULL (was found)
    we write of BUF to FD and load page into cache memory.  
*/
ssize_t lab2_write(int fd, void *buf, size_t count) {
    struct stat f_stat;
    fstat(fd, &f_stat);
    
    off_t offset = lab2_lseek(fd, 0, SEEK_CUR);

    cache_write_t *entry = search_in_cache_mem(fd, offset);

    if (!entry)
    {
        if(write(fd, buf, count)==-1){
            perror("write was failed");
            exit(EXIT_FAILURE);
        }
        entry = load_into_cache_mem(f_stat.st_ino,fd, offset);
    } 
    memcpy(buf, entry->data, count);
    entry->referenced = 1; // flag of link

    return count;
}

/*
    Function return offset.
*/
off_t lab2_lseek(int fd, off_t offset, int whence) {
    return lseek(fd, offset, whence);
}

/*
    Make all changes done to FD actually appear on disk.
*/
int lab2_fsync(int fd){
    return fsync(fd);
}