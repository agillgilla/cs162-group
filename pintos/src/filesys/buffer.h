#ifndef FILESYS_BUFFER_H
#define FILESYS_BUFFER_H

#include <stdbool.h>
#include "devices/block.h"
#include "filesys/off_t.h"
#include "threads/synch.h"

struct cache_block {
    block_sector_t sector;             /* Sector on disk that this cache is for */
    uint8_t data[BLOCK_SECTOR_SIZE];   /* Raw data from sector in cache */
    struct lock block_lock;            /* Lock for synchronization */
    bool valid;                        /* Valid bit (set false on init, always true after) */
    bool dirty;                        /* Dirty bit */
    bool recently_used;                /* Flag for clock algorithm (evict if false) */
};

void filesys_cache_init(void);
void cache_read_at(block_sector_t sector, void *buffer);
void cache_write_at(block_sector_t sector, const void *buffer);
void cache_flush(void);

#endif  /* filesys/buffer.h */
