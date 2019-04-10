/* Tries to create a file, open the file descriptor for it, and then 
  seek and tell on the file. Both should be successful.  Then seek is
  called with a bad file descriptor.  This should cause the process to
  exit with error code -1. */

#include <limits.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  /* Create the file */
  CHECK (create ("seek.txt", 0), "seek.txt");

  /* Open the file descriptor */
  int handle = open ("seek.txt");
  if (handle < 2)
    fail ("open() returned %d", handle);

  /* Seek to the 5th byte in the file (no issue should arise) */
  seek (handle, 5);

  /* Do a seek to the 0th byte in a bad file descriptor (process should exit) */
  seek (42, 0);
}