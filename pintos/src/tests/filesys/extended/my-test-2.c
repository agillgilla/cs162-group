/* Test your buffer cache’s effectiveness by measuring its cache hit rate. */

#include <random.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define BLOCK_SIZE 512
#define BLOCK_COUNT 32

const char *file_name1 = "myfile1";
const char *file_name2 = "myfile2";
const char *file_name3 = "myfile3";

char buf[BLOCK_SIZE];

static void read_bytes(int fd, int wd);

/* Read file */
static void
read_bytes(int f, int w)
{
  int i = 0;
  size_t ret_val;
  for (; i < BLOCK_COUNT; i++) {
    ret_val = read (f, buf, BLOCK_SIZE);
    if (ret_val != BLOCK_SIZE) {
      if (w == 1) {
        fail ("read %zu bytes in \"%s\" returned %zu",
              BLOCK_SIZE, file_name1, ret_val);
      } else if (w == 2) {
        fail ("read %zu bytes in \"%s\" returned %zu",
              BLOCK_SIZE, file_name2, ret_val);
      } else {
        fail ("read %zu bytes in \"%s\" returned %zu",
              BLOCK_SIZE, file_name3, ret_val);
      }
    }
  }
}

void
test_main(void)
{
  int fd;
  size_t i;
  random_init (0);
  random_bytes (buf, sizeof buf);

  /* ===== Create input file 1 =====*/
  msg ("make \"%s\"", file_name1);
  CHECK (create (file_name1, 0), "create \"%s\"", file_name1);
  CHECK ((fd = open (file_name1)) > 1, "open \"%s\"", file_name1);
  for (i=0; i < BLOCK_COUNT; i++) {
    size_t ret_val;
    ret_val = write (fd, buf, BLOCK_SIZE);
      if (ret_val != BLOCK_SIZE)
        fail ("write %zu bytes in \"%s\" returned %zu",
              BLOCK_SIZE, file_name1, ret_val);
    }
  close (fd);
  msg ("close \"%s\"", file_name1);

  /* ===== Create input file 2 ====== */
  msg ("make \"%s\"", file_name2);
  CHECK (create (file_name2, 0), "create \"%s\"", file_name2);
  CHECK ((fd = open (file_name2)) > 1, "open \"%s\"", file_name2);
  for (i=0; i < BLOCK_COUNT; i++) {
    size_t ret_val;
    ret_val = write (fd, buf, BLOCK_SIZE);
      if (ret_val != BLOCK_SIZE)
        fail ("write %zu bytes in \"%s\" returned %zu",
              BLOCK_SIZE, file_name2, ret_val);
    }
  close (fd);
  msg ("close \"%s\"", file_name2);

  /* ===== Create input file 3 =====*/
  msg ("make \"%s\"", file_name3);
  CHECK (create (file_name3, 0), "create \"%s\"", file_name3);
  CHECK ((fd = open (file_name3)) > 1, "open \"%s\"", file_name3);
  for (i=0; i < BLOCK_COUNT; i++) {
    size_t ret_val;
    ret_val = write (fd, buf, BLOCK_SIZE);
      if (ret_val != BLOCK_SIZE)
        fail ("write %zu bytes in \"%s\" returned %zu",
              BLOCK_SIZE, file_name3, ret_val);
    }
  close (fd);
  msg ("close \"%s\"", file_name3);

  /* Read file 2, should be in cache and get a cache hit*/
  msg ("read \"%s\"", file_name2);
  CHECK ((fd = open (file_name2)) > 1, "open \"%s\"", file_name2);
  read_bytes(fd, 2);
  close (fd);
  msg ("close \"%s\"", file_name2);
  int first_hit = get_cache_miss();
  if (first_hit > 0) {
    msg("Correctly hit cache on read");
  }

  /* Read file 1, should not be in cache and get a cache miss*/
  msg ("read \"%s\"", file_name1);
  int currentMisses = get_cache_miss();
  CHECK ((fd = open (file_name1)) > 1, "open \"%s\"", file_name1);
  read_bytes(fd, 1);
  close (fd);
  msg ("close \"%s\"", file_name1);

  // Check if first file correctly hit cache
  int second_hit = get_cache_miss();
  if (second_hit > currentMisses) {
    msg("Correctly fetched from disk");
    currentMisses = second_hit;
  }


  remove("myfile1");
  remove("myfile2");
  remove("myfile3");
}
