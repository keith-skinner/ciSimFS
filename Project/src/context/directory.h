//
// Created by Skinner, Brian on 1/28/19.
//

#ifndef SIMFS_DIRECTORY_H
#define SIMFS_DIRECTORY_H

#include "../common/block.h"




//node for hash table (SIMFS_DIRECTORY)
typedef struct simfs_dir_ent {
    SIMFS_INDEX_TYPE nodeReference; // points to the "physical" file descriptor node
    struct simfs_dir_ent *next;
} SIMFS_DIR_ENT;
typedef SIMFS_DIR_ENT *SIMFS_DIRECTORY[SIMFS_DIRECTORY_SIZE];

#endif //SIMFS_DIRECTORY_H
