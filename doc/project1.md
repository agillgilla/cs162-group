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

Any time we access the `sleep_list`, we need to have a `sleep_lock` around it so that it can't be corrupted by multiple waking threads.  When we try to acquire the `sleep_lock`, we `lock_acquire(...)` which will **always** have to disable interrupts because our method could fail if it is interrupted.

### Rationale

The implementation we described is easy to execute, requires minimal added global variables in thread.c, and doesn’t cause any foreseeable interactions with the existing logic for handling threads.  The time complexity of waking threads from sleep should be no more than linear, and putting threads to sleep will be at constant time operation.

---

## Task 2: Priority Scheduler

### Data structures and functions

```C
struct thread {
    int base_priority;
    int effective_priority;
    struct lock priority_lock;
    ...
}
```

### Algorithms

#### Choosing the next thread to run
Currently, `next_thread_to_run(...)` picks the thread at the end of the `ready_list` (which is arbitrary.)  Instead, it should pick the thread in the `ready_list` with the highest `effective_priority`.

#### Acquiring a Lock
For priority donation when acquiring locks, `lock_acquire(...)` will be called, then if the thread that is calling lock_acquire(...) has a higher `effective_priority` than the current lock holder, then we donate to the current lock holder by setting its `effective_priority` to the `effective_priority` of the (higher priority) thread that is requesting the lock.

#### Releasing a Lock
Once any thread releases the lock, it drops its `effective_priority` back to its `base_priority`.

#### Computing the effective priority
`max(effective_a, effective_b)`

#### Priority Scheduling for semaphores and locks
Currently, `sema_up(...)` wakes up a random thread from the waiters list.  Instead we will change it so that it wakes up the thread with the highest priority in the waiters list.  This will use the `list_max(...)` function in `list.c`.

#### Priority Scheduling for condition variables
`cond_signal(...)` wakes up the thread at the end of the condition variable waiters list.  We need to change this so that it wakes up the thread with the highest priority in the waiters list.  This will use the `list_max(...)` function in `list.c`.

#### Changing a thread's priority
First, we `lock_acquire(thread->priority_lock)` so that the priority can’t be changed anywhere else.  Then we change the thread’s `effective_priority` to the argument passed in, then we `lock_release(thread->priority_lock)` before we return from `thread_set_priority()`.  This keeps the priority of the thread from getting corrupted by multiple sources calling `thread_set_priority()`.

### Synchronization
Race conditions shouldn’t occur during priority scheduling (assuming they don’t already exist) since we will just be adding code to existing scheduling algorithms.

Also, we use a lock (`priority_lock`) to prevent the priority of a thread from being corrupted by simultaneous calls to `thread_set_priority()`.

### Rationale
Our implementation of selecting a new thread will be simple, as we can just run the next thread with highest priority.  Priority lock scheduling is also a simple implementation, we will just check the `effective_priority` of the thread trying to acquire the lock and bump the owner’s priority up to theirs, and reset on the the lock release.  Priority scheduling for semaphores and condition variables are both very similar, just wake the thread with highest priority on the waiters queue, and changing threads priority is straightforward, we only have to think about synchronization which was already explained earlier.

---

## Task 3: Multi-level Feedback Queue Scheduler (MLFQS)

### Data structures and functions
```C
struct thread {
    int niceness;
    int recent_cpu;
    ...
}
```

#### Global variables in `threads.c`:
```C
struct fixed_point load_avg;
struct lock ready_lock;
struct list max_priority_list;
unsigned tied_index;
struct lock max_priority_lock;
```

### Algorithms

#### Updating `recent_cpu`, thread priority, and `load_avg`
We will place the logic for updating `recent_cpu` and thread priority in `thread_tick(...)` because this happens on a tick/time determinant trigger.  As stated in the spec, the algorithm will increment the running thread’s `recent_cpu` value once for every `timer_tick(...)`.  Then, if the `timer_ticks` value is divisible by 4, we recompute every thread’s priority according to the formula, and limit its min/max value to `PRI_MIN` or `PRI_MAX`, respectively.  Then we get any threads in the `ready_list` with max priority ties and add them to `max_priority_list` (after emptying it out.)  Also, if `timer_ticks` is divisible by `TIMER_FREQ`, then we recompute `load_avg` and `recent_cpu` according to their respective formulas.

#### Scheduling the next thread
If there are no threads in the `ready_list`, then we run the idle thread as usual.  Otherwise, we increment `tied_index` and mod it with the length of `max_priority_list` and select the thread at `tied_index` in `max_priority_list` to run.

### Synchronization

We will have a lock around any critical operations that use the `ready_list` so that the data structure doesn’t get corrupted.  We will also have a lock around any operations with the `max_priority_list` to prevent that from getting corrupted.  If all else fails, we can simply disable interrupts on any functions that could possibly cause race conditions with simultaneous calls to them.

### Rationale

This implementation described above seems like the most straightforward and simple way of creating a MLFQS as described in the spec.  We take adequate but limited measures to prevent corruption of key data structures, and we should never miss computations since most of the recompute actions are done in `thread_tick(...)` which is called on every thread tick.

---

## Additional Questions:

1. 
#### Test Case:
Consider a scenario with 4 threads, `thread_1`, `thread_2`, `thread_3`, and `thread_4`, competing for 2 locks, `lock_1` and `lock_2`.  Since `sema_up(...)` is used in the lock release, this bug propagates to locks.  Here are the starting priorities for each thread:
    
    thread_1: 1
    thread_2: 3
    thread_3: 2
    thread_4: 4
    
At time 1, `thread_1` acquires `lock_1`.  Then, at time 2, `thread_3` acquires `lock_2`.  At the next timestep, `thread_4` tries to acquire `lock_2`, and thus bumps `thread_3`’s effective priority to 4.  Then `thread_3` tries to acquire `lock_1`, and propagates the effective priority it got from `thread_4` to `thread_1`, bumping its priority to 4.  Next, `thread_2` tries to acquire `lock_1`, and blocks on it since `thread_1` is still holding it.  Then, `thread_1` releases `lock_1` and the `sema_up(...)` chooses `thread_2` to hold `lock_1`, even though `thread_3` is waiting for `lock_1` and has higher effective priority (but it’s base priority is lower.)

#### Expected Output:
You would expect `thread_3` to acquire `lock_1` before `thread_2` (according to semaphore scheduling priority.)

#### Actual Output:
`thread_2` acquires `lock_1` before `thread_3`.

2. 
timer ticks | R(A) | R(B) | R(C) | P(A) | P(B) | P(C) | thread to run
------------|------|------|------|------|------|------|--------------
 0          |  0   |  0   |  0   |  0   |  0   |  0   |     A
 4          |  4   |  0   |  0   |  63  |  62  |  60  |     A
 8          |  8   |  0   |  0   |  62  |  62  |  60  |     A
12          |  12  |  0   |  0   |  61  |  62  |  60  |     B
16          |  12  |  4   |  0   |  61  |  61  |  60  |     A
20          |  16  |  4   |  0   |  60  |  61  |  60  |     B
24          |  16  |  8   |  0   |  60  |  60  |  60  |     A
28          |  20  |  8   |  0   |  59  |  60  |  60  |     B
32          |  20  |  12  |  0   |  59  |  59  |  60  |     C
36          |  20  |  12  |  4   |  59  |  59  |  59  |     A

3. The first ambiguity was that we didn't know the `TIMER_FREQ`, so we just assumed it was a value over 36 (which includes the entire table, so that implies that we will never have to update `load_avg` (which we initialize to 0) and `recent_cpu` except for incrementing on every timer tick.  There was also ambiguity about what thread we select when we have multiple priorities that are the same value.  We just decided to pick the alphabetically first thread (A, then B, then C) in this scenario.  We also assumed that the `PRI_MAX` was 64 as is usually is, because that wasn't specified either.

