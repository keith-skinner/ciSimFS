//
// Created by Skinner, Brian on 1/23/19.
//

#ifndef SIMFS_BLOCK_H
#define SIMFS_BLOCK_H

#include <unistd.h>

#ifdef SIMFS_LIGHT
#define SIMFS_BLOCK_SIZE 16
#define SIMFS_DATA_SIZE (SIMFS_BLOCK_SIZE - sizeof(SIMFS_NODE_TYPE))
#define SIMFS_INDEX_SIZE 7 //3 bits => x0 - x7
#define SIMFS_MAX_NAME_LENGTH 64
#else //SIMFS_LIGHT
#define SIMFS_BLOCK_SIZE 256
#define SIMFS_DATA_SIZE (SIMFS_BLOCK_SIZE - sizeof(SIMFS_NODE_TYPE))
#define SIMFS_INDEX_SIZE 127 // two bytes => x0000 - xFFFF => 2^16 range
#define SIMFS_MAX_NAME_LENGTH 128
#endif //SIMFS_LIGHT

typedef enum {
    FOLDER_CONTENT_TYPE,
    FILE_CONTENT_TYPE,
    INDEX_CONTENT_TYPE,
    DATA_CONTENT_TYPE,
    INVALID_CONTENT_TYPE
} SIMFS_CONTENT_TYPE;

typedef char SIMFS_NAME_TYPE[SIMFS_MAX_NAME_LENGTH]; // for folder and file names
typedef char SIMFS_DATA_TYPE[SIMFS_DATA_SIZE];
typedef unsigned short SIMFS_INDEX_TYPE; // is used to index blocks in the file system

typedef struct simfs_file_descriptor_type {
    SIMFS_CONTENT_TYPE type; // folder or file
    SIMFS_NAME_TYPE name;
    time_t creationTime; // creation time
    time_t lastAccessTime; // last access
    time_t lastModificationTime; // last modification
    mode_t accessRights; // access rights for the file
    uid_t owner; // owner ID
    size_t size; // capacity limited for this project to 2s^16
    SIMFS_INDEX_TYPE block_ref; // reference to the data or index block
} SIMFS_FILE_DESCRIPTOR_TYPE;

typedef struct simfs_node_type {
    SIMFS_CONTENT_TYPE type;
    union { // content depends on the type
        SIMFS_FILE_DESCRIPTOR_TYPE fileDescriptor; // for directories and files
        SIMFS_DATA_TYPE data; // for data
        SIMFS_INDEX_TYPE index[SIMFS_INDEX_SIZE];  // for indices; all indices but the last point to data blocks
        // the last points to another index block
    } content;
} SIMFS_BLOCK_TYPE;

#endif //SIMFS_BLOCK_H
