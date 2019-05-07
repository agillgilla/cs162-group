#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format)
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format)
    do_format ();

  struct inode *root_node = inode_open(ROOT_DIR_SECTOR);
  thread_current()->working_dir = root_node;


  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void)
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size, bool is_dir)
{
  block_sector_t inode_sector = 0;
  struct dir *dir = dir_open_root ();
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, is_dir)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0)
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  char file_name[NAME_MAX + 1];
  struct dir *dir = dir_open_root();//try_get_dir(name, file_name) == NULL?  
  struct inode *inode = NULL;
  
  if (dir != NULL)
    dir_lookup(dir, file_name, &inode);
  dir_close (dir);

  return file_open (inode);
}

/* Extracts a file name part from *SRCP into PART, and updates *SRCP so that the
   next call will return the next file name part. Returns 1 if successful, 0 at
   end of string, -1 for a too-long file name part.
   Function is given in project 3 spec. */
static int
get_next_part(char part[NAME_MAX + 1], const char **srcp) {
	const char *src = *srcp;
	char *dst = part;

	/* Skip leading slashes. If it's all slashes, we're done. */
	while (*src == '/')
		src++;
	if (*src == '\0')
		return 0;

	/* Copy up to NAME_MAX chars from SRC to DST. Add null terminator. */
	while (*src != '/' && *src != '\0') {
		if (dst < part + NAME_MAX)
			*dst++ = *src;
		else
			return -1;
		src++;
	}
	*dst = '\0';

	/* Advance source pointer. */
	*srcp = src;
	return 1;
}

bool
filesys_chdir(const char *syscall_arg)
{
	return false;
}

/* Attempt to find the directory where the file exists.
   Return dir if it exists, else return NULL. */
struct dir *
try_get_dir(const char *file_path, char next_part[NAME_MAX + 1]) {
	if (*file_path == '\0')
		return NULL;
	
	//char next_part[NAME_MAX + 1];
	struct inode *cur_inode = rel_to_abs(file_path);

	while (get_next_part(next_part, &file_path) > 0 && cur_inode != NULL)
	{
		struct dir *cur_dir = dir_open(cur_inode);
		if (dir_lookup(cur_dir, next_part, &cur_inode))
			dir_close(cur_dir);
		else
			break;
	}

	if (inode_is_dir(cur_inode))
		strlcpy(next_part, ".", sizeof(char) * 2);
	return dir_open(cur_inode);
}

/* Get the correct directory inode according to the given file_path. */
struct inode *
rel_to_abs(const char *file_path)
{
	if (file_path[0] == '/')
		return dir_get_inode(dir_open_root());
	return dir_get_inode(thread_current()->working_dir);
}


/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name)
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir);

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
