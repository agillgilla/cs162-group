#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "devices/shutdown.h"


static void syscall_handler (struct intr_frame *);
bool valid_string(char *str);
bool valid_pointer(void *pointer, size_t len);
void validate_pointer(uint32_t *eax_reg, void *pointer, size_t len);
void validate_string(uint32_t *eax_reg, char *str);


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
    printf("%s: exit(%d)\n", &thread_current ()->name, args[1]);
    thread_exit();
  } else if (args[0] == SYS_EXEC) {
    f->eax = process_execute((char *)args[1]);
  } else if (args[0] == SYS_WAIT) {
  	 f->eax = process_wait(args[1]);
	} else if (args[0] == SYS_CREATE) {

	} else if (args[0] == SYS_REMOVE) {

	} else if (args[0] == SYS_OPEN) {
		
	} else if (args[0] == SYS_FILESIZE) {
		
	} else if (args[0] == SYS_READ) {
		
	} else if (args[0] == SYS_WRITE) {
		if (args[1] == 1) {
			/* Write syscall with fd set to 1, so write to stdout */
	    putbuf((void *) args[2], args[3]);
	    f->eax = args[3];
		}
	} else if (args[0] == SYS_SEEK) {
		
	} else if (args[0] == SYS_TELL) {
		
	} else if (args[0] == SYS_CLOSE) {
		
	}
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
  char *kernel_page_str = pagedir_get_page(thread_current()->pagedir, str);
  char *end_str = str + strlen(kernel_page_str) + 1;
  if (kernel_page_str == NULL ||
  	!(is_user_vaddr(end_str) && pagedir_get_page(thread_current()->pagedir, end_str) != NULL)) {
    return false;
  }
	return true;
}

void validate_pointer(uint32_t *eax_reg, void *pointer, size_t len) {
	if (!valid_pointer(pointer, len)) {
		*eax_reg = -1;
		printf("%s: exit(%d)\n", &thread_current ()->name, -1);
	  thread_exit ();
	  NOT_REACHED ();
	}
}

void validate_string(uint32_t *eax_reg, char *str) {
	if (!valid_string(str)) {
		*eax_reg = -1;
		printf("%s: exit(%d)\n", &thread_current ()->name, -1);
	  thread_exit ();
	  NOT_REACHED ();
	}
}
