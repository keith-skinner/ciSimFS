//
// Created by Skinner, Brian on 1/23/19.
//

#ifndef SIMFS_VOLUME_H
#define SIMFS_VOLUME_H

#include <unistd.h>
#include <stdbool.h>
#include "../common/block.h"

#define SIMFS_INVALID_INDEX ((SIMFS_INDEX_TYPE)0xFF)

/**
 * Setup the volume. If file is not provided, a new volume will be created.
 * @param file : the file to attempt to read from
 * @return : whether the volume init was successful or not
 */
bool initVolume(char * file);
bool releaseVolume(char * file);

SIMFS_INDEX_TYPE volumeAllocateBlock();
void volumeDeallocateBlock(SIMFS_INDEX_TYPE);

SIMFS_INDEX_TYPE volumeRootIndex();
int volumeNumberOfBlocks();
int volumeBlockSize();

#endif //SIMFS_VOLUME_H
