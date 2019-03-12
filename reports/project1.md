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

Also, we decided to add an additional member `mlfqs_priority` to allow for computations with `fixed_point` arithmetic.  We essentially copy the formulas given for computing the `load_avg`, `recent_cpu`, and `priority` values (and convert them into `fixed_point` function calls) into the appropriate locations of the scheduler. If `thread_mlfqs` is on, our `priority_comparator` uses the thread's `mlfqs_priority` to find the maximum priority thread, maintaining the same linear time schedluing like we had in part 2.  We had to wrap `if` statements checking if `thread_mlfqs` is on in certain places to prevent priority donations and "de-donations" and have different functionality for MLFQS rather than the scheduler in part 2.

Again, we disabled interrupts to enforce synchronization instead of using a lock for similar reasons as in part 2.

## Reflection

In terms of the distribution of work for the design document, every member in our team met up and completed it together, and every member contributed to the drafting and discussion that went into our design document. 

Once we began actually coding for the project, Arjun took the most initiative and finished parts 1 and 2 relatively early on without the help of the other members. Part 3 was mostly a collaborative process in which the members met up and completed the majority of the task, while Arjun tied up the loose ends and helped us identify a bug that we were stuck on for the MLFQS blocking. 

Our group can improve on creating a timeline for tasks and distributing the tasks more evenly. Because we are all busy people with varying schedules, it was hard to allocate significant amounts of time where each member was all available to work on the project, and due to the lack of initiative in organizing when to meet, we each ended up focusing on different parts of the project instead of working collaboratively on every single part of the project. Next time, we hope to allocate tasks and have a more comprehensive view/management of each team member's schedules so we can plan the workload more accordingly and avoid one of the team members doing the majority of the work. 


## Code Quality

* Our code **doesn't** exhibit any major memory safety problems.  It doesn't use any C strings so there is no problem there, it doesn't have any memory leaks since we never directly do any memory allocation (it is just done in `list_init()` and `thread_init()`, which properly deallocate when the program finishes execution or when a thread is destroyed.)  Also, there are no race conditions since we disable interrupts whenever accessing a concurrency-dependent data structure.

* We mimic the code style that Pintos uses, including snake case for variables and functions and 2 spaces for indentation.  The only deviation from Pintos's style is not putting curly braces on the next line and not putting spaces between the function name and parentheses, which looked somewhat strange.

* Our code is fairly simple and easy to understand.  It follows a similar pattern when doing tasks that are similar (such as finding the max of a list, etc.)  It implements the algorithms we set out to use in a clean and straightforward manner.

* Our entire code that we added is littered with comments (almost on every other line in some functions) and we leave out comments only in self-explanatory functions/lines.

* We did not leave commented-out code in our final submission.

* We tended to create reusable functions **when applicable**, which was typically only for comparator functions for finding the max of a list, and rarely, if ever, copied exact code from one function to another.

* We found it easier to implement iterating through linked lists ourselves in some situations since it would often be messier to try to use some of the methods given in `list.c`, however we did use `list_max` with a comparator for many functions.

* Very rarely do we have lines that span more than 100 characters.  In lines with many function calls (like for long `fixed_point` arithmetic) we use the appropriate indentation and new lines to prevent the line from being too long and to improve readability.

* We never commited binary files when making commits, those were either ignored by the `.gitignore` or not explicitly added with `git add`.
