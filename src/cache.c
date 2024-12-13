#include "../include/cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

static cache_write_t cache[CACHE_SIZE];

static void do_write_and_set_invalid(cache_write_t * cache_page){
    if (write(cache_page->fd, cache_page->data, BLOCK_SIZE) == -1)
    {
        perror("write fail");
        exit(EXIT_FAILURE);
    } 
    cache_page->valid = 0;
}

static void eject_page(){
    int i = 0;
    cache_write_t * cache_page;
    for (size_t i = 0; ; i = (i+1) % CACHE_SIZE)
    {
        cache_page = &cache[i];
        if (cache_page->valid)
        {
            if (!cache_page->referenced)
            {
                lseek(cache_page->fd, cache_page->offset, SEEK_SET);
                do_write_and_set_invalid(cache_page);
                return;
            } else {
                cache_page->referenced = 0;
            }
        }
        i = (i + 1) % CACHE_SIZE;
    }
    
}



cache_write_t * search_in_cache_mem(int fd, off_t offset) {
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
            cache[i].referenced = 1;
            lseek(fd, offset, SEEK_SET);

            if(read(fd, cache[i].data, BLOCK_SIZE) == -1) {
                perror("errors with read()");
                exit(EXIT_FAILURE);
            }
            
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
    cache_write_t * cache_page;
    for (size_t i = 0; i < CACHE_SIZE; i++)
    {
        if (cache_page->valid &&cache_page->fd == fd)
        {
            lseek(fd, cache_page->offset, SEEK_SET);
            do_write_and_set_invalid(cache_page);
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
        write(fd, buf, count);
        entry = load_into_cache_mem(fd, offset);
        memcpy(buf, entry->data, count);
    }
    
    return count;
}

off_t lab2_lseek(int fd, off_t offset, int whence){
    return lseek(fd, offset, whence);
}

int lab2_fsync(int fd){
    cache_write_t * cache_page;
    for (size_t i = 0; i < CACHE_SIZE; i++)
    {
        if (cache_page->valid && cache_page->fd == fd) {
            lseek(cache_page->fd, cache_page->offset, SEEK_SET);
            do_write_and_set_invalid(cache_page);
        }
    }
    return fsync(fd);
}