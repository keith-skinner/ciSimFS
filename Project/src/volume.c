//
// Created by Skinner, Brian on 1/23/19.
//

#include "volume.h"
#include "bitvector.h"

#include <stdio.h>

//
// superblock is the starting block for the whole file system
// - rootNodeIndex points to the block which is the root folder of the files system
// - numberOfBlock determines the size of the file system
// - blockSize is the size of a single block of the file system
//
typedef union simfs_superblock_type {
    char spacer_dummy[SIMFS_BLOCK_SIZE]; // this makes the struct exactly one block
    struct attr {
        SIMFS_INDEX_TYPE rootNodeIndex;
        int numberOfBlocks;
        int blockSize;
    } attr;
} SIMFS_SUPERBLOCK_TYPE;

typedef struct simfs_volume {
    SIMFS_SUPERBLOCK_TYPE superblock;
    SIMFS_BITVECTOR_TYPE bitvector;
    SIMFS_BLOCK_TYPE block[SIMFS_NUMBER_OF_BLOCKS];
} SIMFS_VOLUME;

SIMFS_VOLUME * simfs_volume = NULL;

bool initVolume(char * file)
{
    fopen(file, "rb");

}

// Todo: Make this multithread compatible
// Todo: Add error handling if simfs_volume is NULL
SIMFS_INDEX_TYPE volumeAllocateBlock()
{
    return simfsFindFreeBlock(simfs_volume->bitvector);
}

// Todo: Make this multithread compatible
// Todo: Add error handling if simfs_volume is NULL
void volumeDeallocateBlock(SIMFS_INDEX_TYPE index)
{
    simfsClearBit(simfs_volume->bitvector, index);
}

// Todo: Add error handling if simfs_volume is NULL
SIMFS_INDEX_TYPE volumeRootIndex()
{
    return simfs_volume->superblock.attr.rootNodeIndex;
}

// Todo: Add error handling if simfs_volume is NULL
int volumeNumberOfBlocks()
{
    return simfs_volume->superblock.attr.numberOfBlocks;
}

// Todo: Add error handling if simfs_volume is NULL
int volumeBlockSize()
{
    return simfs_volume->superblock.attr.blockSize;
}
