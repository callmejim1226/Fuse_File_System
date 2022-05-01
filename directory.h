// Directory manipulation functions.
//
// Feel free to use as inspiration.

// based on cs3650 starter code

#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_NAME_LENGTH 48
#define DIR_SIZE 32

#include <string.h>

#include "blocks.h"
#include "inode.h"
#include "slist.h"

typedef struct dirent {
  char name[DIR_NAME_LENGTH];
  int inum;
  //char _reserved[12];
} dirent_t;

typedef struct directory {
	dirent_t* entries;
} directory_t; // added 

void directory_init();
directory_t get_directory(int inum); 
int directory_lookup(directory_t dd, const char *name);
int tree_lookup(const char *path);
int directory_put(directory_t dd, const char *name, int inum);
int directory_delete(directory_t dd, const char *name);
slist_t *directory_list(const char *path);
void print_directory(directory_t dd);
int directory_remove(const char* path); 

#endif
