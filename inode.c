#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "inode.h"

const int INODE_BLOCK_COUNT = 4096/sizeof(inode_t); // we let one block (4kb size) to
                                                    // to contain all inode blocks
// initialize the given inode with given fields
void inode_init(inode_t* curInode, int mode, int block, const char* name) {
    curInode->mode = mode;
    curInode->block = block;
    curInode->refs = 1;
    curInode->size = 0;
    curInode->name = name;
}
// Allocate a inode and return its index.
int alloc_inode() {
  void *ibm = get_inode_bitmap();

  for (int ii = 0; ii < INODE_BLOCK_COUNT; ++ii) {
    if (!bitmap_get(ibm, ii)) { // if bit at ii has a free block
      bitmap_put(ibm, ii, 1);
      printf("+ alloc_block() -> %d\n", ii);
      return ii;
    }
  }

  return -1;
}

// Deallocate the inode block with the given index
void free_inode(int inum) {
  printf("+ free_inode_block(%d)\n", inum);
  void *ibm = get_inode_bitmap();
  bitmap_put(ibm, inum, 0);
}

// Gets a inode at the given position (index)
inode_t *get_inode(int inum) {
    void * inodeAddr = blocks_get_block(1);
    return  inodeAddr + sizeof(inode_t) * inum;
}

// update the inode's size
void grow_inode(inode_t* inode, int size) {
  inode->size = size;
}
