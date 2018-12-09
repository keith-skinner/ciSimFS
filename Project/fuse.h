#ifndef FUSE_H
#define FUSE_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t uid_t;
typedef uint32_t gid_t;

mode_t getuid() {
    return 0;
}

struct fuse_context 
{
    struct fuse * fuse;
    uid_t uid;
    gid_t gid;
    pid_t pid;
    void *private_data;
    mode_t umask;
};

#endif //FUSE_H