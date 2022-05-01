#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

#include "directory.h"

// initialize the root directory
void directory_init() {
    // *** set the root_dir inode ***

    // retrieve the firsrt inode block and set it as root directory
    inode_t* root_inode = get_inode(0);
    if (root_inode->refs == 0) {
        inode_init(root_inode, 040755, 2, "/");

        // allocate the inode block and record it in inode bitmap
        int inodeIdx = alloc_inode();
        assert(inodeIdx != -1);

        // *** set the root_dir block ***

        // allocate the iblock block and record it in block bitmap
        int blockIdx = alloc_block();
        assert(blockIdx != -1);
    }

}

// retrieve the directory with the given dir inode number
directory_t get_directory(int inum) {
    directory_t dir; // directory to return

    inode_t* dir_inode = get_inode(inum);

    // retrieve the list of entries in given inum's directory
    dirent_t* newEntries = (dirent_t *) blocks_get_block(dir_inode->block);

    dir.entries = newEntries;
    return dir;
}


// find the inode number of given full path and return it
int tree_lookup(const char* path) {

    directory_t dd;
    int static root = 0; // a root inode number

    // split the given path into list of strings
    slist_t* path_list = s_explode(path, '/');
    if (strcmp(path_list->data, "")!=0) {
        // indicates the inappropriate path is given
        // (first item is not root)
        return -ENOENT;
    }

    dd = get_directory(root);
    int inum = root; // the inode number to return
    path_list = path_list->next; // skip the root directory
    int didFound; // a flag that indicates if there was a entry that matched to path

    while (path_list != NULL) {
        didFound = 0;
        for (int i=0; i<DIR_SIZE;i++) {
            // check if any entry from the dd dir equals to path's entry
            if (strcmp(dd.entries[i].name, path_list->data)==0) {
                didFound = 1;
                inum = dd.entries[i].inum;
                inode_t* nested_node = get_inode(inum);
                if (nested_node->mode & 040000) {
                    // indicates the nested inode is a dir
                    dd = get_directory(inum);
                }
                break;
            }
        }
        if (!didFound) {
            // indicates a failure to find the path
            printf("tree_lookup: was not able to find the given path\n");
            return -ENOENT;
        }
        // advance to next path
        path_list = path_list->next;
    }
    return inum;
}

// find the given path name in the given entries and
// returns its inode number
int directory_lookup(directory_t dd, const char* name) {
    for (int i=0; i<DIR_SIZE; i++) {
        if (strcmp(dd.entries[i].name, name)==0) {
            return dd.entries[i].inum;
        }
    }

    // indicates the entry is not found
    return -ENOENT;
}

// insert the given inode and file name as a entry in given directory
int directory_put(directory_t dir, const char *name, int inum) {
    for (int i=0; i<DIR_SIZE; i++) {
        if (dir.entries[i].inum == 0) {
            // indicates that this entry is empty
            // add new entry with given name and inum
            strcpy(dir.entries[i].name, name);
            dir.entries[i].inum  = inum;
            return 0;
        }
    }
    // indicates that the given directory is full
    return -ENOENT;
}

// delete the given inode and file name from a given directory's entry
int directory_delete(directory_t dir, const char* name) {
    int inum; // inode number to remove

    for (int i=0; i<DIR_SIZE; i++) {
        if (strcmp(dir.entries[i].name, name) == 0) {
            // indicate that we found the corresponding entry
            inum = dir.entries[i].inum;
            dir.entries[i].inum = 0;
            memset(&dir.entries[i].name[0], 0, sizeof(dir.entries[i].name));
            return inum;
        }
    }
    // indicates that we weren't able to find the given corresponding entry
    return -ENOENT;
}

// removes the given path and delete it from its parent directory and return its inum
int directory_remove(const char* path) {
    char* pathCp1 = (char*)malloc(strlen(path)*sizeof(char));
    char* pathCp2 = (char*)malloc(strlen(path)*sizeof(char));

    strcpy(pathCp1, path);
    strcpy(pathCp2, path);

    char* fileName = basename(pathCp1); // retrieve the name of ending path
    char* parentDir = dirname(pathCp2); // retrieve the dir until the end path

    // retrieve the parent dir of given path
    int parentDirInum = tree_lookup(parentDir);
    if (parentDirInum < 0) {
            return -ENOENT;
    }
    //assert(parentDirInum >= 0);
    directory_t pDir = get_directory(parentDirInum);
    dirent_t* entryToDel; // a entry to remove

    for (int i=0; i<DIR_SIZE; i++) {
        if (strcmp(pDir.entries[i].name, fileName)==0) {
            // indicates we found the entry to remove
            entryToDel = &(pDir.entries[i]);
            break;
        }
    }

    if (entryToDel == NULL) {
        // indicates that the given entry did not existed
        return -ENOENT;
    }

    directory_t entryDir = get_directory(entryToDel->inum);

    // check if entry to delete has empty contents
    for (int i=0; i<DIR_SIZE; i++) {
        if (entryDir.entries[i].name[0] != 0) {
            // indicates that the entry to remove is not empty
            return -ENOENT;
        }
    }

    // delete entry's metadata
    int inum = entryToDel->inum;
    entryToDel->inum = 0;
    memset(&entryToDel->name[0], 0, sizeof(entryToDel->name));

    free(pathCp1);
    free(pathCp2);
    return inum;
}
