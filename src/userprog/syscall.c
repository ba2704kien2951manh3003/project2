#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/init.h"
#include "devices/shutdown.h" /* Imports shutdown_power_off() for use in halt(). */
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/process.h"
#include "devices/input.h"
#include "threads/malloc.h"
static void syscall_handler (struct intr_frame *);

/* Add */
bool check_ptr(const void *ptr)
{
  if (ptr == NULL || !is_user_vaddr(ptr) || pagedir_get_page(thread_current()->pagedir, ptr) == NULL)
    return false;

  return true;
}
void get_args(struct intr_frame *f, int argc, int *args);
void halt(void);
void exit(int status);
int write (int fd, const void *buffer, unsigned length);
struct lock filesys_lock;
/* Creates a struct to insert files and their respective file descriptor into
   the file_descriptors list for the current thread. */
struct thread_file
{
  struct list_elem file_elem;
  struct file *file_addr;
  int file_descriptor;
};
/* Add */
void
syscall_init (void) 
{
  /* Add */
  lock_init(&filesys_lock);
  /* Add */
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}
/* Add */
void get_args(struct intr_frame *f, int argc, int *args)
{
  if((argc < 1)||(argc > 3))
    exit(-1);
  
  int *ptr;
  for (int i = 0; i < argc; i++)
    {
      ptr = (int *) f->esp + i + 1;
      check_ptr((const void *) ptr);
      args[i] = *ptr;
    }
    
}
/* Add */
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  /* Add */
  int args[3];
  /* Stores the physical page pointer. */
  void * phys_page_ptr;

  uint32_t *esp = f->esp;
  if(!check_ptr(esp))
    exit(-1);
  uint32_t syscall_number = *esp;
  switch(syscall_number)
  {
  case SYS_HALT:
    halt();
    break;
  case SYS_EXIT:
    get_args(f, 1, &args[0]);
    exit(args[0]);
    break;
  case SYS_WRITE:
    get_args(f, 3, &args[0]);
    phys_page_ptr = pagedir_get_page(thread_current()->pagedir, (const void *) args[1]);
    if(phys_page_ptr == NULL)
    {
      exit(-1);
    }
    args[1] = (int) phys_page_ptr;

    f->eax = write(args[0], (const void*)args[1], (unsigned) args[2]);
    break;
  default:
    exit(-1);
    break;
  }
  /* Add */
}

/* Add */
void halt(void)
{
  shutdown_power_off();
}
void exit(int status)
{
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}
int write (int fd, const void *buffer, unsigned length)
{
  // List element to iterate list of file descripor.
  struct list_elem *temp;

  lock_acquire(&filesys_lock);
  // df equal to 1 => Write to STDOUT
  if(fd == 1)
  {
    putbuf(buffer, length);
    lock_release(&filesys_lock);
    return length;
  }
  if(fd == 0 || list_empty(&thread_current()->file_descriptors))
  {
    lock_release(&filesys_lock);
    return 0;
  }
  // Iterate file descriptor to find the file to write
  for (temp = list_front(&thread_current()->file_descriptors); temp != NULL; temp = temp->next)
  {
    struct thread_file *t = list_entry(temp, struct thread_file, file_elem);
    if(t->file_descriptor == fd)
    {
      int ret = (int) file_write(t->file_addr, buffer, length);
      lock_release(&filesys_lock);
      return ret;
    }
  }
  lock_release(&filesys_lock);
  return 0;
}