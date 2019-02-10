//
// Created by Skinner, Brian on 1/23/19.
//

#ifndef SIMFS_BITVECTOR_H
#define SIMFS_BITVECTOR_H

#include "block.h"

#ifdef SIMFS_LIGHT
#   define SIMFS_NUMBER_OF_BLOCKS 4096 // 2^12
#else //SIMFS_LIGHT
#   define SIMFS_NUMBER_OF_BLOCKS 65536 // 2^16
#endif //SIMFS_LIGHT
#define SIMFS_BITS_IN_VECTOR (SIMFS_NUMBER_OF_BLOCKS / 8)


typedef unsigned char SIMFS_BITVECTOR_TYPE[SIMFS_BITS_IN_VECTOR];
typedef SIMFS_BLOCK_TYPE SIMFS_BLOCKS_TYPE[SIMFS_NUMBER_OF_BLOCKS];

void simfsFlipBit(SIMFS_BITVECTOR_TYPE bitvector, SIMFS_INDEX_TYPE index);
void simfsSetBit(SIMFS_BITVECTOR_TYPE bitvector, SIMFS_INDEX_TYPE index);
void simfsClearBit(SIMFS_BITVECTOR_TYPE bitvector, SIMFS_INDEX_TYPE index);
SIMFS_INDEX_TYPE simfsFindFreeBlock(SIMFS_BITVECTOR_TYPE bitvector);

#endif //SIMFS_BITVECTOR_H
