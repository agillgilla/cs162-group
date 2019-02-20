Design Document for Project 1: Threads
======================================

## Group Members

* Alexander Mao <mao@berkeley.edu>
* Arjun Gill <arjun.gill@berkeley.edu>
* Emma Han <emma_han@berkeley.edu>
* Nicholas Ruhman <nwruhman@berkeley.edu>

---

## Task 1: Efficient Alarm Clock

### Data structures and functions

#### In `threads.h`:
```C
struct thread {
    ...
    unsigned wakeup_time;
}
```
#### In `threads.c`:
```C
static struct list sleep_list;
static struct lock sleep_lock;
```

### Algorithms

#### Arranging a wakeup time
First, in `timer_sleep(...)` we block the thread that is passed in the argument.  Then we can set the  `wakeup_time` of the blocked thread to the current timer ticks plut the timer ticks the thread is set to sleep for.  Then we add the thread to the  `sleep_list`.

#### Waking up a thread
In `thread_tick(...)` we can check the min elements in the sleep list to see if it has a `wakeup_time` that is past the current timer ticks.  If any element does, we remove it from the sleep list and call `thread_unblock(...)` on it.  We stop removing elements from the sleep list once the min is no longer greater than the current number of timer ticks.

### Synchronization

Any time we access the `sleep_list`, we need to have a `sleep_lock` around it so that it can't be corrupted by multiple waking threads.  When we try to acquire the `sleep_lock`, we `lock_acquire(...)` will **always** have to disable interrupts because our method could fail if it is interrupted.

### Rationale

The implementation we described is easy to execute, requires minimal added global variables in thread.c, and doesn’t cause any foreseeable interactions with the existing logic for handling threads.  The time complexity of waking threads from sleep should be no more than linear, and putting threads to sleep will be at constant time operation.

---

## Task 2: Priority Scheduler

### Data structures and functions

*Write down any variables, typedefs, or enumerations added or modified with a concise explanation, no pseudocode necessary.*

### Algorithms

*High level description of the algorithms and convince that they satisfy requirements and common edge cases. 1 paragraph for a simple task, 2 for a complex task.*

### Synchronization

*Describe rationality for preventing race conditions and convincing argument that it will work in all cases.*

### Rationale

*Discuss alternatives, pro’s/con’s, as well as whether it is easy, amount of coding, space time complexity, difficulty of extending design for additional features.*

---

## Task 3: Multi-level Feedback Queue Scheduler (MLFQS)

### Data structures and functions

*Write down any variables, typedefs, or enumerations added or modified with a concise explanation, no pseudocode necessary.*

### Algorithms

*High level description of the algorithms and convince that they satisfy requirements and common edge cases. 1 paragraph for a simple task, 2 for a complex task.*

### Synchronization

*Describe rationality for preventing race conditions and convincing argument that it will work in all cases.*

### Rationale

*Discuss alternatives, pro’s/con’s, as well as whether it is easy, amount of coding, space time complexity, difficulty of extending design for additional features.*

---

## Additional Questions:

1. *Prove the existence of priority donation bug in ***_sema_up()_*** function*

2. *Fill out table at **[https://gist.github.com/rogerhub/82395ea1f3ed64db677*9](https://gist.github.com/rogerhub/82395ea1f3ed64db6779)* *

3. *Which ambiguities were in #2 and what rules were used to resolve them?*

