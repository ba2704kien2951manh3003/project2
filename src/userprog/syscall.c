#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

// System call to exit terminated process
void syscall_init(void);
static void syscall_handler(struct intr_frame *f);
void get_args(struct intr_frame *f, int argc, int *args);
bool check_ptr(const void *ptr);
void exit(int);


//////////////////////////////////////////////////////////////
void syscall_init(void){
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

bool check_ptr(const void *ptr){
  if (ptr == NULL || !is_user_vaddr(ptr) || pagedir_get_page(thread_current()->pagedir, ptr) == NULL)
    return false;

  return true;
}

static void syscall_handler(struct intr_frame *f){
  int syscall_number;
  int args[3];
  ASSERT(sizeof(syscall_number) == 4);

  uint32_t *esp = f->esp;
  if(!check_ptr(esp))
    exit(-1);
  syscall_number = *esp;
  // The system call number is in the 32-bit word at the caller's stack pointer.
  // SYS_* constants are defined in syscall-nr.h
  switch (syscall_number)
  {
  case SYS_EXIT:
  {
    get_args(f, 1, &args[0]);
    thread_current()->current_esp = f->esp;

    exit(args[0]);
    NOT_REACHED();
    break;
  }
  /* unhandled case */
  default:
    printf("[ERROR] system call EXIT is unimplemented!\n");
    exit(-1);
    break;
  }
}

void get_args(struct intr_frame *f, int argc, int *args){
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

void exit(int status){
  printf("\n%s: exit(%d)\n\n", thread_current()->name, status);
  thread_exit();
}
