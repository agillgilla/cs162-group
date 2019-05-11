/* Test your buffer cache’s effectiveness by measuring its cache hit rate. */

#include <random.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

#define BLOCK_SIZE 512
#define BLOCK_COUNT 60

const char *file_name = "cache";
char buf[BLOCK_SIZE];

static void read_bytes(int fd);
// int get_hit_rate(void);

/* Read file */
static void
read_bytes(int f)
{
  int i = 0;
  size_t ret_val;
  for (; i < BLOCK_COUNT; i++) {
    ret_val = read (f, buf, BLOCK_SIZE);
    if (ret_val != BLOCK_SIZE)
      fail ("read %zu bytes in \"%s\" returned %zu",
            BLOCK_SIZE, file_name, ret_val);
  }
}

void
test_main(void)
{
  int fd;
  size_t i;
  random_init (0);
  random_bytes (buf, sizeof buf);

  /* Create input file */
  msg ("make \"%s\"", file_name);
  CHECK (create (file_name, 0), "create \"%s\"", file_name);
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);

  for (i=0; i < BLOCK_COUNT; i++) {
    size_t ret_val;
    ret_val = write (fd, buf, BLOCK_SIZE);
      if (ret_val != BLOCK_SIZE)
        fail ("write %zu bytes in \"%s\" returned %zu",
              BLOCK_SIZE, file_name, ret_val);
    }

  close (fd);
  msg ("close \"%s\"", file_name);

  /* Reset cache */
  cache_reset();
  msg ("reset buffer");

  /* Open file to read for the first time */
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
  read_bytes(fd);

  close (fd);
  msg ("close \"%s\"", file_name);

  /* Caculate cache hit rate for first round */
  int first_hit = get_cache_hit();
  int first_total = first_hit + get_cache_miss();
  int first_hit_rate = (first_hit / first_total + 1) * 100;

  /* Open file to read for the second time */
  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
  read_bytes(fd);

  close (fd);
  msg ("close \"%s\"", file_name);

  /* Caculate cache hit rate for second round */
  int second_hit = get_cache_hit();
  int second_total = second_hit + get_cache_miss();
  int second_hit_rate = ((second_hit - first_hit) / (second_total - first_total + 1)) * 100;

  if (second_hit_rate > first_hit_rate) {
    // msg ("New hit rate is higher than old hit rate");
    msg ("New hit rate is higher than old hit rate");

  }
}
