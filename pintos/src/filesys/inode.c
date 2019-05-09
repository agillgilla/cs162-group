#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* Number of direct pointers in inode_disk */
#define DIRECT_PTRS 122

/* Number of pointers in a block pointed to by an indirect pointer */
#define INDIRECT_BLOCK_PTRS 128

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk {
  block_sector_t direct_ptrs[DIRECT_PTRS];  /* Array of direct pointers */
  block_sector_t indirect_ptr;              /* Singly indirect pointer */
  block_sector_t doubly_indirect_ptr;       /* Doubly indirect pointer */

  bool directory;                           /* True if inode is directory. */
  struct inode *parent_node;			    /* Link to parent directory. */
    
  off_t length;                             /* File size in bytes. */
  unsigned magic;                           /* Magic number. */
};

/* A block of pointers to direct blocks (pointed at by an indirect pointer. */
struct indirect_block {
	block_sector_t block_ptrs[INDIRECT_BLOCK_PTRS];
};

static bool allocate_indirect_block(struct indirect_block *block, off_t start_index, off_t stop_index);
static bool inode_alloc(struct inode_disk *inode_d);
static bool inode_dealloc(struct inode *inode);
static bool inode_extend(struct inode_disk *inode_d, off_t length);

/* Allocates indirect pointers in the indirect_block passed in from start_index
   to stop_index, inclusive. Returns true on success and false on failure. */
static bool
allocate_indirect_block(struct indirect_block *block, off_t start_index, off_t stop_index) {
	ASSERT(start_index >= 0);
	ASSERT(stop_index < INDIRECT_BLOCK_PTRS);

	static char empty[BLOCK_SECTOR_SIZE];

	if (block == NULL) {
		return false;
	}
	
	off_t i;
	for (i = start_index; i <= stop_index; i++) {
		/* Allocate a new indirect block (block of pointers) */
		if (!free_map_allocate(1, &block->block_ptrs[i])) {
			return false;
		}

		/* Zero out indirect block */
		block_write(fs_device, block->block_ptrs[i], empty);
	}
	
	return true;
}


/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
  };



/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);

  /* TODO: Change this to a buffer cache lookup later */
  struct inode_disk inode_d = inode->data;

  if (pos > inode_d.length || pos < 0) {
	  return -1;
  }

  /* Convert the byte position to a block index */
  off_t block_index = pos / BLOCK_SECTOR_SIZE;

  /* Bases and limits of direct pointers, indirect pointer, and doubly indirect pointer*/
  off_t direct_base = 0;
  off_t direct_limit = direct_base + DIRECT_PTRS;
  off_t indirect_base = direct_limit;
  off_t indirect_limit = indirect_base + INDIRECT_BLOCK_PTRS;
  off_t doubly_indirect_base = indirect_limit;
  off_t doubly_indirect_limit = doubly_indirect_base + INDIRECT_BLOCK_PTRS * INDIRECT_BLOCK_PTRS;


  if (block_index < direct_limit) {
	  return inode_d.direct_ptrs[block_index];
  } else if (block_index < indirect_limit) {
	  /* Calculate the index of the direct pointer within the indirect block */
	  off_t direct_index_in_indirect = block_index - direct_limit;
	  
	  /* Declare and allocate indirect block */
	  struct indirect_block *inode_indirect;
	  inode_indirect = calloc(1, sizeof(struct indirect_block));
	  
	  /* Read the indirect block in from disk (Change this to buffer cache read later) */
	  block_read(fs_device, inode_d.indirect_ptr, inode_indirect);
	  
	  /* Get the sector number from the direct pointers array */
	  block_sector_t sector = inode_indirect->block_ptrs[direct_index_in_indirect];
	  
	  /* Free the indirect inode struct */
	  free(inode_indirect);
	  /* Return the sector number */
	  return sector;
  } else if (block_index < doubly_indirect_limit) {
	  /* Calculate the index of the indirect pointer within the doubly indirect block */
	  off_t indirect_index_in_doubly_indirect = (block_index - indirect_limit) / INDIRECT_BLOCK_PTRS;
	  /* Calculate the index of the direct pointer within the indirect block */
	  off_t direct_index_in_indirect = (block_index - indirect_limit) % INDIRECT_BLOCK_PTRS;

	  /* Declare and allocate indirect block */
	  struct indirect_block *inode_indirect;
	  inode_indirect = calloc(1, sizeof(struct indirect_block));

	  /* Read the doubly indirect block in from disk (Change this to buffer cache read later) */
	  block_read(fs_device, inode_d.doubly_indirect_ptr, inode_indirect);

	  /* Read the indirect block in from disk (Change this to buffer cache read later) */
	  block_read(fs_device, inode_indirect->block_ptrs[indirect_index_in_doubly_indirect], inode_indirect);

	  /* Get the sector number from the direct pointers array */
	  block_sector_t sector = inode_indirect->block_ptrs[direct_index_in_indirect];

	  /* Free the indirect inode struct */
	  free(inode_indirect);
	  /* Return the sector number */
	  return sector;
  } else {
	  return -1;
  }
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
}

/* Returns if an inode is a directory. */
bool
inode_is_dir(struct inode *inode) {
  return (inode->data).directory;
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool directory)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  //printf("Call to inode_create at sector %zu \n", sector);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      //disk_inode->length = length;
      disk_inode->directory = directory;
      disk_inode->magic = INODE_MAGIC;
      /*
  	  if (free_map_allocate (sectors, &disk_inode->start))
          {
            block_write (fs_device, sector, disk_inode);
            if (sectors > 0)
              {
                static char zeros[BLOCK_SECTOR_SIZE];
                size_t i;

                for (i = 0; i < sectors; i++)
                  block_write (fs_device, disk_inode->start + i, zeros);
              }
            success = true;
          }
  		*/

  	  success = inode_alloc(disk_inode);

  	  success = inode_extend(disk_inode, length) && success;

  	  block_write(fs_device, sector, disk_inode);

      //printf("Just called inode_create and wrote to disk\n");

      free (disk_inode);
    }

  if (directory) {
    //printf("Call to inode_create created directory returned success code %s \n", success?"true":"false");
  }
  

  return success;
}

/* Allocates initial memory for an inode_disk.  This creates
   valid pointers to indirect_blocks that are pointed to by
   the indirect_ptr and doubly_indirect_ptr. */
static bool
inode_alloc(struct inode_disk *inode_d) {
	static char empty[BLOCK_SECTOR_SIZE];

	/* Allocate a new indirect block (block of direct pointers) */
	if (!free_map_allocate(1, &inode_d->indirect_ptr)) {
		return false;
	}
	/* Zero out indirect block */
	block_write(fs_device, inode_d->indirect_ptr, empty);

	/* Allocate new doubly indirect block (block of indirect pointers) */
	if (!free_map_allocate(1, &inode_d->doubly_indirect_ptr)) {
		return false;
	}
	/* Zero out indirect block */
	block_write(fs_device, inode_d->doubly_indirect_ptr, empty);

	return true;
}

/* Deallocates all memory for an inode.  This iterates through
   the multi level indirect blocks and stops according to the
   length of the inode. */
static bool
inode_dealloc(struct inode *inode) {
	/* TODO: Change this to read from disk when we remove data member */
	struct inode_disk inode_d = inode->data;

	/* Calculate the current number of blocks allocated */
	size_t curr_num_blocks = bytes_to_sectors(inode_d.length);

	/* Calculate the current number of blocks allocated (number of blocks to deallocate) */
	size_t blocks_to_deallocate = bytes_to_sectors(inode_d.length);

	/* Bases and limits of direct pointers and indirect pointers */
	off_t direct_base = 0;
	off_t direct_limit = direct_base + DIRECT_PTRS;
	off_t indirect_base = direct_limit;
	off_t indirect_limit = indirect_base + INDIRECT_BLOCK_PTRS;

	off_t i;
	for (i = 0; i < DIRECT_PTRS; i++) {
		inode_d.direct_ptrs[i] = 0;
		free_map_release(inode_d.direct_ptrs[i], 1);

		blocks_to_deallocate--;
		if (blocks_to_deallocate <= 0) {
			return true;
		}
	}

	/* Check if we need to deallocate the indirect block */
	if (curr_num_blocks > direct_limit) {
		struct indirect_block *inode_indirect = calloc(1, sizeof(struct indirect_block));

		/* Read the indirect block in from disk */
		block_read(fs_device, inode_d.indirect_ptr, inode_indirect);

		off_t i;
		for (i = 0; i < INDIRECT_BLOCK_PTRS; i++) {
			inode_indirect->block_ptrs[i] = 0;
			/* Deallocate the direct block */
			free_map_release(inode_indirect->block_ptrs[i], 1);

			blocks_to_deallocate--;
			if (blocks_to_deallocate <= 0) {
				/* Deallocate the indirect block */
				free_map_release(inode_d.indirect_ptr, 1);

				/* Free the indirect_block struct */
				free(inode_indirect);

				return true;
			}
		}
		/* Free the indirect_block struct */
		free(inode_indirect);
	}

	/* Check if we need to deallocate the doubly indirect block */
	if (curr_num_blocks > indirect_limit) {
		struct indirect_block *doubly_indirect = calloc(1, sizeof(struct indirect_block));

		/* Read the indirect block in from disk */
		block_read(fs_device, inode_d.doubly_indirect_ptr, doubly_indirect);

		off_t i;
		for (i = 0; i < INDIRECT_BLOCK_PTRS; i++) {

			struct indirect_block *inode_indirect = calloc(1, sizeof(struct indirect_block));

			/* Read the indirect block in from disk */
			block_read(fs_device, doubly_indirect->block_ptrs[i], inode_indirect);

			off_t j;
			for (j = 0; j < INDIRECT_BLOCK_PTRS; j++) {
				inode_indirect->block_ptrs[j] = 0;
				free_map_release(inode_indirect->block_ptrs[j], 1);

				blocks_to_deallocate--;
				if (blocks_to_deallocate <= 0) {
					/* Deallocate the (second level) indirect block */
					free_map_release(doubly_indirect->block_ptrs[i], 1);

					/* Free the indirect_block struct */
					free(inode_indirect);

					/* Deallocate the doubly indirect block */
					free_map_release(inode_d.doubly_indirect_ptr, 1);
					
					/* Free the doubly indirect block */
					free(doubly_indirect);

					return true;
				}
			}

			/* Deallocate the (second level) indirect block */
			free_map_release(doubly_indirect->block_ptrs[i], 1);

			/* Free the indirect_block struct */
			free(inode_indirect);
		}
	}
	
  return true;
}

/* Increases the available length of inode to the argument
   length provided. Returns true on success and false on 
   failure. */
static bool
inode_extend(struct inode_disk *inode_d, off_t length)
{
	ASSERT(inode_d != NULL);
	ASSERT(length >= 0);

	/* Calculate the current number of blocks and new number of blocks */
	size_t curr_num_blocks = bytes_to_sectors(inode_d->length);
	size_t new_num_blocks = bytes_to_sectors(length);

  //printf("curr_num_blocks: %zd\n", curr_num_blocks);
  //printf("new_num_blocks: %zd\n", new_num_blocks);
  
  //printf("First direct sector number: %zu\n", inode_d->direct_ptrs[0]);
  //printf("Old length: %zu, New length: %zu\n", inode_d->length, length);
	
  if (new_num_blocks < curr_num_blocks) {
		/* This function is only for extending (not shortening) inodes */

    //printf("NEW BLOCKS LESS THAN CURR BLOCKS!\n");
		return false;
	} else if (new_num_blocks == curr_num_blocks) {
		/* Nothing needs to be done */
		return true;
	}

	static char empty[BLOCK_SECTOR_SIZE];

	/* Make sure that the length arg isn't more than maximum possible size */
	off_t max_inode_size = (DIRECT_PTRS + INDIRECT_BLOCK_PTRS + INDIRECT_BLOCK_PTRS * DIRECT_PTRS) * BLOCK_SECTOR_SIZE;
	if (length > max_inode_size || length < inode_d->length) {
		return false;
	}

	/* Bases and limits of direct pointers, indirect pointer, and doubly indirect pointer*/
	size_t direct_base = 0;
	size_t direct_limit = direct_base + DIRECT_PTRS;
	size_t indirect_base = direct_limit;
	size_t indirect_limit = indirect_base + INDIRECT_BLOCK_PTRS;
	size_t doubly_indirect_base = indirect_limit;
	size_t doubly_indirect_limit = doubly_indirect_base + INDIRECT_BLOCK_PTRS * INDIRECT_BLOCK_PTRS;

	/* Check if we need to point more direct blocks to memory (and do it
	if we need to) */
	if (curr_num_blocks < direct_limit) {
		if (new_num_blocks >= direct_limit) {
			/* New last block uses all direct pointers, allocate them all */
			unsigned i;
			for (i = curr_num_blocks; i < direct_limit; i++) {
				if (!free_map_allocate(1, &inode_d->direct_ptrs[i])) {
					return false;
				}
				block_write(fs_device, inode_d->direct_ptrs[i], empty);
				//printf("%s\n", "Zeroed out a block.");
			}
			/* We need to update curr_num_blocks for future allocations */
			curr_num_blocks = direct_limit;
		} else {
			/* New last block doesn't use all direct pointers */
			unsigned i;
			for (i = curr_num_blocks; i < new_num_blocks; i++) {
        //printf("i = %lu, new_num_blocks = %zd\n", i, new_num_blocks);
				if (!free_map_allocate(1, &inode_d->direct_ptrs[i])) {
					return false;
				}
				block_write(fs_device, inode_d->direct_ptrs[i], empty);

        //printf("%s : %zu\n", "Allocating block sector", inode_d->direct_ptrs[i]);
        //printf("%s\n", "Zeroed out a block.");
			}
			/* We don't need to update curr_num_blocks since there will be no more
			allocation after this point */
		}
	}

	/* Check if we need another direct pointer in our indirect block */
	if (curr_num_blocks < indirect_limit && new_num_blocks > direct_limit) {
		
		struct indirect_block *inode_indirect = calloc(1, sizeof(struct indirect_block));

		/* Initial singly and doubly indirect blocks are pre allocated
		if (inode_d->indirect_ptr == 0) {
			// We need to allocate a new indirect block
			if (!free_map_allocate(1, &inode_d->indirect_ptr) {
				return false;
			}
		} else {
			// The indirect block already exists, read it in from disk
			block_read(fs_device, inode_d->indirect_ptr, inode_indirect)
		}
		*/

		/* Read the indirect block in from disk */
		block_read(fs_device, inode_d->indirect_ptr, inode_indirect);

		if (new_num_blocks >= indirect_limit) {
			/* New last block fills entire indirect block, allocate all */
			unsigned i;
			for (i = curr_num_blocks - indirect_base; i < indirect_limit - indirect_base; i++) {
				if (!free_map_allocate(1, &inode_indirect->block_ptrs[i])) {
					return false;
				}
				block_write(fs_device, inode_indirect->block_ptrs[i], empty);
			}
			/* We need to update curr_num_blocks for future allocations */
			curr_num_blocks = indirect_limit;
		} else {
			/* New last block doesn't use all direct pointers in indirect block */
			unsigned i;
			for (i = curr_num_blocks - indirect_base; i < new_num_blocks - indirect_base; i++) {
				if (!free_map_allocate(1, &inode_indirect->block_ptrs[i])) {
					return false;
				}
				block_write(fs_device, inode_indirect->block_ptrs[i], empty);
			}
			/* We don't need to update curr_num_blocks since there will be no more
			allocation after this point */
		}

		/* Write the indirect block back to disk */
		block_write(fs_device, inode_d->indirect_ptr, inode_indirect);

		/* Free the temporary indirect block struct */
		free(inode_indirect);
	}

	/* Check if we need to allocate more in doubly indirect block (either another
	indirect pointer in the doubly indirect block or a direct pointer in an indirect
	block pointed at from an indirect pointer in the doubly indirect block) (or both) */
	/* Precondition: curr_num_blocks >= indirect_limit */
	if (curr_num_blocks < doubly_indirect_limit && new_num_blocks > indirect_limit) {

		/* Calculate the index of the new last indirect pointer within the doubly indirect block */
		off_t new_first_level_index = DIV_ROUND_UP(new_num_blocks - indirect_limit, INDIRECT_BLOCK_PTRS) - 1;
		/* Calculate the index of the new last direct pointer within the indirect block */
		off_t new_second_level_index = (new_num_blocks - indirect_limit) % INDIRECT_BLOCK_PTRS - 1;

		/* Calculate the index of the current last indirect pointer within the doubly indirect block */
		off_t curr_first_level_index = DIV_ROUND_UP(curr_num_blocks - indirect_limit, INDIRECT_BLOCK_PTRS) - 1;
		/* Calculate the index of the current last direct pointer within the indirect block */
		off_t curr_second_level_index = (curr_num_blocks - indirect_limit) % INDIRECT_BLOCK_PTRS - 1;

		/* Start and end of allocation in first level block of indirect pointers (doubly indirect block) */
		off_t min_first_level = curr_first_level_index + 1;
		off_t max_first_level = new_first_level_index;

		struct indirect_block *doubly_indirect = calloc(1, sizeof(struct indirect_block));
		/* Read doubly indirect block in from disk */
		block_read(fs_device, inode_d->doubly_indirect_ptr, doubly_indirect);
		/* Allocate first level indirect pointers as necessary (in doubly indirect block) */
		if (!allocate_indirect_block(doubly_indirect, min_first_level, max_first_level)) {
			return false;
		}

		off_t i;
		for (i = min_first_level; i <= max_first_level; i++) {
			/* By default fill whole indirect block with direct block pointers */
			off_t min = 0;
			off_t max = INDIRECT_BLOCK_PTRS;

			if (i == min_first_level) {
				/* First iteration, start at the current block index in indirect block */
				min = curr_second_level_index + 1;
			}
			if (i == max_first_level) {
				/* Last iteration, end at new last block index in indirect block */
				max = new_second_level_index;
			}
			
			struct indirect_block *indirect_block = calloc(1, sizeof(struct indirect_block));
			/* Read indirect block in from disk */
			block_read(fs_device, doubly_indirect->block_ptrs[i], indirect_block);
			/* Allocate second level direct pointers as necessary (singly indirect block) */
			if (!allocate_indirect_block(indirect_block, min, max)) {
				return false;
			}
			/* Write the indirect block out to disk */
			block_write(fs_device, doubly_indirect->block_ptrs[i], indirect_block);
			/* Free the struct */
			free(indirect_block);
		}
		/* Write the doubly indirect block out to disk */
		block_write(fs_device, inode_d->doubly_indirect_ptr, doubly_indirect);
		/* Free the struct */
		free(doubly_indirect);
	}

	inode_d->length = length;
  //printf("FINISHED SUCCESSFULLY\n");
	return true;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          inode_reopen (inode);
          return inode;
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  block_read (fs_device, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      /* Deallocate blocks if removed. */
      if (inode->removed)
        {
          free_map_release (inode->sector, 1);

		  inode_dealloc(inode);
          //free_map_release (inode->data.start, bytes_to_sectors (inode->data.length));
        }

      free (inode);
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          block_read (fs_device, sector_idx, buffer + bytes_read);

          //printf("%s : %zu\n", "Reading block sector", sector_idx);
        }
      else
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL)
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;

  /* TODO: Change this to a buffer cache lookup later */
  struct inode_disk inode_d = inode->data;

  /* The new length is greater than current length, extend it */
  //if (offset + size - 1 > inode_d.length) {
  if (byte_to_sector(inode, offset + size - 1) == -1) {
    //printf("Extending inode of length %zu to length: %zu...\n", inode_d.length, offset + size);
    /* Extend the length of the inode */
    if (!inode_extend(&inode->data, offset + size)) {
      //printf("Fail on inode_extend, returning 0\n");
      return 0;
    }

    inode->data.length = offset + size;
    //printf("Just set inode length to %zu\n", offset + size);

    /* Write the inode_disk back to disk */
    block_write(fs_device, inode->sector, &inode->data);
  }

  //printf("Executing write of size %zu at offset %zu in inode of length: %zu...\n", size, offset, inode->data.length);

  while (size > 0)
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else
        {
          /* We need a bounce buffer. */
          if (bounce == NULL)
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left)
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

  //printf("Bytes written: %zu\n", bytes_written);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

/* Set dest_inode's data disk_node parent directory to parent_inode. */
void
inode_set_parent(struct inode *dest_inode, const struct inode *parent_inode) 
{
	dest_inode->data.parent_node = parent_inode;
}

void
inode_set_disknode_directory(struct inode *inode, bool is_dir)
{
	inode->data.directory = is_dir;
}
