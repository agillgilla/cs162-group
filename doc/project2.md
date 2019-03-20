Design Document for Project 2: User Programs
============================================

## Group Members

* Arjun Gill <arjun.gill@berkeley.edu>
* Nicholas Ruhman <nwruhman@berkeley.edu>
* Emma Han <emma_han@berkeley.edu>
* Alexander Mao <mao@berkeley.edu>

---

## Task 1: Argument Passing

### Data structures and functions

We will manipulate `start_process(...)` and `process_execute(...)` in order to pass arguments for system calls.  No data structures will have to be changed.

### Algorithms

The main process of parsing the space delimited arguments can be done with `strtok(...)`.  We will use `strtok(...)` to tokenize the string (delimited by a space) and push the arguments given by the user onto the stack for the user process we are creating.  The algorithm for setting up the user process will follow simply from sections 3.1.7-3.1.9 in the project spec.  This will include pushing the user arguments to the stack,  pushing a null pointer sentinel onto the stack (word-aligning beforehand),  then pushing `*argv` and `argc` onto the stack, and finally a “fake” return address.

Parsing the file_name with `strtok(...)` will be done in both `process_execute(...)` and `start_process(...)` and pushing the arguments, null sentinel, `*argv`, `argc`, and the fake return address will be done after `load(...)` in `start_process(...)`.  Adding all these variables to the stack will also entail decrementing the stack pointer many times.

Lastly, there will have to be checking done to ensure that we return a page fault if the user arguments memory location is above `PHYS_BASE` or if the number of user arguments is over our defined maximum.

### Synchronization

There are three options for synchronization.  First, using locks doesn’t seem necessary here and would probably cause problems.  Second, disabling interrupts isn’t viable here because a system call happens in an interrupt handler.  So the last option is to make sure that our function calls are thread-safe.  Our only function call is to `strtok(...)` so we can use the `strtok_r(...)` function which is known to be thread-safe and is the POSIX version of `strtok_s(...)`. 

### Rationale

Our implementation of argument passing will be fairly simple (if we follow the algorithms outlined above.)  The only other conceivable way of parsing the arguments would be to manually loop through the char array and split at spaces, which would be tedious, and would probably entail dynamic memory allocation, which is complicated in the kernel code.  Other than that, the 80x86 calling convention steps just have to be converted to C code and the proper checks on user input have to be conducted.

---

## Task 2: Process Control Syscalls

### Data structures and functions
In process.h
```C
struct wait_status {
	int ref_count;
	tid_t child_tid;
	pid_t pid; 
	int exit_code;
	struct semaphore sema;
	struct lock lock;
	struct list_elem elem;
	}
```
In thread.h
```C
	struct thread {
	...
	struct list children;
	struct wait_status *wait_st;
	}

```
### Algorithms
First we want to verify the virtual memory pointer that is being accessed. This must be done before the system call number is accessed. 

Then, every argument being used for the syscall must be verified as well before we dereference anything.

For verification, we first ensure that none of the pointers are NULL pointers. Then, we use the function ```is_user_addr()``` in threads/vaddr.h in order to verify that the pointer points to a valid place in the user virtual memory. Lastly, we verify if the mapping exists by calling ```lookup_page()``` in pagedir.c with the CREATE flag set to false, in order to make sure that the pointer passed in is properly mapped from virtual to physical memory.

If there are any invalid pointers, we must handle and free all the resources that the current process is using. To do this, we iterate through the locks and free all that are held by the process, as well as free all the pages allocated to that process.

Following are the algorithms we will use for __halt__, __exit__, __exec__, __wait__, and __practice__.

__halt:__ We simply call shutdown_power_off() in devices/shutdown.c

__exit:__ Most of the implementation has been provided, but we want to set ```exit_code``` in the ```wait_st``` member, and then up sema for ```wait_st```. Then, we terminate the thread. ```ref_count``` is then decremented, and if it equals 0, we deallocate ```wait_status```.

__exec:__ In init_thread(), add wait_status struct and list of children, where wait_status->sema value starts at 0. In exec syscall, call process_execute(file) and save the tid_t return value.

__wait:__ If a child ```wait_status->pid``` matches the returned ```tid```, we down sema for the child ```wait_st```. We return ```exit_code``` in ```wait_st``` for the child that is being waited on, and increment ```ref_count``` by 1. 

__practice:__ Increment ```args[1]``` and store it in the ```f->eax``` register, where f is the argument to ```syscall_handler()```.
### Synchronization
We will use the ```lock``` struct inside of ```wait_status``` whenever doing operations on the ```ref_count``` to make sure there are no race conditions.  We don’t have to to lock any other members of ```wait_status``` since they either won’t be modified or they will only be modified by one thread (themself, like ```exit_code```.)
### Rationale

Our implementation is relatively straightforward in that it simply handles multiple cases in the syscal handler after handling the cases in which a syscall cannot be completed due to invalid memory access. 

We used the first verification method outlined in the spec (slower, but simpler to understand) but may switch the the second later on in the project if we have time or want to improve the performance and can do so in a timely manner without bugs.

In terms of the specific syscalls, halt and practice are extremely straightforward. The most important part was making sure that there exists no race conditions in the other syscalls, which we handle adequately using a lock in ```wait_st```.

---

## Task 3: File Operation Syscalls

### Data structures and functions

```C
struct thread {
	…
	struct file *my_file; /* Pointer to binary file running on thread */
	...
}

struct file_entry {
	int fd; /* File descriptor number for file entry */
	struct file *file; /* File struct pointer for file entry */
	struct list_elem elem; /* List element for adding to a list */
}
```

#### In `thread.c`:
Add a struct lock `file_table_lock` so there are no concurrency issues when accessing filesystem.

Add a list of `file_entry`’s called `file_table` in `thread.c` that will be a global file descriptor table to keep track of user processes’ currently open file descriptors.

#### In `process.c`:
Change `load(...)`  to set the `my_file` of a thread’s execution context to the appropriate `file` struct.

#### In `syscall.c`:
Acquire `file_table_lock` at the top of `syscall_handler(...)` to prevent race conditions.

Add more if statements for every file I/O syscall and call the appropriate function in `filesys.c` or `file.c` for the given case.

### Algorithms

The main algorithm for this part is a long series of if/else if statements added to `syscall_handler(...)` in `syscall.c` so that the file I/O system calls call the appropriate functions (already implemented for us) in `filesys.c` and `file.c` and outlined in section 3.2.3 of the spec.  

We will also have to maintain a file struct pointer to the current file being executed by the thread to make sure we don’t overwrite the executable by any of the file syscalls in `syscall_handler(...)` and the member will be initialized in `thread.c` and set in `process.c` (in the `load(...)` function.)

Lastly, we will add a lock on the file table that we are implementing for file I/O calls in `thread.c`, and this lock will be acquired at the top of `syscall_handler(...)` whenever we are doing a file operation.

### Synchronization

The main worry for synchronization in this task is concurrency of file I/O.  This is solved with the global `file_table_lock` that will be initialized in `thread.c` and will be acquired whenever a file I/O system call is performed.

### Rationale

The implementation outlined above is the most efficient and straightforward way of approaching the design problem that we have come up with.  There will have to be some sort of list for the file table and keeping track of file descriptors, so we created the global `file_table` list in thread.c composed of `file_entry`’s that associate file descriptors to files.  There will also be a simple implementation of system file I/O calls by just calling the black box functions provided to us in `filesys.c` and `file.c`.  Lastly, we solve concurrency with a global lock on the file table, which was suggested in the project spec.

--

## Additional Questions

1. `sc-bad-sp` uses an invalid stack pointer on line 18 when making a syscall.  The test case uses movl to set the stack pointer to **64 MB** below the code segment, which is definitely not a valid stack pointer.

2. `sc-bad-arg` passes a valid stack pointer to the system call, but the stack pointer is too close to the top page of user memory, so the stack pointer overflows into the kernel memory.

3. None of the tests test `filesize(...)`, `seek(...)`, or `tell(...)` (the file I/O syscalls to be implemented in task 3.)  Tests need to be added to make sure that `filesize(...)` returns the proper size of a file and it handles edge cases properly, and that `seek(...)` and `tell(...)` both work as outlined in the spec.
