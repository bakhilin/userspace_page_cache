#include "../include/cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define CACHE_SIZE 10
#define BLOCK_SIZE 4096

static cache_write_t cache[CACHE_SIZE];
static int cache_count = 0;

static void eject_page(){
    int i = 0;
    for (;;)
    {
        if (cache[i].valid)
        {
            if (!cache[i].referenced)
            {
                lseek(cache[i].fd, cache[i].offset, SEEK_SET);
                write(cache[i].fd, cache[i].data, BLOCK_SIZE);
                cache[i].valid = 0;
                return;
            } else {
                cache[i].referenced = 0;
            }
        }
        i = (i + 1) % CACHE_SIZE;
    }
    
}

static cache_write_t * search_in_cache_mem(int fd, off_t offset) {
    for (size_t i = 0; i < CACHE_SIZE; i++)
    {
        if (cache[i].valid && cache[i].fd == fd && cache[i].offset == offset)
        {
            return &cache[i];
        }
    }
    return NULL;
}

static cache_write_t * load_into_cache_mem(int fd, off_t offset){
    for (size_t i = 0; i < CACHE_SIZE; i++)
    {
        if (!cache[i].valid)
        {
            cache[i].valid = 1;
            cache[i].fd = fd;
            cache[i].offset = offset;
            cache[i].data = malloc(BLOCK_SIZE);
            lseek(fd, offset, SEEK_SET);
            read(fd, cache[i].data, BLOCK_SIZE);
            cache[i].referenced = 1;
            return &cache[i];
        }
    }
    
    eject_page();   
    return load_into_cache_mem(fd, offset);
}

int lab2_open(const char * path){
    int fd = open(path, O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open file!");
        return -1;
    }
    return fd;
}

int lab2_close(int fd){
    for (size_t i = 0; i < CACHE_SIZE; i++)
    {
        if (cache[i].valid && cache[i].fd == fd)
        {
            lseek(fd, cache[i].offset, SEEK_SET);
            write(fd, cache[i].data, BLOCK_SIZE);
            cache[i].valid = 0;
        }
    }
    return close(fd);
}

ssize_t lab2_read(int fd, void*buf, size_t count) {
    if (count > BLOCK_SIZE)
    {
        fprintf(stderr, "Read size exceeds block size!\n");
        return -1;
    }

    off_t offset = lseek(fd, 0, SEEK_CUR);
    cache_write_t * entry = search_in_cache_mem(fd, offset);

    if (entry != NULL)
    {
        memcpy(buf, entry->data, count);
        entry->referenced = 1;
        return count;
    } else {
        entry = load_into_cache_mem(fd, offset);
        memcpy(buf, entry->data, count);
        return count;
    }
}

ssize_t lab2_write(int fd, void * buf, size_t count){
    if (count > BLOCK_SIZE)
    {
        fprintf(stderr, "Write size exceeds block size!\n");
        return -1;
    }

    off_t offset = lseek(fd, 0, SEEK_CUR);
    cache_write_t * entry = search_in_cache_mem(fd, offset);
    if (entry != NULL)
    {

        memcpy(buf, entry->data, count);
        entry->referenced = 1; // flag of link
    } else {
        entry = load_into_cache_mem(fd, offset);
        memcpy(buf, entry->data, count);
    }
    
    return count;
}

off_t lab2_lseek(int fd, off_t offset, int whence){
    return lseek(fd, offset, whence);
}

int lab2_fsync(int fd){
    for (size_t i = 0; i < CACHE_SIZE; i++)
    {
        if (cache[i].valid && cache[i].fd == fd) {
            lseek(cache[i].fd, cache[i].offset, SEEK_SET);
            write(cache[i].fd, cache[i].data, BLOCK_SIZE);
            cache[i].valid = 0; 
        }
    }
    
    return fsync(fd);
}