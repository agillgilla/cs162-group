/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value)
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0)
    {
      list_push_back (&sema->waiters, &thread_current ()->elem);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema)
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0)
    {
      sema->value--;
      success = true;
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT (sema != NULL);

  old_level = intr_disable ();

  /* Variable to store the priority of the max priority waiter */
  int max_waiter_priority = -1;
  /* Get the thread calling sema_down(...) */
  struct thread *downer = thread_current();

  struct list_elem *waiter = list_begin(&sema->waiters);

  if (!list_empty(&sema->waiters)) {
    //printf("%s%zd\n", "Num waiters: ", list_size(&sema->waiters));
    /* Get the max priority waiter on the semaphore */
    struct list_elem *max_priority_elem = list_max(&sema->waiters, priority_comparator, NULL);
    /* Remove the max priority waiter from the waiters list */
    list_remove(max_priority_elem);
    /* Get the thread from the element */
    struct thread *max_priority_waiter = list_entry(max_priority_elem, struct thread, elem);
    /* Save the priority of the max priority waiter we are unblocking */
    max_waiter_priority = max_priority_waiter->effective_priority;
    /* Unblock the max priority thread waiting on the semaphore */
    thread_unblock(max_priority_waiter);

    // OLD CODE
    //thread_unblock (list_entry (list_pop_front (&sema->waiters), struct thread, elem));
    //printf("%s%s%s%d\n", "Unblocking thread: ", max_priority_waiter->name, " with priority: ", max_priority_waiter->effective_priority);
  }
    
  sema->value++;
  intr_set_level (old_level);
  /* Yield to the CPU if the the priority of the max priority waiter is more than the
     priority of the thread that called sema_down(...).  I didn't think this was originally
     necessary, but after changing this we are passing priority-sema and priority-donate-sema */
  if (max_waiter_priority > downer->effective_priority) {
    thread_yield();
  }
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void)
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++)
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_)
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++)
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  /* Disable interrupts */
  enum intr_level curr_intr_level = intr_disable();
  /* Get the thread requesting the lock (current running thread) */
  struct thread *requester = thread_current();
  /* Set the lock that the requester is waiting for */
  requester->waiting_for = lock;
  /* Try to acquire the lock, see it it is available */
  bool available = sema_try_down(&lock->semaphore);

  if (!available) {
    /* Only do priority donation if not running MLFQS */
    if (!thread_mlfqs) {
      /* Get the holder of the lock we want */
      struct thread *to_donate = lock->holder;

      /* Donate priority (recursively) */
      while (to_donate != NULL) {
        /* Check if we need to continue the chain of donations */
        if (requester->effective_priority > to_donate->effective_priority) {
          to_donate->effective_priority = requester->effective_priority;
        } else {
          break;
        }
        /* Update the next thread to donate priority to */
        if (to_donate->waiting_for != NULL) {
          to_donate = (to_donate->waiting_for)->holder;
        } else {
          break;
        }
      }

    }
    //printf("%s%s\n", requester->name, " is sleeping on the lock.");
    /* Sleep on the lock until it is available */
    sema_down(&lock->semaphore);
  }
  /* We're no longer waiting on any locks */
  requester->waiting_for = NULL;
  /* Set the lock holder to requester */
  lock->holder = requester;
  /* Add this lock to the requester's locks_held */
  list_push_back(&requester->locks_held, &lock->held_elem);  
  
  /*struct sempahore *lock_semaphore = &(lock->semaphore);

  if (!list_empty((lock_semaphore)->waiters)) {
    // Get the highest priority waiter on the lock requester is now holding
    struct list_elem *max_pri_elem = list_max(lock_semaphore->waiters, priority_comparator, NULL);
    // Convert list_elem to thread
    struct thread *max_waiter = list_entry(max_pri_elem, struct thread, elem);
    // Set requester's effective priority to the priority of the max_waiter 
    requester->effective_priority = max_waiter->effective_priority;
  }*/
  
  /* Reset interrupt level */
  intr_set_level(curr_intr_level);

  //sema_down (&lock->semaphore);
  //lock->holder = thread_current ();
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success) {
    struct thread *acquirer = thread_current();
    lock->holder = acquirer;
    list_push_back(&acquirer->locks_held, &lock->held_elem);
  }
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  enum intr_level curr_intr_level = intr_disable();
  /* Get the thread releasing the lock */
  struct thread *releaser = thread_current();
  /* Store the thread's initial priority for later */
  int original_priority = releaser->effective_priority;
  /* Set the holder of the lock to null */
  lock->holder = NULL;
  /* Remove the lock from locks_held */
  list_remove(&lock->held_elem);
  /* Up the lock semaphore */
  sema_up(&lock->semaphore);
  
  /* Only decrease/revert priority if we are not running MLFQS */
  if (!thread_mlfqs) {
    /* Update releasing thread priority */
    if (list_empty(&releaser->locks_held)) {
      /* Reset effective priority to base priority if not holding any locks */
      releaser->effective_priority = releaser->base_priority;
    } else {
      /* Iterate through all the locks we are holding, get the max priority of any waiter
         on those locks, and make that our new priority */

      /* list_elem to iterate through locks we are holding */
      struct list_elem *curr_lock_elem = list_begin(&releaser->locks_held);
      /* Variable for max waiter priority */
      int max_waiter_priority = -1;
      unsigned lock_hold_count = 0;
      /* Iterate through locks we are holding */
      while (curr_lock_elem != list_end(&releaser->locks_held)) {
        lock_hold_count++;
        /* Get the current lock as a struct */ 
        struct lock *curr_lock = list_entry(curr_lock_elem, struct lock, held_elem);

        if (!list_empty(&curr_lock->semaphore.waiters)) {
          /* Get the max priority thread waiting on the lock */
          struct thread *max_waiter = list_entry(list_max(&curr_lock->semaphore.waiters, priority_comparator, NULL), struct thread, elem);

          //printf("%s%s\n", "Max waiter: ", max_waiter->name);
          //printf("%s%d\n", "Max waiter priority on iteration was: ", max_waiter->effective_priority);
          
          /* Update the max waiter priority if it is higher */
          if (max_waiter->effective_priority > max_waiter_priority) {
            max_waiter_priority = max_waiter->effective_priority;
          }
        }
        
        /* Go to next element in locks_held list*/
        curr_lock_elem = list_next(curr_lock_elem);
      }
      //printf("%s%s%s%d\n", "Thread: ", releaser->name, ", #Locks: ", lock_hold_count);
      //printf("%s%d\n", "Final max waiter priority was: ", max_waiter_priority);
      /* Update our current effective priority */
      if (max_waiter_priority > releaser->base_priority) {
        releaser->effective_priority = max_waiter_priority;
        //printf("%s%d\n", "Releasing and resetting priority to: ", max_waiter_priority);
      } else if (max_waiter_priority == -1) {
        releaser->effective_priority = releaser->base_priority;
      }
    }

  }

  //printf("%s\n", "Just finished a release.  Cool beans, man");
  /* Reset interrupt level to what it was */
  intr_set_level(curr_intr_level);

  /* Yield to CPU if our priority dropped */
  if (original_priority > releaser->effective_priority) {
    thread_yield();
  }
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock)
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
    struct thread *waiting_thread;      /* Thread waiting on semaphore */
  };

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock)
{
  struct semaphore_elem waiter;

  waiter.waiting_thread = thread_current();

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  sema_init (&waiter.semaphore, 0);
  list_push_back (&cond->waiters, &waiter.elem);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED)
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) {
    /* Variable to store waiter while iterating through semaphore waiters */
    struct list_elem *curr_waiter = list_begin(&cond->waiters);
    /* Variable to store max priority waiter */
    struct list_elem *max_waiter = curr_waiter;

    while (curr_waiter != list_end(&cond->waiters)) {
      /* Get the max waiter and current waiter threads */
      struct thread *max_waiter_thread = list_entry(max_waiter, struct semaphore_elem, elem)->waiting_thread;
      struct thread *curr_waiter_thread = list_entry(curr_waiter, struct semaphore_elem, elem)->waiting_thread;
      /* Update the max waiter if we found it */
      if (curr_waiter_thread->effective_priority > max_waiter_thread->effective_priority) {
        max_waiter = curr_waiter;
      }
      /* Iterate through list */
      curr_waiter = list_next(curr_waiter);
    }
    /* Remove waiter we are going to wake up from waiters list */
    list_remove(max_waiter);
    /* Call sema_up on the max waiter (it will be unblocked) */
    sema_up (&list_entry (max_waiter, struct semaphore_elem, elem)->semaphore);

    // OLD CODE:
    //sema_up (&list_entry (list_pop_front (&cond->waiters), struct semaphore_elem, elem)->semaphore);
  }
    
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock)
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}
