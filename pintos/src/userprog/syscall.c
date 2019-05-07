#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/malloc.h"
#include <string.h>


static void syscall_handler (struct intr_frame *);
bool valid_string(char *str);
bool valid_pointer(void *pointer, size_t len);
void validate_pointer(uint32_t *eax_reg, void *pointer, size_t len);
void validate_string(uint32_t *eax_reg, char *str);
int open_fd(struct file *file);
struct file* fd_to_file(int fd);


void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  uint32_t* args = ((uint32_t*) f->esp);
  //printf("System call number: %d\n", args[0]);

  validate_pointer(&f->eax, args, sizeof(uint32_t));

  if (args[0] == SYS_PRACTICE) {
    f->eax = args[1] + 1;
  } else if (args[0] == SYS_HALT) {
    shutdown_power_off();
  } else if (args[0] == SYS_EXIT) {
    f->eax = args[1];
	thread_current()->wait_st->exit_code = args[1];
    thread_exit();
  } else if (args[0] == SYS_EXEC) {
  	validate_string(&f->eax, (char *)args[1]);
    f->eax = process_execute((char *)args[1]);
  } else if (args[0] == SYS_WAIT) {
  	f->eax = process_wait(args[1]);
	} else if (args[0] == SYS_CREATE || args[0] == SYS_MKDIR) {
		if ((char *) args[1] == (char *) NULL) {
			thread_current()->wait_st->exit_code = -1;
    	thread_exit();
		} else {
				validate_string(&f->eax, (char *)args[1]);

				lock_acquire (&file_sys_lock);
				if (args[0] == SYS_CREATE) {
					/* New File. */
					f->eax = filesys_create((char *)args[1], args[2], false);
				}
				else { 
					/* New Directory. */
					f->eax = filesys_create((char *)args[1], 0, true);
				}
				lock_release (&file_sys_lock);
		}
	} else if (args[0] == SYS_REMOVE) {
		lock_acquire (&file_sys_lock);
		f->eax = filesys_remove((char *) args[1]);
		lock_release (&file_sys_lock);
	} else if (args[0] == SYS_OPEN) {
		if (args[1] == NULL) {
			f->eax = -1;
		} else {
			validate_string(&f->eax, (char *)args[1]);
			
			lock_acquire (&file_sys_lock);
			struct file *file = filesys_open((char *) args[1]);
			struct inode* file_inode = file_get_inode(file);
			lock_release (&file_sys_lock);
			if (file == NULL) {
				f->eax = -1;
			}
			else if (inode_is_dir(file_inode)) {
				f->eax = dir_open(file_inode);
			}
			else {
				f->eax = open_fd(file);
			}
			
		}
	} else if (args[0] == SYS_ISDIR) {
		if (!((char *)args[1] == (char *)NULL)) {
			struct file *file = fd_to_file(args[1]);
			if (file != NULL) {
				struct inode* inode = file_get_inode(file);
				f->eax = inode_is_dir(inode);
			}
		}
	} 
  /* File syscalls with file as input */
  if (args[0] == SYS_FILESIZE) {
  	struct file *file = fd_to_file(args[1]);
  	lock_acquire (&file_sys_lock);
  	f->eax = file_length(file);
  	lock_release (&file_sys_lock);
	} else if (args[0] == SYS_READ) {
		validate_pointer(&f->eax, (void *) args[2], (size_t) args[3]);

		if (args[1] == 1) {
			/* Read from stdout, error and exit process */
			thread_current()->wait_st->exit_code = -1;
	  	thread_exit ();
		}

		if (args[1] < 0 || args[1] > thread_current()->fd_count) {
			/* Invalid fd, error and exit process */
			thread_current()->wait_st->exit_code = -1;
	  	thread_exit ();
		}

    struct file *file = fd_to_file(args[1]);
    if (file == NULL) {
    	f->eax = -1;
    } else {
	    lock_acquire (&file_sys_lock);
	    f->eax = file_read(file, (void *) args[2], (off_t) args[3]);
	    lock_release (&file_sys_lock);
  	}
	} else if (args[0] == SYS_WRITE) {

		if (args[1] < 0 || args[1] > thread_current()->fd_count) {
		  /* Invalid fd, error and exit process */
		  thread_current()->wait_st->exit_code = -1;
	  	  thread_exit ();
		}
		
		if (args[1] == 1) {
			/* Write syscall with fd set to 1, so write to stdout */
	        putbuf((void *) args[2], args[3]);
	        f->eax = args[3];
		} else {
			validate_pointer(&f->eax, args[2], args[3]);

			struct file *file = fd_to_file(args[1]);
			struct inode* file_inode = file_get_inode(file);
			if (file == NULL || inode_is_dir(file_inode)) {
				f->eax = -1;
			} else {
				lock_acquire (&file_sys_lock);
				f->eax = file_write(file, (void *) args[2], (off_t) args[3]);
				lock_release (&file_sys_lock);
			}
		}
	} else if (args[0] == SYS_SEEK) {

		if (args[1] < 0 || args[1] > thread_current()->fd_count) {
			/* Invalid fd, error and exit process */
			thread_current()->wait_st->exit_code = -1;
	  	thread_exit ();
		}

    struct file *file = fd_to_file(args[1]);

    if (file == NULL) {
    	thread_current()->wait_st->exit_code = -1;
	  	thread_exit ();
  	}

  	lock_acquire (&file_sys_lock);
  	file_seek(file, (off_t) args[2]);
  	lock_release (&file_sys_lock);
    
	} else if (args[0] == SYS_TELL) {
    struct file *file = fd_to_file(args[1]);
    f->eax = file_tell(file);

	} else if (args[0] == SYS_CLOSE) {
    //struct file *file = fd_to_file(args[1]);
    //file_close(file);
	}
}

int open_fd(struct file *file) {
	/* Get the current running thread */
  struct thread *cur = thread_current ();
  /* Allocate new file_entry */
  struct file_entry *new_file_entry = malloc(sizeof(struct file_entry));
  /* Initialize the file member of file_entry */
  new_file_entry->file = file;
  /* Get the new fd number */
  int new_fd = cur->fd_count;
  /* Set the fd number on the file_entry */
  new_file_entry->fd = new_fd;
  /* Increment the fd_count for the thread */
  cur->fd_count += 1;
  /* Append the file_entry to the file_table */
  list_push_back(&cur->file_table, &new_file_entry->elem);
  /* Return the fd number */
  return new_fd;
}

struct file* fd_to_file(int fd) {
  /* Get the current running thread */
  struct thread *cur = thread_current ();
  /* Get file_table from the running thread */
  struct list f_table = cur->file_table;
  /* Initialize the list entry point */
  struct list_elem *entry;

  /* Iterate through file_table to find the corresponding file entry */
  for (entry = list_begin (&f_table); entry != list_end (&f_table);
       entry = list_next (entry))
    {
      struct file_entry *f = list_entry (entry, struct file_entry, elem);
      if (f->fd == fd) {
        return f->file;
      }
    }
    /* If no corresponding file descriptor, return NULL */
    return NULL;
}

bool valid_pointer(void *pointer, size_t len) {
	/* Make sure that the pointer doesn't leak into kernel memory */
  return is_user_vaddr(pointer + len) && pagedir_get_page(thread_current()->pagedir, pointer + len) != NULL;
}

bool valid_string(char *str) {
	/* Check that string is in user virtual address space */
	if (!is_user_vaddr(str)) {
		return false;
	}
	/* Check if the string is actually mapped in page memory */
  char *kernel_page_str = pagedir_get_page(thread_current()->pagedir, (void *) str);

  if (kernel_page_str == NULL) {
  	return false;
  } else {
  	char *end_str = str + strlen(kernel_page_str) + 1;

  	if (!is_user_vaddr(end_str) || pagedir_get_page(thread_current()->pagedir, end_str) == NULL) {
  		return false;
  	}
  }

	return true;
}

void validate_pointer(uint32_t *eax_reg, void *pointer, size_t len) {
	if (!valid_pointer(pointer, len)) {
		*eax_reg = -1;
		//printf("%s: exit(%d)\n", &thread_current ()->name, -1);
		thread_current()->wait_st->exit_code = -1;
	  thread_exit ();
	  NOT_REACHED ();
	}
}

void validate_string(uint32_t *eax_reg, char *str) {
	if (!valid_string(str)) {
		*eax_reg = -1;
		//printf("%s: exit(%d)\n", &thread_current ()->name, -1);
		thread_current()->wait_st->exit_code = -1;
	  thread_exit ();
	  NOT_REACHED ();
	}
}
