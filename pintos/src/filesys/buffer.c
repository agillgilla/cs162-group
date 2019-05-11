#include "filesys/buffer.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "devices/block.h"

/* Buffer cache with a maximum capacity of 64 disk blocks */
#define CACHE_BLOCKS 64

struct cache_block cache_blocks[CACHE_BLOCKS];    /* Array of cache_blocks */
struct lock cache_lock;                           /* Lock for cache_block synchronization */
unsigned clock_index;                             /* Current position of the clock hand for clock algorithm */
size_t cache_miss;                                /* Number of cache misses */
size_t cache_hit;                                 /* Number of cache hits */

static struct cache_block *cache_get_block(void);
static struct cache_block *cache_check(block_sector_t sector);

void
filesys_cache_init(void)
{
  lock_init(&cache_lock);
  clock_index = 0;
  cache_miss = 0;
  cache_hit = 0;

  int i;
  for (i = 0; i < CACHE_BLOCKS; i++) {
    lock_init(&cache_blocks[i].block_lock);
    cache_blocks[i].valid = false;
    cache_blocks[i].dirty = false;
  }
}

/* Checks if block with sector number SECTOR
   is in the cache.  Returns a pointer to the cache
   block if it is and NULL otherwise. 

   Precondition: Must be holding the global cache lock. */
static struct cache_block *
cache_check(block_sector_t sector)
{
  int i;
  for (i = 0; i < CACHE_BLOCKS; i++) {
    /* We only care if the entry is valid */
    if (cache_blocks[i].valid) {
      /* Check if the sector number is the same as the one we are looking for */
      if (cache_blocks[i].sector == sector) {
        /* Cache hit.  Increment counter and return block pointer */
        cache_hit++;
        return &cache_blocks[i];
      }
    }
  }
  /* We didn't find the entry.
  Cache miss, increment counter and return null. */
  cache_miss++;
  return NULL;
}

/* Gets a block to use from the cache.  If not full, just returns
   an invalid entry.  If it is full, then it evicts using clock
   algorithm and returns the evicted block. */
static struct cache_block *
cache_get_block(void)
{
  /* Loop until we find a block to return, in which case
  we will just break via return */
  while (1) {
    if (!cache_blocks[clock_index].valid) {
      /* This cache block isn't valid, so we can just return it */
      return &cache_blocks[clock_index];
    }

    if (cache_blocks[clock_index].recently_used) {
      /* Don't evict if the block was recently used */
      cache_blocks[clock_index].recently_used = false;
    } else {
      if (cache_blocks[clock_index].dirty) {
        /* This cache block is dirty, write it back to disk */
        block_write(fs_device, cache_blocks[clock_index].sector, cache_blocks[clock_index].data);
        cache_blocks[clock_index].dirty = false;
      }
      /* Invalidate the entry so we know we can use it */
      cache_blocks[clock_index].valid = false;
      
      return &cache_blocks[clock_index];
    }

    /* Move the clock hand and reset to beginning if it gets too big */
    clock_index++;
    if (clock_index == CACHE_BLOCKS) {
      clock_index = 0;
    }
  }
}

void
cache_read_at(block_sector_t sector, void *buffer)
{
  /* Acquire the main cache lock */
  lock_acquire(&cache_lock);
  
  /* Check if the block is in the cache and retrieve it */
  struct cache_block *block = cache_check(sector);

  if (block == NULL) {
    /* The block wasn't in the cache, get a new one and set it up */
    block = cache_get_block();
    block->sector = sector;
    block->valid = true;
    block->dirty = false;
    block_read(fs_device, sector, block->data);
    block->recently_used = true;
    memcpy(buffer, block->data, BLOCK_SECTOR_SIZE);
  } else {
    /* The block was in the cache, copy into buffer and update recently_used */
    block->recently_used = true;
    memcpy(buffer, block->data, BLOCK_SECTOR_SIZE);
  }

  /* Release the main cache lock */
  lock_release(&cache_lock);
}

void
cache_write_at(block_sector_t sector, const void *buffer)
{

  /* Acquire the main cache lock */
  lock_acquire(&cache_lock);
  
  /* Check if the block is in the cache and retrieve it */
  struct cache_block *block = cache_check(sector);

  if (block == NULL) {
    /* The block wasn't in the cache, get a new one and set it up */
    block = cache_get_block();
    block->sector = sector;
    block->valid = true;
    block->dirty = true;
    block->recently_used = true;
    memcpy(block->data, buffer, BLOCK_SECTOR_SIZE);
  } else {
    /* The block was in the cache, copy into buffer and update recently_used */
    block->dirty = true;
    block->recently_used = true;
    memcpy(block->data, buffer, BLOCK_SECTOR_SIZE);
  }

  /* Release the main cache lock */
  lock_release(&cache_lock);
}

void
cache_flush(void)
{
  /* Acquire the main cache lock */
  lock_acquire(&cache_lock);
  
  int i;
  for (i = 0; i < CACHE_BLOCKS; i++) {
    lock_acquire(&cache_blocks[i].block_lock);
    if (cache_blocks[i].valid && cache_blocks[i].dirty) {
      block_write(fs_device, cache_blocks[i].sector, cache_blocks[i].data);
      cache_blocks[i].dirty = false;
    }
    lock_release(&cache_blocks[i].block_lock);
  }

  /* Release the main cache lock */
  lock_release(&cache_lock);
}
