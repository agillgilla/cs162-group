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
static int lcm(int a, int b);
static int gcd(int a, int b);
static bool frac_greater_than(int num_a, int denom_a, int num_b, int denom_b);

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
  int first_miss = get_cache_miss();
  int first_total = first_hit + first_miss;
  int first_hit_rate = (first_hit / first_total + 1) * 100;

  CHECK ((fd = open (file_name)) > 1, "open \"%s\"", file_name);
  read_bytes(fd);

  close (fd);
  msg ("close \"%s\"", file_name);

  /* Caculate cache hit rate for second round */
  int second_hit = get_cache_hit() - first_hit;
  int second_miss = get_cache_miss() - first_miss;

  int second_total = second_hit + second_miss;
  int second_hit_rate = ((second_hit - first_hit) / (second_total - first_total + 1)) * 100;

  remove("cache");

  if (frac_greater_than(second_hit, second_total, first_hit, first_total)) {
    // msg ("New hit rate is higher than old hit rate");
    msg ("New hit rate is higher than old hit rate");
  } else {
    msg ("New hit rate is NOT higher than old hit rate");
  }
}

/* Returns true if fraction A is greater than fraction
   B.  Requres args: numerator of A, denominator of A,
   numerator of B, denominator of B */
static bool frac_greater_than(int num_a, int denom_a, int num_b, int denom_b)
{
  //printf("Comparing %d/%d > %d/%d\n", num_a, denom_a, num_b, denom_b);
  int common_denom = lcm(denom_a, denom_b);

  int a_multiplier = common_denom / denom_a;
  int b_multiplier = common_denom / denom_b;

  int new_num_a = a_multiplier * num_a;
  int new_num_b = b_multiplier * num_b;

  return new_num_a > new_num_b;
}

/* Return the greatest common divisor of A and B */
static int gcd(int a, int b)
{  
  /* 0 is divisible by everything */
  if (a == 0 || b == 0)
    return 0;

  /* Base case */
  if (a == b)
    return a;

  /* a is larger */
  if (a > b) {
    return gcd(a - b, b);
  } else {
    /* b is larger */
    return gcd(a, b - a);
  }
}

/* Return the least common multiple of A and B */
static int lcm(int a, int b)
{
  return (a * b) / gcd(a, b);
}
