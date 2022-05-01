// Inode manipulation routines.
//
// Feel free to use as inspiration.

// based on cs3650 starter code

#ifndef INODE_H
#define INODE_H
#include <fcntl.h>
#include "blocks.h"
#include "bitmap.h" 
typedef struct inode {
  const char* name; // the path name of the inode
  int refs;  // reference count
  int mode;  // permission & type
  int size;  // bytes
  int block; // single block pointer (if max file size <= 4K)
  time_t sTime; // start time
  time_t cTime; // creation time
} inode_t;

void inode_init(inode_t* curInode,int mode, int block, const char* name); // added
void print_inode(inode_t *node);
inode_t *get_inode(int inum);
int alloc_inode();
void free_inode();
void grow_inode(inode_t *node, int size);
int shrink_inode(inode_t *node, int size);
int inode_get_pnum(inode_t *node, int fpn);

#endif
