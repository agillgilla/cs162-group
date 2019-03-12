Final Report for Project 1: Threads
===================================
## Changes

### Part 1:
At first, we planned on pushing threads to the back of our sleeping thread list within the `timer_sleep()` function and then iterate through the sleeping thread list within the `timer_interrupt()` function. We realized that since `timer_interrupt()` runs more frequently than `timer_sleep()`, we instead opted to inserting the threads into the list in sorted order. While both searching for the thread and inserting in the thread in the right order were linear time complexity, the first was linear time upon insertion while the latter was linear time upon accesses. Because accesses happen much more often, reducing accesses to constant time greatly improved our performance. 

We also initially planned to use locks within `timer_sleep()`. This had issues since it may cause the thread that handles `timer_interrupt`s to be blocked, our final implementation disables interrupts instead of using locks to maintain synchronization.

### Part 2:
Our original expectation for `lock_release()` was that only the single highest priority donation was relevant, such that any thread that releases a lock would have its `effective_priority` reset to its `base_priority`. However, this failed to consider cases where there could be multiple locks waiting  for each other. Our final implementation includes recursive priority donations, so a donating thread `A` that is donating to a thread `B` will recurse and donate to a thread `C` that `B` is waiting for, and continues down the chain.  We also have a different method for "de-donating" when releasing locks whereby a thread `X` that is releasing a lock `L_i` will iterate through the list of all of the locks it is holding, `locks_held`, and set its `effective_priority` to that of the highest priority thread `max_priority_thread` waiting on any given lock held by `X`.

We also initially planned to use locks throughout part 2 but instead chose to simply disable interrupts in order to maintain synchronization.  This is because for one thing, disabling interrupts is easier than using locks, and also because all of the functions called for scheduling need to happen quickly and not fall asleep.

### Part 3:
Initially, our plan was to move all of the threads that match the maximum priority in the ready list to another list, `max_priority_threads`, and keep and index to iterate through `max_priority_threads`. However, this is a complex (and somewhat inefficient) solution for a simple task. Our final implementation inherently executes round robin by only removing the first maximum priority thread from the `ready_list` and pushing it back to the end once it yields or there is an interrupt.

Also, we decided to add additional variables ```thread_mlfqs``` and ```mlfqs_priority``` to distinguish mlfqs threads from non-mlfqs threads and to keep track of ```mlfqs_priority``` in decreasing order in the list. If a thread is mlfqs, ```priority_comparator``` uses ```mlfqs_priority``` to order the priority in decreasing order, maintaining the linear time access like the non-mlfqs thread. 

Again, we disabled interrupts to enforce synchronization instead of using a lock for similar reasons as in part 2.

## Reflection

In terms of the distribution of work for the design document, every member in our team met up and completed it together, and every member contributed to the drafting and discussion that went into our design document. 

Once we began actually coding for the project, Arjun took the most initiative and finished parts 1 and 2 relatively early on without the help of the other members. Part 3 was mostly a collaborative process in which the members met up and completed the majority of the task, while Arjun tied up the loose ends and helped us identify a bug that we were stuck on for the MLFQS blocking. 

Our group can improve on creating a timeline for tasks and distributing the tasks more evenly. Because we are all busy people with varying schedules, it was hard to allocate significant amounts of time where each member was all available to work on the project, and due to the lack of initiative in organizing when to meet, we each ended up focusing on different parts of the project instead of working collaboratively on every single part of the project. Next time, we hope to allocate tasks and have a more comprehensive view/management of each team member's schedules so we can plan the workload more accordingly and avoid one of the team members doing the majority of the work. 
