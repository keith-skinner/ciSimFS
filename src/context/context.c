//
// Created by Skinner, Brian on 1/23/19.
//

#include "context.h"

/**
 * @param str : the value to hash
 * @return : A hash value within the limits of the directory
 */
inline unsigned long hash(char *str) {
    register unsigned long hash = 5381;
    register unsigned char c;
    while ((c = (unsigned char) (*str++)) != '\0')
        hash = ((hash << 5) + hash) ^ c; /* hash * 33 + c */
    return hash % SIMFS_DIRECTORY_SIZE;
}

