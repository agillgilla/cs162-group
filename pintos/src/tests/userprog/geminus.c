/* Tries to create a file, open the file descriptor for it, and then 
  remove (delete) the file twice. It can either fail silently or 
  result in exit with error code -1. */

#include <limits.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  /* Create the file */
  CHECK (create ("geminus.txt", 0), "geminus.txt");
  /* Open the file descriptor */
  int handle = open ("geminus.txt");
  if (handle < 2)
    fail ("open() returned %d", handle);

  /* Remove the file, should be successful */
  bool success = remove ("geminus.txt");

  if (!success) {
    fail ("first remove() failed");
  }

  /* Second remove should fail */
  bool success2 = remove ("geminus.txt");

  if (success2) {
    fail ("second remove() succeeded");
  }
}