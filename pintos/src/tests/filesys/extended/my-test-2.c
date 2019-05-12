/* Test your buffer cache’s effectiveness by measuring its cache hit rate. */

#include <random.h>
#include <syscall.h>
#include <stdio.h>
#include "tests/lib.h"
#include "tests/main.h"

#define BLOCK_SIZE 512
#define BLOCK_COUNT 64

char buf[BLOCK_SIZE];

static void read_bytes(int fd);

/*random file name*/

/* Read file */
static void
read_bytes(int f)
{
  size_t ret_val;
  ret_val = read (f, buf, BLOCK_SIZE);
}

void
test_main(void)
{
  int fd;

  char *first_name;
  char *second_name;
  random_init (0);
  random_bytes (buf, sizeof buf);

  /* Create 64 files with different names, and random contents to fill up cache*/
  msg ("make 64");
  size_t i;
  for (i=0; i < BLOCK_COUNT; i++) {
    char *file_name;
    snprintf(file_name, "file%zu", 5, i);
    CHECK (create (file_name, 0), "create \"%s\"", file_name);
    CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);

    size_t ret_val;
    ret_val = write (fd, buf, BLOCK_SIZE);
    if (ret_val != BLOCK_SIZE) {
      fail ("write %zu bytes in \"%s\" returned %zu", BLOCK_SIZE, file_name, ret_val);
    }
    close (fd);
  }
  msg ("close 64");

  if(get_cache_miss() == 64) {
    msg("Correctly filled cold buffer to brim");
  } else {
    msg("Error filling cache");
  }

  /* Open first file to read, should get a hit */
  first_name = "file1";
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
  read_bytes(fd);
  close (fd);
  msg ("close \"%s\"", file_name);
  // Output if correctly
  if(get_cache_hit() == 1) {
    msg("Correctly got cache hit");
  } else {
    msg("Did not get cache hit");
  }

  /* Open a new file, should evict second file */
  int prev_cache_miss = get_cache_miss;
  char *evict_file;
  evict_file = "evict";
  CHECK (create (file_name, 0), "create \"%s\"", file_name);
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);

  size_t ret_val;
  ret_val = write (fd, buf, BLOCK_SIZE);
  if (ret_val != BLOCK_SIZE) {
    fail ("write %zu bytes in \"%s\" returned %zu", BLOCK_SIZE, file_name, ret_val);
  }
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
  read_bytes(fd);
  close (fd);

  if (get_cache_miss() == prev_cache_miss + 1) {
    msg("Correctly brought in new file to cache")
  } else {
    msg("Did not miss on new file")
  }

  /* Try to read second file and see if it is properly evicted */
  prev_cache_miss = get_cache_miss();
  second_name = "file2";
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
  read_bytes(fd);
  close (fd);

  if (get_cache_miss() == prev_cache_miss + 1) {
    msg("Correctly brought in new file to cache")
  } else {
    msg("Did not miss on new file")
  }

}
