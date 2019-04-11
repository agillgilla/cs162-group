Final Report for Project 2: User Programs
=========================================

## Changes

### Task 1 (Argument Passing):

Our code didn't really exhibit any changes for Task 1.  We stuck to using `strtok_r(...)`.  For tokenizing the space-delimited input string.  We didn't end up having to check if the user arguments were valid (below `PHYS_BASE`) because that check is administered in `validate_segment(...)` which is called in `load(...)`.

One algorithmic design choice that we didn't anticipate doing was iterating through the input string with `strtok_r(...)` twice to first count the number of arguments, then actually declare an array for the addresses of the arguments, and finally fill it in another loop.

### Task 2 (Process Operation Syscalls):

For Task 2, we did have to make some changes from our original design document.  First, we didn't think of validating the address of the stack pointer (only the arguments passed into the syscalls) so we had to check that, as well as the addresses extending all the way to the end of strings for syscalls where strings are input arguments.

Most of the implementation for the syscalls didn't change much at all, but (as mentioned by Jason in the design review) we had to create a new data structure and algorithm for when processes are loaded so the parent that spawns the process knows if the process was loaded properly.  This entailed creating the `exec_info` struct that was used to allow the parent thread to block until the child was finished loading (or encountered an error while loading.) This was crucial for implementing the required behavior for `process_execute(...)`.

### Task 3 (File Operation Syscalls):

Since implementing file operation syscalls themselves is fairly straightforward and mostly just requires calling functions in `filesys.c` or `file.c`, there were no changes in our code from the design doc.  Also, we correctly decided to use a global lock on the filesystem and a per-process file table, which is exactly what we ended up implementing in our code.

The only deviation from our design document was correctly preventing file writes to a running executable.  We followed the advice Jason gave us at the design review and called `file_deny_write` on a new thread's executable and re-allowing writes to it when it is closed.

## Reflection


## Student Testing Report

### Test 1: Double Remove (Geminus):

**Description:** This test is meant to make sure that the kernel behaves appropriately when a syscall to remove a file is called twice. 

**Overview:** The test works by first creating a text file, opening a file handle to the file, removing it, and then removing it again.  The expectation is that the output shows the code running fine until the second remove, where the return value is `false` indicating that the syscall failed.

**Output:**:
```C
Copying tests/userprog/geminus to scratch partition...
qemu -hda /tmp/0uiF41ArSh.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading..........
Kernel command line: -q -f extract run geminus
Pintos booting with 4,088 kB RAM...
382 pages available in kernel pool.
382 pages available in user pool.
Calibrating timer...  419,020,800 loops/s.
hda: 5,040 sectors (2 MB), model "QM00001", serial "QEMU HARDDISK"
hda1: 167 sectors (83 kB), Pintos OS kernel (20)
hda2: 4,096 sectors (2 MB), Pintos file system (21)
hda3: 102 sectors (51 kB), Pintos scratch (22)
filesys: using hda2
scratch: using hda3
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'geminus' into the file system...
Erasing ustar archive...
Executing 'geminus':
(geminus) begin
(geminus) geminus.txt
(geminus) end
geminus: exit(0)
Execution of 'geminus' complete.
Timer: 63 ticks
Thread: 5 idle ticks, 58 kernel ticks, 0 user ticks
hda2 (filesys): 108 reads, 213 writes
hda3 (scratch): 101 reads, 2 writes
Console: 894 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
```

**Result:**
```C
PASS
```

**Kernel Bugs:** Two bugs that would have affected the output of this test case would be 1) if the kernel simply didn't look up the file descriptor of the file to remove properly, the file wouldn't actually be removed, and 2) if the kernel didn't forward the return value of the filesystem call back to the `eax` register, then the test would fail because the return value wouldn't be `true` then `false`.

---

### Test 1: Double Remove (Geminus):

**Description:** This test is meant to make sure that the kernel works for calling `seek(...)`, but fails on an invalid file descriptor. 

**Overview:** The test works by first creating a text file, opening a file handle to the file, and then seeking to a valid index in the file. The test then calls `seek(...)` on an invalid file descriptor, which should cause the program to exit with an error code of -1.  We mostly created this test because none of the tests given tested `seek(...)` (and specifically not with a bad file descriptor.)

**Output:**:
```C
Copying tests/userprog/seek-bad-fd to scratch partition...
qemu -hda /tmp/Tm1E7Kz_pG.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading..........
Kernel command line: -q -f extract run seek-bad-fd
Pintos booting with 4,088 kB RAM...
382 pages available in kernel pool.
382 pages available in user pool.
Calibrating timer...  391,987,200 loops/s.
hda: 5,040 sectors (2 MB), model "QM00001", serial "QEMU HARDDISK"
hda1: 167 sectors (83 kB), Pintos OS kernel (20)
hda2: 4,096 sectors (2 MB), Pintos file system (21)
hda3: 102 sectors (51 kB), Pintos scratch (22)
filesys: using hda2
scratch: using hda3
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'seek-bad-fd' into the file system...
Erasing ustar archive...
Executing 'seek-bad-fd':
(seek-bad-fd) begin
(seek-bad-fd) seek.txt
seek-bad-fd: exit(-1)
Execution of 'seek-bad-fd' complete.
Timer: 65 ticks
Thread: 5 idle ticks, 61 kernel ticks, 0 user ticks
hda2 (filesys): 87 reads, 212 writes
hda3 (scratch): 101 reads, 2 writes
Console: 905 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
```

**Result:**
```C
PASS
```

**Kernel Bugs:** Two bugs that would have affected the output of this test case would be 1) if the kernel didn't register valid calls to `seek(...)` properly by calling the appropriate function in `file.c`, then the first call may not behave properly, and 2) if the kernel didn't check the file descriptor before trying to call `seek(...)` it would not exit from the process with the appropriate exit code.

---

### Review:

Writing tests in pintos is pretty easy, and doesn't require that much work.  Writing the actual source (`.c`) file is very easy and can't really be improved.  However, while writing the `.ck` file it can be weird to understand the syntax and know what things should be printed by the OS.  Changing the code in the `Make.tests` file was easy, but could be easy to make a mistake if you forget to add an entry to one of the sections.  I learned how the `make check` works by looking at the makefiles when writing tests.

## Code Quality

* Our code doesn't exhibit any major memory safety problems (that we know of.)  Any strings `malloc`'d are freed when the process exits or the function completes.  We rarely use `malloc` otherwise.  There is no poor error handling in our code, any time an error occurs the process sets its exit code or the function exits gracefully.  There is no potential for race conditions in our code, either, because we use locks when necessary as detailed in the design document.

* We maintained the same code style as in Pintos.  Same two-space indentation and spacing around kernel function calls, as well as keeping `snake_case` for variable and function names.

* The code algorithms are as simple as we think possible and fairly easy to understand if someone is looking over it who hasn't seen it before.

* We put a multitude of comments in our code, especially around complex sections in task 1, and the algorithmic portions of task 2.  Most of the code in `syscall.c` is self explanatory, so we don't have as many comments there, but it seems fine to me.

* We didn't leave commented-out code in the final submission.

* We created functions when possible rather than copying and pasting.  This includes file descriptor functions in `syscall.c` and some functions in `process.c`.

* We used the provided list implementation any time we needed a list (which was only for waiting on children and the file table.)

* We don't have any source code that is excessively long, we put new lines and indent accordingly whenever there is a very long line.

* We never commited binary files to Git.
