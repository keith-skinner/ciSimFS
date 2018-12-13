#ifndef __SIMFS_H_
#define __SIMFS_H_

#include <fuse.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
//////////////////////////////////////////////////////////////////////////
//
// defines for the volume
//
//////////////////////////////////////////////////////////////////////////

#define SIMFS_BLOCK_SIZE 16 // 256
#define SIMFS_NUMBER_OF_BLOCKS 4096 // 65536 // 2^16
#define SIMFS_MAX_NAME_LENGTH 64 // 128
#define SIMFS_DATA_SIZE 14 // 254 // SIMFS_BLOCK_SIZE - sizeof(SIMFS_NODE_TYPE)
#define SIMFS_INDEX_SIZE 7 // 127 // two bytes => x0000 - xFFFF => 2^16 range

//////////////////////////////////////////////////////////////////////////
//
// defines for the in-memory data structures
//
//////////////////////////////////////////////////////////////////////////

#define SIMFS_DIRECTORY_SIZE 4099 // 65537 // prime number for the size of the directory (it is larger than 2^16)
#define SIMFS_MAX_NUMBER_OF_OPEN_FILES 64 // 1024
#define SIMFS_MAX_NUMBER_OF_PROCESSES 64 // 1024
#define SIMFS_MAX_NUMBER_OF_OPEN_FILES_PER_PROCESS 16 // 64

//////////////////////////////////////////////////////////////////////////
//
// data structures for "physical" file system
//
//////////////////////////////////////////////////////////////////////////

typedef enum {
    FOLDER_CONTENT_TYPE,
    FILE_CONTENT_TYPE,
    INDEX_CONTENT_TYPE,
    DATA_CONTENT_TYPE,
    INVALID_CONTENT_TYPE
} SIMFS_CONTENT_TYPE;

typedef unsigned short SIMFS_INDEX_TYPE; // is used to index blocks in the file system
#define SIMFS_INVALID_INDEX ((SIMFS_INDEX_TYPE)0xFF)

//
// superblock starting block in the whole file system
//
// rootNodeIndex points to the block which is the root folder of the files system
// numberOfBlock determines the size of the file system
// blockSize is the size of a single block of the file system
//
typedef union simfs_superblock_type { // size of the block with some unused part
    char spacer_dummy[SIMFS_BLOCK_SIZE]; // this makes the struct exactly one block
    struct attr {
        SIMFS_INDEX_TYPE rootNodeIndex; // should point to the first block after the last bitvector block
        int numberOfBlocks;
        int blockSize;
    } attr;
} SIMFS_SUPERBLOCK_TYPE;

//
// file descriptor node for blocks holding folder or file information
//
//   for files:
//       te size indicates the size of the file
//       the block reference is initialized to SIMFS_INVALID_INDEX
//           - it will point to an index block when the file has content
//
//   for directories:
//       the size indicates the number of files or directories in this folder
//       the block reference points to an index block that holds references to the file and folder blocks
//
typedef char SIMFS_NAME_TYPE[SIMFS_MAX_NAME_LENGTH]; // for folder and file names

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

//
// a block for holding data
//
typedef char SIMFS_DATA_TYPE[SIMFS_DATA_SIZE];

//
// various interpretations of a file system block
//
typedef struct simfs_node_type {
    SIMFS_CONTENT_TYPE type;
    union { // content depends on the type
        SIMFS_FILE_DESCRIPTOR_TYPE fileDescriptor; // for directories and files
        SIMFS_DATA_TYPE data; // for data
        SIMFS_INDEX_TYPE index[SIMFS_INDEX_SIZE];  // for indices; all indices but the last point to data blocks
        // the last points to another index block
    } content;
} SIMFS_BLOCK_TYPE;

//
// "physical" file system structure
//
// superblock - one block
//
// bitvector - one bit per block ( (SIMFS_NUMBER_OF_BLOCKS/8 / SIMFS_BLOCK_SIZE) blocks )
//
// blocks (folder, file, data, or index) - SIMFS_NUMBER_OF_BLOCKS
//
//
typedef struct simfs_volume {
    SIMFS_SUPERBLOCK_TYPE superblock;
    unsigned char bitvector[SIMFS_NUMBER_OF_BLOCKS / 8]; //
    SIMFS_BLOCK_TYPE block[SIMFS_NUMBER_OF_BLOCKS];
} SIMFS_VOLUME;

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
} SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE;


// per-process open file table
typedef int SIMFS_FILE_HANDLE_TYPE;
// a node for a local list of open files (per process)
typedef struct simfs_per_process_open_file_type
{
    // access rights for this process
    mode_t accessRights;
    // link to the entry for the file in the global table
    SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE *globalEntry;
} SIMFS_PER_PROCESS_OPEN_FILE_TYPE;

typedef struct simfs_process_control_block_type {
    // process identifier
    pid_t pid;
    int numberOfOpenFiles;
    // current working directory; set to the root of the volume on mounting
    SIMFS_INDEX_TYPE currentWorkingDirectory;
    SIMFS_PER_PROCESS_OPEN_FILE_TYPE openFileTable[SIMFS_MAX_NUMBER_OF_OPEN_FILES_PER_PROCESS];
    struct simfs_process_control_block_type *next;
} SIMFS_PROCESS_CONTROL_BLOCK_TYPE;

// file system context
typedef struct simfs_context_type {
    // the hashtable-based in-memory directory
    SIMFS_DIRECTORY directory;
    // an in-memory copy of the bitvector of the simulated volume
    unsigned char bitvector[SIMFS_NUMBER_OF_BLOCKS / 8];
    SIMFS_OPEN_FILE_GLOBAL_TABLE_TYPE globalOpenFileTable[SIMFS_MAX_NUMBER_OF_OPEN_FILES];
    SIMFS_PROCESS_CONTROL_BLOCK_TYPE *processControlBlocks;
} SIMFS_CONTEXT_TYPE;

//////////////////////////////////////////////////////////////////////////
//
// file system function declarations
//
//////////////////////////////////////////////////////////////////////////

typedef enum simfs_error {
    SIMFS_NO_ERROR,
    SIMFS_ALLOC_ERROR,
    SIMFS_DUPLICATE_ERROR,
    SIMFS_NOT_FOUND_ERROR,
    SIMFS_NOT_EMPTY_ERROR,
    SIMFS_ACCESS_ERROR,
    SIMFS_WRITE_ERROR,
    SIMFS_READ_ERROR
} SIMFS_ERROR;


SIMFS_ERROR simfsMountFileSystem(char *simfsFileName);
SIMFS_ERROR simfsUmountFileSystem(char *simfsFileName);

SIMFS_ERROR simfsCreateFileSystem(char *simfsFileName);
SIMFS_ERROR simfsCreateFile(SIMFS_NAME_TYPE fileName, SIMFS_CONTENT_TYPE type);
SIMFS_ERROR simfsDeleteFile(SIMFS_NAME_TYPE fileName);
SIMFS_ERROR simfsGetFileInfo(SIMFS_NAME_TYPE fileName, SIMFS_FILE_DESCRIPTOR_TYPE *infoBuffer);

SIMFS_ERROR simfsOpenFile(SIMFS_NAME_TYPE fileName, SIMFS_FILE_HANDLE_TYPE *fileHandle);
SIMFS_ERROR simfsCloseFile(SIMFS_FILE_HANDLE_TYPE fileHandle);

SIMFS_ERROR simfsWriteFile(SIMFS_FILE_HANDLE_TYPE fileHandle, char *writeBuffer);
SIMFS_ERROR simfsReadFile(SIMFS_FILE_HANDLE_TYPE fileHandle, char **readBuffer);

// ... other functions already in there
unsigned long hash(char *str); //done - given
void simfsFlipBit(unsigned char *bitvector, unsigned short bitIndex);//done - given
void simfsSetBit(unsigned char *bitvector, unsigned short bitIndex); //done - given
void simfsClearBit(unsigned char *bitvector, unsigned short bitIndex); //done - given
unsigned short simfsFindFreeBlock(unsigned char *bitvector); //done - given

/*
 * The following functions can be used to simulate FUSE context's user and 
 * process identifiers for testing. These identifiers are obtainable by 
 * calling fuse_get_context() the fuse library.
 */
// follows FUSE naming convention
struct fuse_context *simfs_debug_get_context();
char *simfsGenerateContent(int size);

#endif
