#include <thread.h>
#include <stdio.h>
#include "../libc/arch/arm/syscall_arch.h"
#include <fs_syscalls.h>
#include <assert.h>
#include <stdio.h>

void* f(void* arg)
{
   printf("this is the thread\n");
   printf("Arg address: %x\n", arg);
   assert(((int)arg) == 1);

   return arg;
}

void main(void)
{
   printf("George is awesome, trevor sux\n");
   thread_t thread;

   printf("before thread_create\n");

   int rc = thread_create(&thread, f, (void*) 1);
   assert(rc == 0);

   printf("after thread_create\n");

   while(1) { }
}
