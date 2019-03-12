Final Report for Project 1: Threads
===================================
## Changes

### Part 1:
At first, we planned on pushing threads to the back of our sleeping thread list within the ```timer_sleep``` function and then iterate through the sleeping thread list within the ```timer_interrupt``` function. We realized that since ```timer_interrupt``` runs more frequently than ```timer_sleep```, we instead opted to inserting the threads into the list in sorted order. While both searching for the thread and inserting in the thread in the right order were linear time complexity, the first was linear time upon insertion while the latter was linear time upon accesses. Because accesses happen much more often, reducing accesses to constant time greatly improved our performance. 

We also initially planned to use locks within ```timer_sleep```. This had issues since it may cause the thread that handles ```timer_interrupts``` to be blocked, our final implementation disables interrupts instead of using locks to maintain synchronization.

### Part 2:

### Part 3:
Initially, our plan was to remove all of the threads that match the maximum priority in our waiting list. However, this led to unforseen stalling. Our final implementation utilizes round robin to break ties in order to solve this problem, and only removes the first item of our queue and adds it back to the other end of our queue. 

## Reflection

In terms of the distribution of work for the design document, every member in our team met up and completed it together, and every member contributed to the drafting and discussion that went into our design document. 

Once we began actually coding for the project, Arjun took the most initiative and finished parts 1 and 2 relatively early on without the help of the other members. Part 3 was mostly a collaborative process in which the members met up and completed the majority of the task, while Arjun tied up the loose ends and helped us identify a bug that we were stuck on for the MLFQS blocking. 

Our group can improve on creating a timeline for tasks and distributing the tasks more evenly. Because we are all busy people with varying schedules, it was hard to allocate significant amounts of time where each member was all available to work on the project, and due to the lack of initiative in organizing when to meet, we each ended up focusing on different parts of the project instead of working collaboratively on every single part of the project. Next time, we hope to allocate tasks and have a more comprehensive view/management of each team member's schedules so we can plan the workload more accordingly and avoid one of the team members doing the majority of the work. 
