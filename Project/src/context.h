//
// Created by Skinner, Brian on 1/23/19.
//

#ifndef SIMFS_CONTEXT_H
#define SIMFS_CONTEXT_H

#include "error.h"
#include "bitvector.h"

#define SIMFS_DIRECTORY_SIZE 4099 // 65537 // prime number for the size of the directory (it is larger than 2^16)
#define SIMFS_MAX_NUMBER_OF_OPEN_FILES 64 // 1024
#define SIMFS_MAX_NUMBER_OF_PROCESSES 64 // 1024
#define SIMFS_MAX_NUMBER_OF_OPEN_FILES_PER_PROCESS 16 // 64

//TODO: Make interaction functions w/ multithreading capabilities
//TODO: See what you can hide in the .c file.

//node for hast table (SIMFS_DIRECTORY)
typedef struct simfs_dir_ent {
    SIMFS_INDEX_TYPE nodeReference; // points to the "physical" file descriptor node
    struct simfs_dir_ent *next;
} SIMFS_DIR_ENT;

//Hash table for file system entry names
typedef SIMFS_DIR_ENT *SIMFS_DIRECTORY[SIMFS_DIRECTORY_SIZE];


// global open file table
typedef struct simfs_open_file_global_type {
    SIMFS_CONTENT_TYPE type; // folder or file
    SIMFS_INDEX_TYPE fileDescriptor; // reference to the file descriptor node
    unsigned short referenceCount; // reference count
    time_t creationTime; // creation time
    time_t lastAccessTime; // last access
    time_t lastModificationTime; // last modification
    mode_t accessRights; // access rights for the file
    uid_t owner; // owner ID
    size_t size;
} SIMFS_GLOBAL_OPEN_FILE;


// per-process open file table
typedef int SIMFS_FILE_HANDLE;

// a node for a local list of open files (per process)
typedef struct simfs_per_process_open_file_type
{
    // access rights for this process
    mode_t accessRights;
    // link to the entry for the file in the global table
    SIMFS_GLOBAL_OPEN_FILE *globalEntry;
} SIMFS_PROCESS_OPEN_FILE_NODE;

typedef SIMFS_PROCESS_OPEN_FILE_NODE SIMFS_PROCESS_OPEN_FILES[SIMFS_MAX_NUMBER_OF_OPEN_FILES_PER_PROCESS];


typedef struct simfs_process_control_block_type {
    // process identifier
    pid_t pid;
    int numberOfOpenFiles;
    // current working directory; set to the root of the volume on mounting
    SIMFS_INDEX_TYPE currentWorkingDirectory;
    SIMFS_PROCESS_OPEN_FILES openFileTable;
    struct simfs_process_control_block_type *next;
} SIMFS_PROCESS_CONTROL_BLOCK;

// file system context
typedef SIMFS_GLOBAL_OPEN_FILE SIMFS_GLOBAL_OPEN_FILE_TABLE[SIMFS_MAX_NUMBER_OF_OPEN_FILES];
typedef struct simfs_context_type {
    // the hashtable-based in-memory directory
    SIMFS_DIRECTORY directory;

    // an "in-memory" copy of the bitvector of the simulated volume
    SIMFS_BITVECTOR_TYPE bitvector;

    SIMFS_GLOBAL_OPEN_FILE_TABLE globalOpenFileTable[SIMFS_MAX_NUMBER_OF_OPEN_FILES];
    SIMFS_PROCESS_CONTROL_BLOCK *processControlBlocks;
} SIMFS_CONTEXT_TYPE;

#endif //SIMFS_CONTEXT_H
