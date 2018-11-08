#include "simfs.h"

#include <stdio.h>

int main() {

//    srand(time(NULL)); // uncomment to get true random values in get_context()

    SIMFS_VOLUME *simfs_volume = malloc(sizeof(SIMFS_VOLUME));

    simfsMountFileSystem(simfs_volume);

    // TODO: implement thorough testing of all the functionality

    // the following is just some sample code for simulating user and process identifiers that are
    // needed in the simfs functions
    int count = 10;
    char *content;
    struct fuse_context *context;
    for (int i = 0; i < count; i++)
    {
        context = simfs_debug_get_context();
        printf("user ID = %02i, process ID = %02i, group ID = %02i, umask = %04o\n",
               context->uid, context->pid, context->gid, context->umask);
        content = simfsGenerateContent(i * 10);
        printf("\"content = %s\"\n", content);
    }

    return EXIT_SUCCESS;
}
