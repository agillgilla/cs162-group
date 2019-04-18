Design Document for Project 3: File System
============================================

## Group Members

* Arjun Gill <arjun.gill@berkeley.edu>
* Nicholas Ruhman <nwruhman@berkeley.edu>
* Emma Han <emma_han@berkeley.edu>
* Alexander Mao <mao@berkeley.edu>

---

## Task 1: Buffer Cache

### Data structures and functions

#### In `inode.c`:
```C

#define CACHE_BLOCKS 64;

struct cache_block *cache_blocks[CACHE_BLOCKS]; /* Array of cache_blocks */
struct lock cache_lock;                         /* Lock for cache_block synchronization */
unsigned clock_index;                           /* Current position of the clock hand for clock algorithm */

struct cache_block {
    block_sector_t sector;    /* Sector on disk that this cache is for */
    void *data;               /* Raw data from sector in cache */
    struct lock block_lock;   /* Lock for synchronization */
    bool valid;               /* Valid bit (set false on init, always true after) */
    bool dirty;               /* Dirty bit */
    bool recently_used;       /* Flag for clock algorithm (evict if false) */
}

void filesys_cache_init();

void cache_read_at(block_sector_t sector, void void *buffer, off_t size, off_t block_ofs);

void cache_write_at(block_sector_t sector, const void *buffer, off_t size, off_t block_ofs);

void cache_flush();
```

### Algorithms

The algorithms necessary for the buffer cache are mostly encapsulated within the four functions above.  First, in `filesys_cache_init()`, we initialize all the necessary values for the cache.  This includes initalizing the global `cache_lock`, and iterating throught the `cache_blocks` to initialize all the member variables for each `cache_block`. 

We have made the `cache_read_at(...)` and `cache_write_at(...)` functions mimic the function style for file reads and writes.

In `cache_read_at(...)` and `cache_write_at(...)`, we need to first check if the `sector` exists in the cache by iterating through the entries until we find it (or reach the end.)  Then, for `cache_read_at(...)`, if the `sector` exists, we just read it into the `buffer`, otherwise we need to run the clock algorithm on `cache_blocks` until we find an entry to evict, then evict it and load the new entry into its place.  If we are doing `cache_write_at(...)`, if the `sector` is loaded into the cache, we directly write to it and set the `dirty` bit accordingly.  Otherwise, we find an entry to evict again, write it out to disk, then pull the `sector` we are writing to into the cache and write to it.  Also, whenever a `cache_block` is read or written to, we set the `recently_used` flag to `true`.

***NOTE:*** Evicting a block and loading a block from disk will require acquisition of the `block_lock` on said block.

In `cache_flush()`, we simply iterate through `cache_blocks` and directly write all valid entries into the appropriate `block_sector_t` using the `block_write(...)` function.

### Synchronization

Synchronization will be handled using `lock`s for all operations in the cache.

First, for maintaining consistency of the `cache_blocks` array, we have a global `cache_lock` before any operations on setting/reading members of any entries in `cache_blocks`.

Second, for handling any attempts at concurrent reads or writes in the filesystem (to the same sector) there will have to be usage of the `block_lock` in each `cache_block`.  Before any read or write to a block, a thread of execution will have to have called `lock_acquire(...)` on the `block_lock` for the `cache_block` it is trying to read or write the `void *data` to (or read from.)

### Rationale

The implementation of the buffer cache described above was selected for many reasons.  First, the implementation is simple, as it implements only the aspects of a typical cache that are necessary for this situation (valid and dirty bits.)  Also, the implementation satisfies all the requirements described in 4.4.1 of the spec: We limit the cache to 64 sectors, we implement the clock algorithm as specified, we don't use a bounce buffer (just copy data directly to sectors), we implement a write-back cache that flushes on eviction/shutdown.

The only issues with our implementation is that it doesn't use LRU (when it probably could) and that finding a sector in the cache takes time linear to the number of the cache size (in this case `O(64)`) because the array has to be iterated through to find the corresponding `block_sector_t`.  Both of these issues could be fixed with more code, but the implementation we have described is simpler and more straightforward.  

---

## Task 2: Extensible Files

### Data structures and functions

#### In `inode.c`:
```C

struct lock inode_list_lock;

struct inode_disk {
    block_sector_t direct_ptrs[124];      /* Array of direct pointers */
    block_sector_t indirect_ptr;          /* Singly indirect pointer */
    block_sector_t doubly_indirect_ptr;   /* Doubly indirect pointer */
    
    off_t length;                         /* File size in bytes. */
    unsigned magic;                       /* Magic number. */
}

static block_sector_t byte_to_sector (const struct inode *inode, off_t pos);

bool inode_create (block_sector_t sector, off_t length);

void inode_close (struct inode *inode);

off_t inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset);

off_t inode_write_at (struct inode *inode, const void *buffer_, off_t size, off_t offset);
```

### Algorithms

First, we can change `byte_to_sector` to use the `length` of the `inode_disk` and find the sector that is necessary by going through direct, indirect, or doubly indirect pointers as necessary, rather than assuming that the data is stored in contiguous sectors.

Second, since we are adding direct, indirect, and doubly indirect pointers to the `struct inode_disk`, we now have to handle the `inode_` functions differently so that we are intializing the data structure (and its members) properly, and handling it appropriately for creation, reads, writes, and closing.

For `inode_create(...)`, we need to `malloc` the `inode_disk` properly and initialize it's length.

For `indoe_close(...)`, we need to free the `inode_disk` struct and properly free the sectors that are now unused in the free map.

For `inode_read_at(...)`, we have to decide if we are going to read/iterate through the direct, indirect, or doubly indirect pointers to get to the sector(s) we need to read from and properly read the data at the offset desired.

For `inode_write_at(...)`, we must first check if the file needs to be extended, then allocate the apropriate sectors associated to the new pointers and call the functions necessary in `free-map.c` for reserving the sectors.

### Synchronization

Since all of the `inode_` functions could possibly be called concurrently, we need a way of serializing them so that there is coherency, especially if there are multiple writes on a single `inode_disk`.  To deal with this, we simply use the `inode_list_lock` which must be acquired before doing **any** operations on the `inode`s in the `open_inodes` list in `inode.c`.

### Rationale

This implementation we have come up with matches the requirements specified in the spec exactly.  It supports files over 8 MiB because 1 doubly indirect pointer does that on its own (`128 * 128 * 512 B = 8 MiB`.)   Our modifications to the `inode_disk` struct also are the most straightforward way to implement a Unix-y filesystem while having inodes fit in exactly `512 B`.  These modifications also make for the most simple and minimally invasive changes necessary to the existing filesystem while still satisfying the requirements.

---

## Task 3: Subdirectories

### Data structures and functions



### Algorithms



### Synchronization



### Rationale



---

## Additional Questions

1. 
