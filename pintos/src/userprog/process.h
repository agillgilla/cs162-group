#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

struct file_entry {
	int fd;                 /* File descriptor number for file entry */
	struct file *file;      /* File struct pointer for file entry */
	struct list_elem elem;  /* List element for adding to a list */
};

#endif /* userprog/process.h */
