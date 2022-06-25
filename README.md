# Fuse File System 


For this project, I used given resources files (below) in order to build a FUSE filesystem driver that will let you mount a 1MB disk image (data file) as a filesystem. It has a basic features of file system, such as, creating, reading from, writing into, renaming, and deleting files and directories. 

The following contents are provided as the starter codes:

- [Makefile](Makefile)   - Targets are explained in the assignment text
- [README.md](README.md) - This README
- [helpers](helpers)     - Helper code implementing access to bitmaps and blocks
- [hints](hints)         - Incomplete bits and pieces that we might want to use as inspiration
- [nufs.c](nufs.c)       - The main file of the file system driver
- [test.pl](test.pl)     - Tests to exercise the file system

## Makefiles
The provided Makefile should simplify our development cycle. It provides the following targets:

- make nufs - compile the nufs binary. This binary can be run manually as follows:

$ ./nufs [FUSE_OPTIONS] mount_point disk_image
- make mount - mount a filesystem (using data.nufs as the image) under mnt/ in the current directory

- make unmount - unmount the filesystem

- make test - run some tests on our implementation.

- make gdb - same as make mount, but run the filesystem in GDB for debugging

- make clean - remove executables and object files, as well as test logs and the data.nufs.

## Running the tests

```
$ sudo apt-get install libtest-simple-perl
```

Then using `make test` will run the provided tests.


