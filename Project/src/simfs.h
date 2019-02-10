/*
 * Keith Skinner
 * Lab: Project
 * Date: 12/13/2018
 */

#ifndef __SIMFS_H_
#define __SIMFS_H_

#include <fuse.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#include "volume/volume.h"
#include "common/block.h"
#include "context/context.h"

//  file descriptor node for blocks holding folder or file information
//  for files:
//      the size indicates the size of the file
//      the block reference is initialized to SIMFS_INVALID_INDEX
//          - it will point to an index block when the file has content
//  for directories:
//      the size indicates the number of files or directories in this folder
//      the block reference points to an index block that holds references to the file and folder blocks

//  "physical" file system structure
//  superblock - one block
//  bitvector - one bit per block ( (SIMFS_NUMBER_OF_BLOCKS/8 / SIMFS_BLOCK_SIZE) blocks )
//  blocks (folder, file, data, or index) - SIMFS_NUMBER_OF_BLOCKS

//////////////////////////////////////////
//   file system function declarations  //
//////////////////////////////////////////

SIMFS_ERROR simfsMountFileSystem(char *simfsFileName);
SIMFS_ERROR simfsUmountFileSystem(char *simfsFileName);
SIMFS_ERROR simfsCreateFileSystem(char *simfsFileName);
SIMFS_ERROR simfsGetFileInfo(SIMFS_NAME_TYPE fileName, SIMFS_FILE_DESCRIPTOR_TYPE *infoBuffer);

SIMFS_ERROR simfsCreateFile(SIMFS_NAME_TYPE fileName, SIMFS_CONTENT_TYPE type);
SIMFS_ERROR simfsDeleteFile(SIMFS_NAME_TYPE fileName);

SIMFS_ERROR simfsOpenFile(SIMFS_NAME_TYPE fileName, SIMFS_FILE_HANDLE_TYPE *fileHandle);
SIMFS_ERROR simfsCloseFile(SIMFS_FILE_HANDLE_TYPE fileHandle);
SIMFS_ERROR simfsWriteFile(SIMFS_FILE_HANDLE_TYPE fileHandle, char *writeBuffer);
SIMFS_ERROR simfsReadFile(SIMFS_FILE_HANDLE_TYPE fileHandle, char **readBuffer);

/*
 * The following functions can be used to simulate FUSE context's user and 
 * process identifiers for testing. These identifiers are obtainable by 
 * calling fuse_get_context() the fuse library.
 */
// follows FUSE naming convention

struct fuse_context *simfs_debug_get_context();
char *simfsGenerateContent(int size);

#endif
