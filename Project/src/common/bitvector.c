//
// Created by Skinner, Brian on 1/23/19.
//

#include "bitvector.h"

/**
 * Finds a block index labeled free in the bitvector
 * @return the index to the free block
 */
inline SIMFS_INDEX_TYPE simfsFindFreeBlock(SIMFS_BITVECTOR_TYPE bitvector) {
    //find a byte not yet full of all 1's
    SIMFS_INDEX_TYPE i = 0;
    while (bitvector[i] == 0xFF)
        i += 1;

    //find out which bit is the first 0 bit
    register unsigned char mask = 0x80;
    SIMFS_INDEX_TYPE j = 0;
    while (bitvector[i] & mask)
        mask >>= ++j;

    //return the index
    return (SIMFS_INDEX_TYPE) ((i * 8) + j); // i bytes and j bits are all "1", so this formula points to the first "0"
}

/**
 * Flip a specific bit in the bitvector. Specifically the bitIndex
 * @param bitvector : who's getting their bit flipped?
 * @param bitIndex : which bit is getting flipped?
 */
inline void simfsFlipBit(SIMFS_BITVECTOR_TYPE bitvector, SIMFS_INDEX_TYPE index) {
    SIMFS_INDEX_TYPE blockIndex = (SIMFS_INDEX_TYPE) (index / 8);
    SIMFS_INDEX_TYPE bitShift = (SIMFS_INDEX_TYPE) (index % 8);
    register unsigned char mask = 0x80;
    bitvector[blockIndex] ^= (mask >> bitShift);
}

/**
 * Set a specific bit in the bitvector to 1.
 * @param bitvector : Who's getting their bit flipped?
 * @param bitIndex : which bit is getting flipped?
 */
inline void simfsSetBit(SIMFS_BITVECTOR_TYPE bitvector, SIMFS_INDEX_TYPE index) {
    SIMFS_INDEX_TYPE blockIndex = (SIMFS_INDEX_TYPE) (index / 8);
    SIMFS_INDEX_TYPE bitShift = (SIMFS_INDEX_TYPE) (index % 8);
    register unsigned char mask = 0x80;
    bitvector[blockIndex] |= (mask >> bitShift);
}

/**
 * Set a specific bit in the bitvector to 0.
 * @param bitvector : Who's getting their bit flipped?
 * @param bitIndex : which bit is getting flipped?
 */
inline void simfsClearBit(SIMFS_BITVECTOR_TYPE bitvector, SIMFS_INDEX_TYPE index) {
    SIMFS_INDEX_TYPE blockIndex = (SIMFS_INDEX_TYPE) (index / 8);
    SIMFS_INDEX_TYPE bitShift = (SIMFS_INDEX_TYPE) (index % 8);
    register unsigned char mask = 0x80;
    bitvector[blockIndex] &= ~(mask >> bitShift);
}
