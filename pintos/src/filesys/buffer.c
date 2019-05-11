#include "filesys/buffer.h"
#include "threads/synch.h"

/* Buffer cache with a maximum capacity of 64 disk blocks */
#define CACHE_BLOCKS 64

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
  int i;
  for (i=0 ;i < CACHE_BLOCKS; i++)
    lock_init(&cache_blocks[i] -> block_lock);
    cache_blocks[i] -> valid = false;
    cache_blocks[i] -> dirty = false;
}

void
cache_read_at(block_sector_t sector, void *buffer, off_t size, off_t block_ofs)
{
  bool cached = false;
  int i;
  for (i=0; i < CACHE_BLOCKS; i++) {
    /* read data into buffer */
    if (cache_blocks[i]->valid && cache_blocks[i]->sector == sector) {
      lock_acquire(&cache_blocks[clock_index]->block_lock);
      memcpy (buffer, cache_blocks[i]->data, BLOCK_SECTOR_SIZE);
      cache_blocks[i]->recently_used = true;
      lock_release(&cache_blocks[clock_index]->block_lock);

      cached = true;
      cache_hit ++;
      break;
      }
    }

  /* Run clock algorithm to find an entry */
    while(!cached) {
      if (cache_blocks[clock_index]->recently_used) {
          cache_blocks[clock_index]->recently_used = false;
        } else {
          lock_acquire(&cache_blocks[clock_index]->block_lock);
          memcpy (buffer, cache_blocks[i]->data, BLOCK_SECTOR_SIZE);
          cache_blocks[clock_index]->recently_used = true;
          lock_release(&cache_blocks[clock_index]->block_lock);
          cached = true;
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

}

void
cache_flush(void)
{
  // for (i = 0; i < CACHE_BLOCKS; i++) {
  //   if (cache_blocks[i]->valid && cache_blocks[i]->dirty) {
  //     block_write(fs_device, cache_blocks[i]->sector, cache_blocks[i]->data)
  //   }
  // }
}
