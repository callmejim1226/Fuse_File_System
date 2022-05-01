#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>

#include "inode.h"
#include "storage.h"
#include "blocks.h"
#include "directory.h"

// Define a disk (storage) to be used for fuse
void* diskS;

// Initializes the disk's storage
void storage_init(const char* path) {

    // initialize the disk's blocks
    blocks_init(path);

    // initialize the root directory
    directory_init();
}

// Sets a given path object's attributes to given stat
int storage_stat(const char *path, struct stat *st) {
     printf("Storage_stat got called with path: %s\n", path);
    // Retrieve an index of the given path in inode table
    int posIdx = tree_lookup(path);
    if (posIdx < 0) {
        // indicates there was no file/dir existed with given path
        printf("No file existed\n");
        return -ENOENT;
    }
    // Get the inode at specified index(inode)
    inode_t * curInode = get_inode(posIdx);

    // write sizeof stat bytes of 0 to st
    memset(st, 0, sizeof(struct stat));

    st->st_mode = curInode->mode;
    st->st_size = curInode->size;
    st->st_nlink = curInode->refs;
    st->st_uid = getuid(); // question! ******
    struct timespec mtime = { curInode->cTime, (long) curInode->cTime};
    st->st_mtim = mtime;
    return 0;
}

// Reads all the given dir's entries
int storage_readdir(const char* path, void* buf, struct stat st, fuse_fill_dir_t filler) {
    printf("readdir with given path: %s\n", path);
    directory_t dir;
    // retrieve the given path's directory
    int dir_inum = tree_lookup(path);
    if (dir_inum != -ENOENT) {
            dir = get_directory(dir_inum);
    }

    char fullPath[256]; // a full path including the dir's entries
    for (int i=0 ; i<DIR_SIZE; i++) {
        char* eName = dir.entries[i].name;
        if (eName[0] != 0) {
            // indicates there is an entry in dir and get full path
            strcpy(fullPath, path);
            strcat(fullPath, eName);

            // get stat (metadata) in given stat
            storage_stat(fullPath, &st);
            filler(buf, eName, &st, 0);
        }
    }

    return 0;
}

// creates new file/dir with given path and mode
int storage_mknod(const char* path, int mode) {

    char* pathCp1 = (char*)malloc(strlen(path)*sizeof(char));
    char* pathCp2 = (char*)malloc(strlen(path)*sizeof(char));

    strcpy(pathCp1, path);
    strcpy(pathCp2, path);

    char* fileName = basename(pathCp1); // retrieve the name of ending path
    char* dirName = dirname(pathCp2); // retrieve the dir until the end path

    int dirInum = tree_lookup(dirName);
    inode_t* dirInode = get_inode(dirInum);

    if (dirInode == NULL) {
        // indicates there is no inode with given path
        return -ENOENT;
    }
    directory_t dir = get_directory(dirInum);
    if (directory_lookup(dir, fileName) != -ENOENT) {
        // indicates the filename already exists in its parent dir
        return -EEXIST;
    }

    // search for free inode
    int freeInum = alloc_inode(); // search for free inode number
    assert(freeInum > 0);
    inode_t * newInode = get_inode(freeInum);

    int freeBlock = alloc_block();  // search for free block number
    assert(freeBlock > 2);
    inode_init(newInode, mode, freeBlock, path); // initialize the new inode

    // add the entry to its parent directory
    int rv = directory_put(dir, fileName, freeInum);

    free(pathCp1);
    free(pathCp2);

    return rv;
}


// link the given from path to given to path
int storage_link(const char* from, const char* to) {
    // retrieve the inode of given from path
    int fInum = tree_lookup(from);
    if (fInum < 0) {
        // indicates invalid path given
        return -ENOENT;
    }
    inode_t* fInode = get_inode(fInum);

    char* pathCp1 = (char*)malloc(strlen(to)*sizeof(char));
    char* pathCp2 = (char*)malloc(strlen(to)*sizeof(char));

    strcpy(pathCp1, to);
    strcpy(pathCp2, to);

    char* fileName = basename(pathCp1); // retrieve the name of ending path
    char* dirName = dirname(pathCp2); // retrieve the dir until the end path

    int toParentDirInum = tree_lookup(dirName);
    directory_t toParentDir;
    if (toParentDirInum != -ENOENT) {
        // indicates valid dir inum 
        toParentDir = get_directory(toParentDirInum);
    }

    inode_t* toParent = get_inode(toParentDirInum);
    if (toParent->block <= 0) {
        // indicates invalid block number
        return -ENOENT;
    }
    
    int link = directory_put(toParentDir, (const char*) fileName, fInum);
    if (link == 0) {
        // indicates a success to linking
        fInode->refs += 1;
    }
    free(pathCp1);
    free(pathCp2);
    return link;
}

// unlink the given path
int storage_unlink(const char* path) {

    char* pathCp1 = (char*)malloc(strlen(path)*sizeof(char));
    char* pathCp2 = (char*)malloc(strlen(path)*sizeof(char));

    strcpy(pathCp1, path);
    strcpy(pathCp2, path);

    char* fileName = basename(pathCp1); // retrieve the name of ending path
    char* dirName = dirname(pathCp2); // retrieve the dir until the end path

    int parentDirInum = tree_lookup(dirName);
    //assert(parentDirInum >= 0);
    if (parentDirInum < 0) {
        return -ENOENT;
    }

    directory_t parentDir = get_directory(parentDirInum);

    // delete the given file name and inode from its parent dir
    int fileInum = directory_delete(parentDir, (const char*) fileName);
    if (fileInum < 0) {
        return -ENOENT;
    }

    inode_t* unlinkedNode = get_inode(fileInum);

    // if given inode has no references, remove all links
    if (unlinkedNode->refs == 0) {
        if (unlinkedNode->block > 2) {
            // indicates the inode is not pointing to bitmap or inode blocks
            free_block(unlinkedNode->block);
            unlinkedNode->block = 0;
        }
        unlinkedNode->size = 0;
        free_inode(fileInum);
    }

    free(pathCp1);
    free(pathCp2);
    return 0;
}

// remove the given path directory
int storage_rmdir(const char* path) {

    int rv = directory_remove(path); // remove the path directory and get its inum
    if (rv < 0) {
        return -ENOENT;
    }

    inode_t* inodeToDel = get_inode(rv);

    // if inode has no references, remove all links
    if (inodeToDel->refs == 0) {
        if (inodeToDel->block > 2) {
            // indicates the inode is not pointing to bitmap or inode blocks
            free_block(inodeToDel->block);
            inodeToDel->block = 0;
        }
        inodeToDel->size = 0;
        free_inode(rv);
    }
    return 0;

}

// rename the given from path to given to path
int storage_rename(const char* from, const char* to) {
    if (storage_link(from, to) < 0) {
        return -ENOENT;
    }
    return storage_unlink(from);
}

// change the mode of the given path with given mode
int storage_chmod(const char* path, mode_t mode) {
    // retrieve the path's inode
    int pathInum = tree_lookup(path);
    //assert(pathInum >= 0);
    if (pathInum < 0) {
        return -ENOENT;
    }
    inode_t* pathInode = get_inode(pathInum);
    pathInode->mode = mode;

    return 0;
}

// truncate the given path file to given size
int storage_truncate(const char* path, off_t size) {
    // retrieve the path's inode
    int pathInum = tree_lookup(path);
    //assert(pathInum >= 0);
    if (pathInum < 0) {
        return -ENOENT;
    }
    inode_t* pathInode = get_inode(pathInum);
    pathInode->size = size;

    return 0;
}

// retrieve the whole data that the given path contains
const char* retrieve_data(const char* path) {
    // retrieve the given path's inode
    int pathInum = tree_lookup(path);
    //assert(pathInum >= 0);
    if (pathInum < 0) {
        return NULL;
    }
    inode_t* pathInode = get_inode(pathInum);

    int pathBNum = pathInode->block;
    assert(pathBNum > 2);
    const char* wholeData = (const char*) blocks_get_block(pathBNum);

    return wholeData;
}

// read the given path bytes at the given max size bytes and copy it in given buf
int storage_read(const char* path, char* buf, size_t size, off_t offset) {
    const char* wholeData = retrieve_data(path); // get whole data that is in given path
    if (wholeData == NULL) {
        return -ENOENT;
    }

    int dataSize = strlen(wholeData) + 1;
    if (dataSize > size) {
        // indicates the file data size is bigger than the given max size
        // so read only max size
        dataSize = size;
    }

    strncpy(buf, wholeData, dataSize); // read bytes and copy it in given buf
    return dataSize; // return n bytes read
}

// write the data in the given path with provided size
int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
    // retrieve the given path's inode
    int pathInum = tree_lookup(path);
    if (pathInum < 0) {
        return -ENOENT;
    }
    //assert(pathInum >= 0);
    inode_t* pathInode = get_inode(pathInum);

    assert(pathInode->block!=0);
    void * dataB = blocks_get_block(pathInode->block);

    memcpy(dataB, buf, size); // write the given data into data block

    grow_inode(pathInode, size); // update inode's size
    return size; // indicates success on write
}

// set the time of the given path inode
int storage_set_time(const char* path, const struct timespec ts[2]) {
    // retrieve the given path's inode
    int pathInum = tree_lookup(path);
    if (pathInum < 0) {
        return -ENOENT;
    }
    //assert(pathInum >= 0);
    inode_t* pathInode = get_inode(pathInum);

    pathInode->sTime = ts[0].tv_sec;
    pathInode->cTime = ts[1].tv_sec;

    return 0;
}
