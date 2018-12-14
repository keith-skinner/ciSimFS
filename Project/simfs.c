/*
 * Keith Skinner
 * Lab: Project
 * Date: 12/13/2018
 */
#include "simfs.h"
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

void simfsCreateFileTime(SIMFS_BLOCK_TYPE *block);

void mountDirectory(SIMFS_INDEX_TYPE index);

void simfsDeallocateBlock(SIMFS_INDEX_TYPE index);

SIMFS_INDEX_TYPE simfsAllocateBlock();

// all in-memory information about the system
SIMFS_CONTEXT_TYPE *simfsContext;
SIMFS_VOLUME *simfsVolume;


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

/**
 * Finds a free block and sets that bit to being used in the bitvector
 * @return the index to the new block
 */
inline SIMFS_INDEX_TYPE simfsAllocateBlock() {
    SIMFS_INDEX_TYPE index = simfsFindFreeBlock(simfsContext->bitvector);
    simfsSetBit(simfsContext->bitvector, index);
    simfsSetBit(simfsVolume->bitvector, index);
    return index;
}

/**
 * flips the index bit in the volume and context
 * @param index : the bit to be flipped
 */
inline void simfsDeallocateBlock(SIMFS_INDEX_TYPE index) {
    simfsClearBit(simfsContext->bitvector, index);
    simfsClearBit(simfsVolume->bitvector, index);
}

/**
 * Finds a block index labeled free in the bitvector
 * @return the index to the free block
 */
inline SIMFS_INDEX_TYPE simfsFindFreeBlock(unsigned char *bitvector) {
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
inline void simfsFlipBit(unsigned char *bitvector, SIMFS_INDEX_TYPE bitIndex) {
    SIMFS_INDEX_TYPE blockIndex = (SIMFS_INDEX_TYPE) (bitIndex / 8);
    SIMFS_INDEX_TYPE bitShift = (SIMFS_INDEX_TYPE) (bitIndex % 8);
    register unsigned char mask = 0x80;
    bitvector[blockIndex] ^= (mask >> bitShift);
}

/**
 * Set a specific bit in the bitvector to 1.
 * @param bitvector : Who's getting their bit flipped?
 * @param bitIndex : which bit is getting flipped?
 */
inline void simfsSetBit(unsigned char *bitvector, SIMFS_INDEX_TYPE bitIndex) {
    SIMFS_INDEX_TYPE blockIndex = (SIMFS_INDEX_TYPE) (bitIndex / 8);
    SIMFS_INDEX_TYPE bitShift = (SIMFS_INDEX_TYPE) (bitIndex % 8);
    register unsigned char mask = 0x80;
    bitvector[blockIndex] |= (mask >> bitShift);
}

/**
 * Set a specific bit in the bitvector to 0.
 * @param bitvector : Who's getting their bit flipped?
 * @param bitIndex : which bit is getting flipped?
 */
inline void simfsClearBit(unsigned char *bitvector, SIMFS_INDEX_TYPE bitIndex) {
    SIMFS_INDEX_TYPE blockIndex = (SIMFS_INDEX_TYPE) (bitIndex / 8);
    SIMFS_INDEX_TYPE bitShift = (SIMFS_INDEX_TYPE) (bitIndex % 8);
    register unsigned char mask = 0x80;
    bitvector[blockIndex] &= ~(mask >> bitShift);
}


/**
 * Set the creationTime, lastAccessTime and lastModificationTime of a file to now.
 * @param block : reference to the block that holds the the file descriptor to the newly created file
 */
inline void simfsCreateFileTime(SIMFS_BLOCK_TYPE *block) {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    block->content.fileDescriptor.creationTime = time.tv_sec;
    block->content.fileDescriptor.lastAccessTime = time.tv_sec;
    block->content.fileDescriptor.lastModificationTime = time.tv_sec;
}

/**
 * Initiate the superblock for first time use in the simfsVolume
 */
void initSuperBlock() {
    simfsVolume->superblock.attr.rootNodeIndex = 0;
    simfsVolume->superblock.attr.blockSize = SIMFS_BLOCK_SIZE;
    simfsVolume->superblock.attr.numberOfBlocks = SIMFS_NUMBER_OF_BLOCKS;
    simfsFlipBit(simfsVolume->bitvector, simfsVolume->superblock.attr.rootNodeIndex);
}

/**
 * Create the index block for the root folder
 */
void initRootFolderIndex() {
    simfsFlipBit(simfsVolume->bitvector, 1);
    simfsVolume->block[0].content.fileDescriptor.block_ref = 1;
    simfsVolume->block[1].type = INDEX_CONTENT_TYPE;
}


/**
 * Create the filedescriptor for the root directory
 */
void initRootFolder() {
    SIMFS_BLOCK_TYPE *block = &simfsVolume->block[0];
    block->type = FOLDER_CONTENT_TYPE;
    block->content.fileDescriptor.type = FOLDER_CONTENT_TYPE;
    strcpy(block->content.fileDescriptor.name, "/");
    block->content.fileDescriptor.accessRights = (mode_t) umask(00000);
    block->content.fileDescriptor.owner = 0;
    simfsCreateFileTime(0);

    block->content.fileDescriptor.size = 0;
    initRootFolderIndex();
}

/**
 * read the contents of a file into simfsVolume
 * @param fileName : file to use as the source
 * @return : SIMFS_NO_ERROR if successful, SIMFS_ALLOC_ERROR if not
 */
SIMFS_ERROR readVolumeFromFile(char *fileName) {
    FILE *file = fopen(fileName, "rb");
    if (file == NULL)
        return SIMFS_ALLOC_ERROR;
    fread(simfsVolume, 1, sizeof(SIMFS_VOLUME), file);
    fclose(file);
    return SIMFS_NO_ERROR;
}

/**
 * write the contents of the volume to the file
 * @param fileName : file to write contents to
 * @return : SIMFS_NO_ERROR if successful, SIMFS_ALLOC_ERROR if not
 */
SIMFS_ERROR writeVolumeToFile(char *fileName) {
    FILE *file = fopen(fileName, "wb");
    if (file == NULL)
        return SIMFS_ALLOC_ERROR;
    fwrite(simfsVolume, 1, sizeof(SIMFS_VOLUME), file);
    fclose(file);
    return SIMFS_NO_ERROR;
}

/**
 * Malloc the simfs Volume and then read the data from the file into the volume
 * @param fileName : the name of the file what holds
 * @return
 */
SIMFS_ERROR initVolume(char *fileName) {
    simfsVolume = malloc(sizeof(SIMFS_VOLUME));
    if (simfsVolume == NULL)
        return SIMFS_ALLOC_ERROR;
    if (fileName != NULL)
        if (readVolumeFromFile(fileName) != SIMFS_NO_ERROR)
            return SIMFS_ALLOC_ERROR;
    return SIMFS_NO_ERROR;
}


/**
 * Adds a single directory entry to the hash table
 * @param index : index to the file descriptor block to add to the hash table
 */
void addDirectory(SIMFS_INDEX_TYPE index) {
    unsigned long dir_index = hash(simfsVolume->block[index].content.fileDescriptor.name);
    SIMFS_DIR_ENT *entry = malloc(sizeof(SIMFS_DIR_ENT));
    entry->next = simfsContext->directory[dir_index];
    entry->nodeReference = index;
    simfsContext->directory[dir_index] = entry;
}


/**
 * calls mountDirectory on all directories in this index block
 * @param size
 * @param block
 */
void mountSubDirectory(SIMFS_BLOCK_TYPE *block, size_t size) {
    const size_t index_block_size = SIMFS_INDEX_SIZE - 1;
    for (size_t index_block = 0; index_block < (size / index_block_size); ++index_block) {
        for (size_t index = 0; index < index_block_size; ++index)
            mountDirectory(block->content.index[index]);
        block = &(simfsVolume->block[index_block_size]);
    }
    for (size_t index = 0; index < (size % index_block_size); ++index)
        mountDirectory(block->content.index[index]);
}

/**
 * Adds this and all sub folders and files to the hash table (directory)
 * @param index : the index to the file descriptor block for this folder
 */
void mountDirectory(SIMFS_INDEX_TYPE index) {
    //adds this entry to the hash table
    addDirectory(index);
    //stop recursion if file, not folder
    if (simfsVolume->block[index].type == FILE_CONTENT_TYPE)
        return;
    //add all sub files and folders to hash table
    mountSubDirectory(
            &(simfsVolume->block[simfsVolume->block[index].content.fileDescriptor.block_ref]),
            simfsVolume->block[index].content.fileDescriptor.size);
}

/**
 * Allocate simfsContext and copy the bitvector from simfsVolume into simfsVolume
 * Then create the
 * @return : SIMFS_ALLOC_ERROR if successful, SIMFS_NO_ERROR if not
 */
SIMFS_ERROR initContext() {
    simfsContext = calloc(1, sizeof(SIMFS_CONTEXT_TYPE));
    if (simfsContext == NULL)
        return SIMFS_ALLOC_ERROR;
    memcpy(simfsContext->bitvector, simfsVolume->bitvector, SIMFS_NUMBER_OF_BLOCKS / 8);
    mountDirectory(simfsVolume->superblock.attr.rootNodeIndex);
    return SIMFS_NO_ERROR;
}

/**
 * Free all currently allocated process control blocks
 * This should only be called if releasing the Context
 */
void freeProcessControlBlocks() {
    SIMFS_PROCESS_CONTROL_BLOCK_TYPE *iter = simfsContext->processControlBlocks;
    while (iter != NULL) {
        SIMFS_PROCESS_CONTROL_BLOCK_TYPE *temp = iter;
        iter = iter->next;
        free(temp);
    }
}

/**
 * Writes the contents of the volume to the file and frees the volume
 * @param fileName : file to write the contents of releaseVolume
 */
void releaseVolume(char *fileName) {
    writeVolumeToFile(fileName);
    free(simfsVolume);
    simfsVolume = NULL;
}


/**
 * Frees the context. Used in unmounting the filesystem
 */
void releaseContext() {
    freeProcessControlBlocks();
    free(simfsContext);
    simfsContext = NULL;
}

/**
 * Allocates space for the file system and saves it to disk.
 */
SIMFS_ERROR simfsCreateFileSystem(char *simfsFileName) {
    if (initVolume(NULL) != SIMFS_NO_ERROR)
        return SIMFS_ALLOC_ERROR;
    initSuperBlock();
    initRootFolder();
    releaseVolume(simfsFileName);
    return SIMFS_NO_ERROR;
}


/**
 * Sets up simfsVolume with the file provided, then mounts the file system by
 * adding all folders and files in simfsVolume to the hashtable (directory).
 * @param simfsFileName : Serialized simfsVolume file.
 * @return : SIMFS_NO_ERROR if successful, some error if not.
 */
SIMFS_ERROR simfsMountFileSystem(char *simfsFileName) {
    //TODO: implement - DONE
    SIMFS_ERROR error = initVolume(simfsFileName);
    if (error != SIMFS_NO_ERROR)
        return error;
    error = initContext();
    if (error != SIMFS_NO_ERROR)
        return error;
    return SIMFS_NO_ERROR;
}

/**
 * Saves the file system to a disk and de-allocates the memory.
 * Assumes that all synchronization has been done.
 */
SIMFS_ERROR simfsUmountFileSystem(char *simfsFileName) {
    //TODO: implement - DONE
    releaseVolume(simfsFileName);
    releaseContext();
    return SIMFS_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////

/**
 * Finds the control block in the list of control blocks for that process id
 * @param controlBlocks : list of control blocks
 * @param pid : process id to look for
 * @return : reference to the control block for that process if successful, NULL if not.
 */
SIMFS_PROCESS_CONTROL_BLOCK_TYPE *findControlBlock(SIMFS_PROCESS_CONTROL_BLOCK_TYPE *controlBlocks, pid_t pid) {
    while (controlBlocks != NULL) {
        if (controlBlocks->pid == pid)
            break;
        controlBlocks = controlBlocks->next;
    }
    return controlBlocks;
}

/**
 * hashes the file name and returns the start of the linked list associated with that
 * directory entry of the hash table
 * @param fileName : name of the file to find
 * @return : head of linked list to all entries that hashed to that position
 */
SIMFS_DIR_ENT *getDirectoryByFileName(SIMFS_NAME_TYPE fileName) {
    unsigned long pos = hash(fileName);
    SIMFS_DIR_ENT *entry = simfsContext->directory[pos];
    return entry;
}

/**
 * Finds a file based on its file name and returns the block number
 * @param fileName : file to find in the hashtable
 * @return : block number of the fileName if successful, SIMFS_INVALID_INDEX if not
 */
SIMFS_INDEX_TYPE findFile(SIMFS_NAME_TYPE fileName) {
    SIMFS_DIR_ENT *entry = getDirectoryByFileName(fileName);
    SIMFS_BLOCK_TYPE *block = &simfsVolume->block[entry->nodeReference];
    while (entry != NULL) {
        if (strcmp(fileName, block->content.fileDescriptor.name) == 0)
            return entry->nodeReference;
        entry = entry->next;
    }
    return SIMFS_INVALID_INDEX;
}

/**
 * Creates a default block for a new file
 * @param name : name of the new file
 * @param context : current context
 * @param type : folder or file
 * @return : a new file descriptor block
 */
SIMFS_BLOCK_TYPE newFileBlock(SIMFS_NAME_TYPE name, struct fuse_context *context, SIMFS_CONTENT_TYPE type) {
    SIMFS_BLOCK_TYPE block = {0};
    block.type = type;
    block.content.fileDescriptor.type = type;
    block.content.fileDescriptor.size = 0;
    block.content.fileDescriptor.owner = context->uid;
    block.content.fileDescriptor.accessRights = context->umask;
    strcpy(block.content.fileDescriptor.name, name);
    simfsCreateFileTime(&block);
    block.content.fileDescriptor.block_ref =
            (block.type == FOLDER_CONTENT_TYPE) ? simfsAllocateBlock() : SIMFS_INVALID_INDEX;
    return block;
}

SIMFS_INDEX_TYPE getCurrentWorkingDirectory(struct fuse_context *context){
    SIMFS_PROCESS_CONTROL_BLOCK_TYPE *controlBlock =
            findControlBlock(simfsContext->processControlBlocks, context->pid);
    SIMFS_INDEX_TYPE index = simfsVolume->superblock.attr.rootNodeIndex;
    if (controlBlock != NULL)
        index = controlBlock->currentWorkingDirectory;
    return index;
}

void addFileToDirectory(SIMFS_INDEX_TYPE directory, SIMFS_INDEX_TYPE file) {
    size_t size = simfsVolume->block[directory].content.fileDescriptor.size;
    SIMFS_INDEX_TYPE index = simfsVolume->block[directory].content.fileDescriptor.block_ref;
    for (int i = 0; i < size / (SIMFS_INDEX_SIZE - 1); ++i)
        index = simfsVolume->block[index].content.index[SIMFS_INDEX_SIZE - 1];
    if (size % (SIMFS_INDEX_SIZE - 1) == 0) {
        simfsVolume->block[index].content.index[SIMFS_INDEX_SIZE - 1] = simfsAllocateBlock();
        index = simfsVolume->block[index].content.index[SIMFS_INDEX_SIZE - 1];
        simfsVolume->block[index].type = INDEX_CONTENT_TYPE;
    }
    simfsVolume->block[index].content.index[size % (SIMFS_INDEX_SIZE - 1)] = file;
}

/**
 * Depending on the type parameter the function creates a file or a folder in the current directory
 * of the process. If the process does not have an entry in the processControlBlock, then the root directory
 * is assumed to be its current working directory.
 *
 * Hashes the file name and check if the file with such name already exists in the in-memory directory.
 * If it is then it return SIMFS_DUPLICATE_ERROR.
 * Otherwise:
 *    - finds an available block in the storage using the in-memory bitvector and flips the bit to indicate
 *      that the block is taken
 *    - initializes a local buffer for the file descriptor block with the block type depending on the parameter type
 *      (i.e., folder or file)
 *    - creates an entry in the conflict resolution list for the corresponding in-memory directory entry
 *    - copies the local buffer to the disk block that was found to be free
 *    - copies the in-memory bitvector to the bitevector blocks on the simulated disk
 *
 *  The access rights and the the owner are taken from the context (umask and uid correspondingly).
 */
SIMFS_ERROR simfsCreateFile(SIMFS_NAME_TYPE fileName, SIMFS_CONTENT_TYPE type) {
    // TODO: implement - DONE
    struct fuse_context *context = simfs_debug_get_context();
    SIMFS_INDEX_TYPE index = getCurrentWorkingDirectory(context);
    if (findFile(fileName) != SIMFS_INVALID_INDEX)
        return SIMFS_DUPLICATE_ERROR;
    SIMFS_INDEX_TYPE pos = simfsAllocateBlock();
    SIMFS_BLOCK_TYPE block = newFileBlock(fileName, context, type);
    memcpy(&simfsVolume->block[pos], &block, sizeof(block));
    addDirectory(pos);
    addFileToDirectory(index, pos);
    return SIMFS_NO_ERROR;
}

bool hasDeleteAccess(SIMFS_INDEX_TYPE file, struct fuse_context *context) {
    if (simfsVolume->block[file].content.fileDescriptor.owner == context->uid)
        if (simfsVolume->block[file].content.fileDescriptor.accessRights & S_IWUSR)
            return true;
    if (simfsVolume->block[file].content.fileDescriptor.accessRights & S_IWOTH)
        return true;
    return false;
}

void deleteIndexBlock(size_t size, SIMFS_INDEX_TYPE index)
{
    for (int i = 0; i < size / (SIMFS_INDEX_SIZE - 1); ++i) {
        for (int j = 0; j<(SIMFS_INDEX_SIZE-1); ++j)
            simfsDeallocateBlock(simfsVolume->block[index].content.index[j]);
        SIMFS_INDEX_TYPE temp = index;
        index = simfsVolume->block[index].content.index[SIMFS_INDEX_SIZE - 1];
        simfsDeallocateBlock(temp);
    }
    for (int i=0; i<size % (SIMFS_INDEX_SIZE-1); ++i) {
        simfsDeallocateBlock(simfsVolume->block[index].content.index[i]);
    }
    simfsDeallocateBlock(index);
}

//////////////////////////////////////////////////////////////////////////

/**
 * Deletes a file from the file system.
 *
 * Hashes the file name and check if the file is in the directory. If not, then it returns SIMFS_NOT_FOUND_ERROR.
 * Otherwise:
 *    - finds the reference to the file descriptor block
 *    - if the referenced block is a folder that is not empty, then returns SIMFS_NOT_EMPTY_ERROR.
 *    - Otherwise:
 *       - checks if the process owner can delete this file or folder; if not, it returns SIMFS_ACCESS_ERROR.
 *       - Otherwise:
 *          - frees all blocks belonging to the file by flipping the corresponding bits in the in-memory bitvector
 *          - frees the reference block by flipping the corresponding bit in the in-memory bitvector
 *          - clears the entry in the folder by removing the corresponding node in the list associated with
 *            the slot for this file
 *          - copies the in-memory bitvector to the bitvector blocks on the simulated disk
 */
SIMFS_ERROR simfsDeleteFile(SIMFS_NAME_TYPE fileName)
{
    // TODO: implement - DONE
    struct fuse_context *context = simfs_debug_get_context();
    SIMFS_INDEX_TYPE directory = getCurrentWorkingDirectory(context);
    SIMFS_INDEX_TYPE file = findFile(fileName);
    if (file == SIMFS_INVALID_INDEX)
        return SIMFS_NOT_FOUND_ERROR;
    SIMFS_BLOCK_TYPE *descriptor = &(simfsVolume->block[file]);
    if (descriptor->type == FOLDER_CONTENT_TYPE)
        if (descriptor->content.fileDescriptor.size != 0)
            return SIMFS_NOT_EMPTY_ERROR;
    if (!hasDeleteAccess(directory, context))
        return SIMFS_ACCESS_ERROR;
    deleteIndexBlock(descriptor->content.fileDescriptor.size,
                     descriptor->content.fileDescriptor.block_ref);
    return SIMFS_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////

/**
 * Finds the file in the in-memory directory and obtains the information about the file from the file descriptor
 * block referenced from the directory.
 *
 * If the file is not found, then it returns SIMFS_NOT_FOUND_ERROR
 */
SIMFS_ERROR simfsGetFileInfo(SIMFS_NAME_TYPE fileName, SIMFS_FILE_DESCRIPTOR_TYPE *infoBuffer) {
    // TODO: implement - DONE
    SIMFS_DIR_ENT *node = simfsContext->directory[hash(fileName)];
    while (node != NULL) {
        SIMFS_BLOCK_TYPE *block = &(simfsVolume->block[node->nodeReference]);
        if (strcmp(fileName, block->content.fileDescriptor.name) == 0) {
            memcpy(infoBuffer, &(block->content.fileDescriptor), sizeof(SIMFS_FILE_DESCRIPTOR_TYPE));
            return SIMFS_NO_ERROR;
        }
        node = node->next;
    }
    return SIMFS_NOT_FOUND_ERROR;
}

//////////////////////////////////////////////////////////////////////////

SIMFS_PROCESS_CONTROL_BLOCK_TYPE *makeNewControlBlock(struct fuse_context *context) {
    SIMFS_PROCESS_CONTROL_BLOCK_TYPE *newControlBlock = calloc(1, sizeof(SIMFS_PROCESS_CONTROL_BLOCK_TYPE));
    newControlBlock->currentWorkingDirectory = simfsVolume->superblock.attr.rootNodeIndex;
    newControlBlock->numberOfOpenFiles = 0;
    newControlBlock->pid = context->pid;
    newControlBlock->next = simfsContext->processControlBlocks;
    simfsContext->processControlBlocks = newControlBlock;
    return newControlBlock;
}

/*
 * Hashes the name and searches for it in the in-memory directory. If the file does not exist,
 * the SIMFS_NOT_FOUND_ERROR is returned.
 *
 * Otherwise:
 *    - checks the per-process open file table for the process, and if the file has already been opened
 *      it returns the index of the openFileTable with the entry of the file through the parameter fileHandle, and
 *      returns SIMFS_DUPLICATE_ERROR as the return value
 *
 *    - otherwise, checks if there is a global entry for the file, and if so, then:
 *       - it increases the reference count for this file
 *
 *       - otherwise, it creates an entry in the global open file table for the file copying the information
 *         from the file descriptor block referenced from the entry for this file in the directory
 *
 *       - if the process does not have its process control block in the processControlBlocks list, then
 *         a file control block for the process is created and added to the list; the current working directory
 *         is initialized to the root of the volume and the number of the open files is initialized to 0
 *
 *       - if an entry for this file does not exits in the per-process open file table, the function finds an
 *         empty slot in the table and fills it with the information including the reference to the entry for
 *         this file in the global open file table.
 *
 *       - returns the index to the new element of the per-process open file table through the parameter fileHandle
 *         and SIMFS_NO_ERROR as the return value
 *
 * If there is no free slot for the file in either the global file table or in the per-process
 * file table, or if there is any other allocation problem, then the function returns SIMFS_ALLOC_ERROR.
 *
 */
SIMFS_ERROR simfsOpenFile(SIMFS_NAME_TYPE fileName, SIMFS_FILE_HANDLE_TYPE *fileHandle) {
    // TODO: implement - DONE
    struct fuse_context *context = simfs_debug_get_context();
    SIMFS_INDEX_TYPE file = findFile(fileName);
    if (file == SIMFS_INVALID_INDEX)
        return SIMFS_NOT_FOUND_ERROR;

    SIMFS_PROCESS_CONTROL_BLOCK_TYPE *controlBlock = findControlBlock(
            simfsContext->processControlBlocks, context->pid);
    if (controlBlock == NULL)
        controlBlock = makeNewControlBlock(context);

    for (SIMFS_FILE_HANDLE_TYPE i = 0; i < controlBlock->numberOfOpenFiles; ++i) {
        SIMFS_INDEX_TYPE index = controlBlock->openFileTable[i].globalEntry->fileDescriptor;
        if (index == file) {
            *fileHandle = i;
            return SIMFS_DUPLICATE_ERROR;
        }
    }

    int openFileTableIndex = 0;
    for (; openFileTableIndex < SIMFS_MAX_NUMBER_OF_OPEN_FILES; ++openFileTableIndex) {
        if (simfsContext->globalOpenFileTable[openFileTableIndex].fileDescriptor == file) {
            ++(simfsContext->globalOpenFileTable[openFileTableIndex].referenceCount);
            break;
        }
    }
    if (openFileTableIndex == SIMFS_MAX_NUMBER_OF_OPEN_FILES) {
        for (openFileTableIndex = 0; openFileTableIndex < SIMFS_MAX_NUMBER_OF_OPEN_FILES; ++openFileTableIndex) {
            if (simfsContext->globalOpenFileTable[openFileTableIndex].fileDescriptor == SIMFS_INVALID_INDEX) {
                simfsContext->globalOpenFileTable[openFileTableIndex].fileDescriptor = file;
                simfsContext->globalOpenFileTable[openFileTableIndex].referenceCount = 1;
                simfsContext->globalOpenFileTable[openFileTableIndex].accessRights = simfsVolume->block[file].content.fileDescriptor.accessRights;
                simfsContext->globalOpenFileTable[openFileTableIndex].type = simfsVolume->block[file].type;
                simfsContext->globalOpenFileTable[openFileTableIndex].size = simfsVolume->block[file].content.fileDescriptor.size;
                simfsContext->globalOpenFileTable[openFileTableIndex].creationTime = simfsVolume->block[file].content.fileDescriptor.creationTime;
                simfsContext->globalOpenFileTable[openFileTableIndex].lastAccessTime = simfsVolume->block[file].content.fileDescriptor.lastAccessTime;
                simfsContext->globalOpenFileTable[openFileTableIndex].lastModificationTime = simfsVolume->block[file].content.fileDescriptor.lastModificationTime;
                simfsContext->globalOpenFileTable[openFileTableIndex].owner = simfsVolume->block[file].content.fileDescriptor.owner;
                break;
            }
        }
    }

    if (openFileTableIndex == SIMFS_MAX_NUMBER_OF_OPEN_FILES)
        return SIMFS_ALLOC_ERROR;
    if (controlBlock->numberOfOpenFiles == SIMFS_MAX_NUMBER_OF_OPEN_FILES_PER_PROCESS)
        return SIMFS_ALLOC_ERROR;

    *fileHandle = controlBlock->numberOfOpenFiles;
    controlBlock->openFileTable[controlBlock->numberOfOpenFiles].accessRights =
            simfsContext->globalOpenFileTable[openFileTableIndex].accessRights;
    controlBlock->openFileTable[controlBlock->numberOfOpenFiles].globalEntry =
            &(simfsContext->globalOpenFileTable[openFileTableIndex]);
    ++(controlBlock->numberOfOpenFiles);
    return SIMFS_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////

/*
 * The function replaces content of a file with new one pointed to by the parameter writeBuffer.
 *
 * Checks if the file handle points to a valid file descriptor of an open file. If the entry is invalid
 * (e.g., if the reference to the global table is NULL, or if the entry in the global table is INVALID_CONTENT_TYPE),
 * then it returns SIMFS_NOT_FOUND_ERROR.
 *
 * Otherwise, it checks the access rights for writing. If the process owner is not allowed to write to the file,
 * then the function returns SIMFS_ACCESS_ERROR.
 *
 * Then, the functions calculates the space needed for the new content and checks if the write buffer can fit into
 * the remaining free space in the file system. If not, then the SIMFS_ALLOC_ERROR is returned.
 *
 * Otherwise, the function removes all blocks currently held by this file, and then acquires new blocks as needed
 * modifying bits in the in-memory bitvector as needed.
 *
 * It then copies the characters pointed to by the parameter writeBuffer (until '\0' but excluding it) to the
 * new blocks that belong to the file. The function copies any modified block of the in-memory bitvector to
 * the corresponding bitvector block on the disk.
 *
 * Finally, the file descriptor is modified to reflect the new size of the file, and the times of last modification
 * and access.
 *
 * The function returns SIMFS_WRITE_ERROR in response to exception not specified earlier.
 *
 */
SIMFS_ERROR simfsWriteFile(SIMFS_FILE_HANDLE_TYPE fileHandle, char *writeBuffer) {
    // TODO: implement
    struct fuse_context *context = simfs_debug_get_context();
    SIMFS_PROCESS_CONTROL_BLOCK_TYPE *controlBlock = findControlBlock(simfsContext->processControlBlocks, context->pid);
    if (controlBlock == NULL)
        return SIMFS_NOT_FOUND_ERROR;
    if (controlBlock->openFileTable[fileHandle].globalEntry->fileDescriptor == SIMFS_INVALID_INDEX)
        return SIMFS_NOT_FOUND_ERROR;

    if (controlBlock->openFileTable[fileHandle].globalEntry->owner == context->uid)
        if (!(controlBlock->openFileTable[fileHandle].accessRights & S_IWUSR))
            return SIMFS_ACCESS_ERROR;
    if (!(controlBlock->openFileTable[fileHandle].accessRights & S_IWOTH))
        return SIMFS_ACCESS_ERROR;

    size_t length = strlen(writeBuffer);
    size_t num_data = length / SIMFS_DATA_SIZE;
    if (length % SIMFS_DATA_SIZE != 0)
        ++num_data;
    size_t num_index = num_data / (SIMFS_INDEX_SIZE - 1);
    if (num_data % (SIMFS_INDEX_SIZE - 1) != 0)
        ++num_index;
    size_t required = num_data + num_index;

    size_t have = 0;
    for (int blockGroup = 0; have < required && blockGroup < SIMFS_NUMBER_OF_BLOCKS / 8; ++blockGroup)
        for (int blockNum = 0; blockNum < 8; ++blockNum)
            have += (simfsVolume->bitvector[blockGroup] ^ (1 << blockNum)) ? 1 : 0;
    if (have < required)
        return SIMFS_ALLOC_ERROR;

    deleteIndexBlock(
            controlBlock->openFileTable[fileHandle].globalEntry->size,
            simfsVolume->block[controlBlock->openFileTable[fileHandle].globalEntry->fileDescriptor].content.fileDescriptor.block_ref);
    SIMFS_INDEX_TYPE newIndex = simfsAllocateBlock();
    simfsVolume->block[controlBlock->openFileTable[fileHandle].globalEntry->fileDescriptor].content.fileDescriptor.block_ref = newIndex;

    //Write data to blocks

    return SIMFS_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////

/*
 * The function returns the complete content of the file to the caller through the parameter readBuffer.
 *
 * Checks if the file handle points to a valid file descriptor of an open file. If the entry is invalid
 * (e.g., if the reference to the global table is NULL, or if the entry in the global table is INVALID_CONTENT_TYPE),
 * then it returns SIMFS_NOT_FOUND_ERROR.
 *
 * Otherwise, it checks the user's access right to read the file. If the process owner is not allowed to read the file,
 * then the function returns SIMFS_ACCESS_ERROR.
 *
 * Otherwise, the function allocates memory sufficient to hold the read content with an appended end of string
 * character; the pointer to newly allocated memory is passed back through the readBuffer parameter. All the content
 * of the blocks is concatenated using the allocated space, and an end of string character is appended at the end of
 * the concatenated content.
 *
 * The function returns SIMFS_READ_ERROR in response to exception not specified earlier.
 *
 */
SIMFS_ERROR simfsReadFile(SIMFS_FILE_HANDLE_TYPE fileHandle, char **readBuffer) {
    // TODO: implement -
    struct fuse_context *context = simfs_debug_get_context();
    SIMFS_PROCESS_CONTROL_BLOCK_TYPE *controlBlock = findControlBlock(simfsContext->processControlBlocks, context->pid);
    if (controlBlock == NULL)
        return SIMFS_NOT_FOUND_ERROR;
    if (controlBlock->openFileTable[fileHandle].globalEntry->fileDescriptor == SIMFS_INVALID_INDEX)
        return SIMFS_NOT_FOUND_ERROR;

    if (controlBlock->openFileTable[fileHandle].globalEntry->owner == context->uid)
        if (!(controlBlock->openFileTable[fileHandle].accessRights & S_IRUSR))
            return SIMFS_ACCESS_ERROR;
    if (!(controlBlock->openFileTable[fileHandle].accessRights & S_IROTH))
        return SIMFS_ACCESS_ERROR;


    SIMFS_INDEX_TYPE index = simfsVolume->block[
            controlBlock->openFileTable[fileHandle].globalEntry->fileDescriptor]
            .content.fileDescriptor.block_ref;
    if (index == INVALID_CONTENT_TYPE) {
        *readBuffer = NULL;
        return SIMFS_NO_ERROR;
    }

    size_t size = controlBlock->openFileTable[fileHandle].globalEntry->size;
    char *contents = calloc(size + 1, sizeof(char));
    int i = 0;
    if (size > SIMFS_DATA_SIZE) {
        for (int indexPage = 0; indexPage < size / (SIMFS_INDEX_SIZE - 1); ++indexPage) {
            for (int dataIter = 0; dataIter < SIMFS_INDEX_SIZE - 1; ++dataIter) {
                SIMFS_INDEX_TYPE dataBlock = simfsVolume->block[index].content.index[dataIter];
                for (int data = 0; data < SIMFS_BLOCK_SIZE; ++data)
                    contents[i++] = simfsVolume->block[dataBlock].content.data[data];
            }
            index = simfsVolume->block[index].content.index[SIMFS_INDEX_SIZE - 1];
        }
        for (int dataIter = 0; dataIter < SIMFS_INDEX_SIZE - 1; ++dataIter) {
            SIMFS_INDEX_TYPE dataBlock = simfsVolume->block[index].content.index[dataIter];
            for (int data = 0; data < SIMFS_DATA_SIZE; ++data)
                contents[i++] = simfsVolume->block[dataBlock].content.data[data];
        }
    } else {
        for (int data = 0; data < size; ++data)
            contents[i++] = simfsVolume->block[index].content.data[data];
    }

    contents[i] = '\0';
    *readBuffer = contents;
    return SIMFS_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////

/*
 * Removes the entry for the file with the file handle provided as the parameter from the open file table
 * for this process. It decreases the number of open files for in the process control block of this process, and
 * if it becomes zero, then the process control block for this process is removed from the processControlBlocks list.
 *
 * Decreases the reference count in the global open file table, and if that number is 0, it also removes the entry
 * for this file from the global open file table.
 *
 */

SIMFS_ERROR simfsCloseFile(SIMFS_FILE_HANDLE_TYPE fileHandle) {
    // TODO: implement

    return SIMFS_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////
//
// The following functions are provided only for testing without FUSE.
//
// When linked to the FUSE library, both user ID and process ID can be obtained by calling fuse_get_context().
// See: http://libfuse.github.io/doxygen/structfuse__context.html
//
//////////////////////////////////////////////////////////////////////////

/*
 * Simulates FUSE context to get values for user ID, process ID, and umask through fuse_context
 */

struct fuse_context *simfs_debug_get_context() {
    // TODO: replace its use with FUSE's fuse_get_context()
    struct fuse_context *context = malloc(sizeof(struct fuse_context));
    context->fuse = NULL;
    context->uid = (uid_t) rand() % 10 + 1; //NOLINT
    context->pid = (pid_t) rand() % 10 + 1; //NOLINT
    context->gid = (gid_t) rand() % 10 + 1; //NOLINT
    context->private_data = NULL;
    context->umask = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH; // can be changed as needed
    return context;
}

char *simfsGenerateContent(int size) {
    size = (size <= 0 ? rand() % 1000 : size); //NOLINT // arbitrarily chosen as an example

    char *content = malloc((size_t) size);

    int firstPrintable = ' ';
    int len = '~' - firstPrintable;

    for (int i = 0; i < size - 1; i++)
        content[i] = (char) (firstPrintable + rand() % len); //NOLINT

    content[size - 1] = '\0';
    return content;
}
