Final Report for Project 3: File System
=======================================

## Changes

### Task 1 (Buffer Cache):



### Task 2 (Extensible Files):



### Task 3 (Subdirectories):



## Reflection

### Task 1


### Task 2


### Task 3


### Tests


## Student Testing Report

### Test 1: <enter test name here>:

**Description:** 

**Overview:** 

**Output:**: 

**Result:** 

**Kernel Bugs:** 

---

### Test 2: <enter test name here>:

**Description:** 

**Overview:** 

**Output:** 

**Result:** 

**Kernel Bugs:** 

---

### Review:



## Code Quality

* Our code doesn't exhibit any major memory safety problems (that we know of.)  Any strings `malloc`'d are freed when the process exits or the function completes.  We rarely use `malloc` otherwise.  There is no poor error handling in our code, any time an error occurs the process sets its exit code or the function exits gracefully.  There is no potential for race conditions in our code, either, because we use locks when necessary as detailed in the design document.

* We maintained the same code style as in Pintos.  Same two-space indentation and spacing around kernel function calls, as well as keeping `snake_case` for variable and function names.

* The code algorithms are as simple as we think possible and fairly easy to understand if someone is looking over it who hasn't seen it before.

* We put a multitude of comments in our code, especially around complex sections in task 2, and the special/difficult cases for task 3.  Most of the code in `cache.c` is self explanatory, so we don't have as many comments there,  because things are handled with the typical way for caches (clock algorithm for eviction, setting appropriate flags on reads/writes, flushing, etc.)

* We didn't leave commented-out code in the final submission.

* We created functions when possible rather than copying and pasting.  This includes helper functions in `cache.c` for cache searches and evictions/flushes and `filesys.c` for getting parts of file paths and checking various properties of `dir`s`.  We also created helper functions in `inode.c` for extending inodes and gettsers/setters of struct members for opaque types

* We didn't have to create any of our own lists for this project.

* We don't have any source code that is excessively long, we put new lines and indent accordingly whenever there is a very long line.

* We never commited binary files to Git.

