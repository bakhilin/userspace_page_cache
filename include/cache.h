#ifndef CACHE_H

#define CACHE_H

#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>

#define CACHE_SIZE 10
#define BLOCK_SIZE 4096

typedef struct  cache_write{
    ino_t inode;
    int  fd;
    bool  valid;
    int  referenced; // second chance
    off_t  offset;
    void*  data;
}  cache_write_t;

int  lab2_open(const char * path);
int  lab2_close(int fd);
ssize_t  lab2_read(int fd, void* buf, size_t count);
ssize_t  lab2_write(int fd,  void * buf, size_t count);
off_t  lab2_lseek(int fd, off_t offset, int whence);
int  lab2_fsync(int fd);
cache_write_t *  search_in_cache_mem(ino_t inode, off_t offset);

#endif