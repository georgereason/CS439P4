#include <thread.h>
#include <stdio.h>
#include "../libc/arch/arm/syscall_arch.h"
#include <fs_syscalls.h>

void* f(void* arg)
{
   return arg;
}

void main(void)
{
   printf("George is awesome, trevor sux\n");
   thread_t thread;
   thread_create(&thread, f, (void*) 1);
}
