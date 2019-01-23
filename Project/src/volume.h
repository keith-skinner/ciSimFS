//
// Created by Skinner, Brian on 1/23/19.
//

#ifndef SIMFS_VOLUME_H
#define SIMFS_VOLUME_H

#include <unistd.h>
#include <stdbool.h>
#include "block.h"

#define SIMFS_INVALID_INDEX ((SIMFS_INDEX_TYPE)0xFF)

bool initVolume(char * file);

SIMFS_INDEX_TYPE volumeAllocateBlock();
void volumeDeallocateBlock(SIMFS_INDEX_TYPE);

SIMFS_INDEX_TYPE volumeRootIndex();
int volumeNumberOfBlocks();
int volumeBlockSize();

#endif //SIMFS_VOLUME_H
