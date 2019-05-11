#ifndef FILESYS_BUFFER_H
#define FILESYS_BUFFER_H

#include <stdbool.h>
#include "devices/block.h"
#include "filesys/off_t.h"


size_t cache_miss;
size_t cache_hit;

void filesys_cache_init(void);
void cache_read_at(block_sector_t sector, void *buffer, off_t size, off_t block_ofs);
void cache_write_at(block_sector_t sector, const void *buffer, off_t size, off_t block_ofs);
void cache_flush(void);

#endif  /* filesys/buffer.h */
