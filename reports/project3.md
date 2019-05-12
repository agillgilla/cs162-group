Final Report for Project 3: File System
=======================================

## Changes

### Task 1 (Buffer Cache):

The buffer cache exhibited the least amount of changes from our original design compared to the other two tasks.  One mistake we made in the design doc was storing the main array of `cache_block`s as an array of pointers to `cache_block`s.  This would have worked, however we would have had to `malloc` all the `cache_block`s on initialization and `free` them all on closing.  Instead we just declared the static array of `cache_block`s so we didn't have to deal with this.

We also had to change our function declarations for `cache_read_at(...)` and `cahce_write_at(...)` because we didn't need to specify a offset and size, we just read/write the whole 512 byte block from/to disk.

Everything else is essentially the same, we just added some helper functions, `cache_check(...)` which loops through the cache to see if an entry already exists, and `cache_get_block(...)` which returns a block to use in the cache either by returning an invalid one or evicting according to the clock algorithm.

### Task 2 (Extensible Files):

The main idea for our implementation of extensible files using `inode`s remained the same, however there were some subtle changes that we had to deal with by using helper functions.  Our main `inode_extend(...)` function (which has the most complex logic in `inode.c`) didn't change much, but we did have to account for many scenarios such as dynamically allocating indirect pointers in the doubly indirect block and many edge cases.

We also created a helper function, `inode_alloc(...)`, which pre-allocates all direct block pointers, the indirect block, and the doubly indirect block.  The last helper function, `inode_dealloc(...)`, is used to completely deallocate an `inode`'s disk space, and does so by freeing blocks at all levels.  This helper function is only called by `inode_remove(...)`.

### Task 3 (Subdirectories):

There were many edge cases and implementation subtleties that we overlooked while writing the design doc for subdirectories.  The first was that we needed to add a `dir *` object to our `file_entry` struct that was stored in the per-process `file_table`.  This is so we could have file descriptors open to directories, too.

We also had to create special functions for handling file I/O operations using `.` and `..`.  This could have been handled by adding those entries to the directory when calling `mkdir(...)`, however, that would cause us to have to change `dir_lookup(...)` or `readdir(...)` to skip those special entries.  This caused lots of errors, so instead we created special cases and helper functions to handle these.

Some crucial helper functions we created were:

* `dirname(...)` and `basename(...)`:  These two functions were actually inspired by functions in DeforaOS, a POSIX open source OS, and return the directory or base file name given a full file path.  For example, given `/home/a/b/c`, `dirname(...)` would return `/home/a/b` and `basename(...)` would return `c`.  These functions were extremely useful for breaking apart file paths.

* `try_to_get_dir(...)`: This was a crucial function that, given a full file path, iterates through using `dir_lookup` to get the directory fo the file that is requested, handling both relative and absolute file paths.

* `rel_to_abs(...)`: This function simply determined if a path was relative or absolute by checking if it started with a `/`, and returned either the root directory or the current working directory accordingly.

* `is_parent_dir(...)`: This function checks if an `inode` is the parent of another `inode` to make sure that disallowed directory deletions don't happen.

* `print_dir_structure(...)`: This function didn't add functionality to the project, but was **extremely** useful for debugging, as it essentially dumped a Linux style `tree` function output of the current directory structure of the filesystem.

We also took into account Jason's recommendation of storing the current working directory as an `inode` pointer to the directory, and added a field in `struct inode` to have a pointer to the parent of a given `inode`.

## Reflection

### Task 1 

Emma started task 1 with the read function, and Alex fixed her read/flush functions and implemented the write function of it, but Arjun ended up rewriting most of it.

### Task 2

Arjun did task 2.

### Task 3

Nick started the base of task 2 and Arjun finished it.

### Tests


## Student Testing Report

### Test 1: <my-test-1>:

**Description:** 

Test cache's effectiveness by measuring cache hit rate. 
First, read from a file and measure cache hit rate for a cold cache. Then read the file again sequentially to measure if the cache hit rate improved. 

**Overview:** 

The test works by creating a file with random number with 512 bytes generated with `random_bytes`.
New syscalls cache_reset(), get_cache_hit(), get_cache_miss() are added to reset the cache and fetch cache_hit, cache_miss that are used to calculate the cache hit rate. 

First, reset the cache by calling syscall `cache_reset()` and read sequentially. Close the file, calculate the cache hit rate by calling `get_cache_hit()`, `get_cache_miss()` and open it to read for the second time. If the new cache hit rate is greater than cache hit rate measured for a cold cache, test passes.  

**Output:**: 

```C
Copying tests/filesys/extended/my-test-1 to scratch partition...
Copying tests/filesys/extended/tar to scratch partition...
qemu -hda /tmp/Rp9GWVD8j9.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading...........
Kernel command line: -q -f extract run my-test-1
Pintos booting with 4,088 kB RAM...
382 pages available in kernel pool.
382 pages available in user pool.
Calibrating timer...  314,163,200 loops/s.
hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
hda1: 185 sectors (92 kB), Pintos OS kernel (20)
hda2: 243 sectors (121 kB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'my-test-1' into the file system...
Putting 'tar' into the file system...
Erasing ustar archive...
Executing 'my-test-1':
(my-test-1) begin
(my-test-1) make "cache"
(my-test-1) create "cache"
(my-test-1) open "cache"
(my-test-1) close "cache"
(my-test-1) reset buffer
(my-test-1) open "cache"
(my-test-1) close "cache"
(my-test-1) open "cache"
(my-test-1) close "cache"
(my-test-1) New hit rate is higher than old hit rate
(my-test-1) end
my-test-1: exit(0)
Execution of 'my-test-1' complete.
Timer: 62 ticks
Thread: 0 idle ticks, 60 kernel ticks, 2 user ticks
hdb1 (filesys): 93 reads, 562 writes
hda2 (scratch): 242 reads, 2 writes
Console: 1276 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...

```

**Result:** 

```C
PASS
```

**Kernel Bugs:** 

The potential kernel bugs are 1) if the file fails open properly by obtaining a valid file descriptor which will be used for write and close and 2) if zero division error occurs when calculating cache hit rate for the second time. If total of new cache hit and miss rate equals -1, it will cause kernel to panic with division error.  

---

### Test 2: <enter test name here>:

**Description:** 
Test if clock algorithm works correctly by checking when files are evicted by artificially forcing the clock algorithm to evict blocks, and then checking the cache_miss and cache_hit rate

**Overview:** 
The test starts with filling the cache with 64 files of block size 512 generated with `random_bytes`. It is then filled completely, and 64 cache misses should be expected. After that, it will attempt to read the first entry put in (which should hit the cache), read a block that's not in the cache (which should cause a cache miss and evict the second entry), and then read the second again (which should cause another cache miss). If the new cache hit number is 1 and cache miss rate is 66, then the test passes. This test uses the same syscalls as the previous test.

**Output:** 

**Result:** 

**Kernel Bugs:** 

---

### Review:

Emma: I was unfamiliar with writing test cases so I had to go through the provided test cases and learned which functions could be used in the test. Especially for this case, I had to implement new syscalls by editing few syscall files in pitos, because functions that are in the user program cannot be called. Besides writing a test file, I learned a lot from editing files from different directories to understand how syscalls and tests run. 


## Code Quality

* Our code doesn't exhibit any major memory safety problems (that we know of.)  Any strings `malloc`'d are freed when the process exits or the function completes.  We rarely use `malloc` otherwise.  There is no poor error handling in our code, any time an error occurs the process sets its exit code or the function exits gracefully.  There is no potential for race conditions in our code, either, because we use locks when necessary as detailed in the design document.

* We maintained the same code style as in Pintos.  Same two-space indentation and spacing around kernel function calls, as well as keeping `snake_case` for variable and function names.

* The code algorithms are as simple as we think possible and fairly easy to understand if someone is looking over it who hasn't seen it before.

* We put a multitude of comments in our code, especially around complex sections in task 2, and the special/difficult cases for task 3.  Most of the code in `cache.c` is self explanatory, so we don't have as many comments there,  because things are handled with the typical way for caches (clock algorithm for eviction, setting appropriate flags on reads/writes, flushing, etc.)

* We didn't leave commented-out code in the final submission.

* We created functions when possible rather than copying and pasting.  This includes helper functions in `cache.c` for cache searches and evictions/flushes and `filesys.c` for getting parts of file paths and checking various properties of `dir`s.  We also created helper functions in `inode.c` for extending inodes and gettsers/setters of struct members for opaque types

* We didn't have to create any of our own lists for this project.

* We don't have any source code that is excessively long, we put new lines and indent accordingly whenever there is a very long line.

* We never commited binary files to Git.

