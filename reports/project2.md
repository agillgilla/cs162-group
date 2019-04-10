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


## Code Quality


