#include "filesys/buffer.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "devices/block.h"

/* Buffer cache with a maximum capacity of 64 disk blocks */
#define CACHE_BLOCKS 64;

struct cache_block *cache_blocks[CACHE_BLOCKS];   /* Array of cache_blocks */
struct lock cache_lock;                           /* Lock for cache_block synchronization */
unsigned clock_index;                             /* Current position of the clock hand for clock algorithm */


struct cache_block {
    block_sector_t sector;    /* Sector on disk that this cache is for */
    void *data;               /* Raw data from sector in cache */
    struct lock block_lock;   /* Lock for synchronization */
    bool valid;               /* Valid bit (set false on init, always true after) */
    bool dirty;               /* Dirty bit */
    bool recently_used;       /* Flag for clock algorithm (evict if false) */
};

void
filesys_cache_init(void)
{
  lock_init(&cache_lock);
  clock_index = 0;
  cache_miss = 0;
  cache_hit = 0;

  for (i=0 ;i < CACHE_BLOCKS; i++)
    lock_init(cache_blocks[i] -> &block_lock)
    cache_blocks[i] -> valid = false;
    cache_block[i] -> dirty = false;
}

void
cache_read_at(block_sector_t sector, void void *buffer, off_t size, off_t block_ofs)
{
  *void cached;
  for (i=0; i < CACHE_BLOCKS; i++)
    /* read data into buffer */
    if (cache_blocks[i]->valid && cache_blocks[i]->sector == sector) {
      cashed_blocks[i]-> lock_acquire(cashed_blocks[clock_index]->&block_lock)
      memcpy (buffer, cache_blocks[i]->data, BLOCK_SECTOR_SIZE);
      cached_blocks[i]->recently_used = true;
      cashed_blocks[i]-> lock_release(cashed_blocks[clock_index]->&block_lock);

      cashed = true;
      cache_hit ++;
      break;
      }

  /* Run clock algorithm to find an entry */
    while(!cashed) {
      if (cache_blocks[clock_index]->recently_used) {
          cache_blocks[clock_index]->recently_used = false;
        } else {
          cashed_blocks[clock_index]-> lock_acquire(cashed_blocks[clock_index]->&block_lock)
          memcpy (buffer, cache_blocks[i]->data, BLOCK_SECTOR_SIZE);
          cached_blocks[clock_index]->recently_used = true;
          cashed_blocks[clock_index]-> lock_release(cashed_blocks[clock_index]->&block_lock);
          cashed = true;
        }

      if (clock_index == CACHE_BLOCKS) {
        clock_index = 0
      } else {
        clock_index ++;
      }
      cache_miss ++;
    }
}

void
cache_write_at(block_sector_t sector, const void *buffer, off_t size, off_t block_ofs)
{
  //boolean indicating whether the sector we are trying to access is already cached
  bool cached = false;

  //Check if the sector is already in the cache
  //keep track of first invalid bit if it exists
  int invalidIndex = -1;
  for (int i = 0; i < CACHE_BLOCKS; i++) {
    cache_lock->lock_acquire(cached_blocks[i]->&block_lock);
    //If valid bit is false, and it is the first one we see, keep track of the index so we can quickly pull from disk later
    if (cache_blocks[i]->valid == false && invalidIndex == -1) {
      invalidIndex = i;
    }

    //If valid bit is true, and the sector matches, then it means we have a cache hit
    if (cached_blocks[i]->valid == true && cache_blocks[i]->sector == sector) {
      memcpy(cache_blocks[i]->data, buffer, BLOCK_SECTOR_SIZE);
      cached_blocks[i]->recently_used = true;
      cached_blocks[i]->dirty = true;
      cached = true;
      cache_hit++;

    }
    cache_lock->lock_release(cached_blocks[i]->&block_lock);

  }

  //If there is invalid bit and cached is false, simply write to it, and set valid to true
  if (invalidIndex != -1 && cached == false) {
    cache_lock->lock_acquire(cached_blocks[invalidIndex]->&block_lock);
    memcpy(cache_blocks[invalidIndex]->data, buffer, BLOCK_SECTOR_SIZE);
    cached_blocks[invalidIndex]->recently_used = true;
    cached_blocks[invalidIndex]->dirty = true;
    cached_blocks[invalidIndex]->valid = true;
    cached_blocks[invalidIndex]->sector = sector;
    cache_lock->lock_release(cached_blocks[invalidIndex]->&block_lock);
    cached = true;
    cache_miss++;
  }

  //Else, if the cache was still not successful but no invalid blocks, then we need clock to replace
  while(cached == false) {
    cache_lock->lock_acquire(cached_blocks[clock_index]->&block_lock);
    //If recently used, don't evict
    if (cache_blocks[clock_index]->recently_used == true) {
      cache_blocks[clock_index]->recently_used = false;
      clock_index = (clock_index + 1) % BLOCK_SECTOR_SIZE;
    //if not recently used, evict
    } else {
      //if evicted block is dirty, write it to disk
      if (cache_blocks[clock_index]->dirty == true) {
        block_write(fs_device, cache_blocks[clock_index]->sector, cache_blocks[clock_index]->data);
      }
      //write changes to cache
      memcpy(cache_blocks[clock_index]->data, buffer, BLOCK_SECTOR_SIZE);
      cached_blocks[clock_index]->recently_used = true;
      cached_blocks[clock_index]->dirty = true;
      cached_blocks[clock_index]->valid = true;
      cached_blocks[clock_index]->sector = sector;
      cached = true;
    }
    cache_lock->lock_release(cached_blocks[clock_index]->&block_lock);
    cache_miss++;
  }
}

void
cache_flush(void)
{
}