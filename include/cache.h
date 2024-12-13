#ifndef CACHE_H

#define CACHE_H

#include <stddef.h>
#include <sys/types.h>

#define CACHE_SIZE 10
#define BLOCK_SIZE 4096

typedef struct cache_write1{
    int fd1;
    int valid1;
    int referenced1; // second chance
    off_t offset1;
    void* data1;
} cache_write_t1;

int lab1_open(const char * path);
int lab1_close(int fd);
ssize_t lab1_read(int fd, void* buf, size_t count);
ssize_t lab1_write(int fd,  void * buf, size_t count);
off_t lab1_lseek(int fd, off_t offset, int whence);
int lab1_fsync(int fd);
cache_write_t * search_in_cache_mem1(int fd, off_t offset);

#endif