#include <stdio.h>
#include "../libc/arch/arm/syscall_arch.h"
#include <fs_syscalls.h>
#include <thread.h>
#include <assert.h>

void* f(void* arg)
{
   return arg;
}

int main() {
	__syscall3(99, 0, 0, 0);

	printf("Hello world... from hello.c\n");

	printf("LET'S TEST %d\n", 10);

	int* mem = 0;
	mem = (int*) malloc(100);

	printf("malloc returned %x\n", mem);

	mem[0] = 1;
	mem[10] = 2;

    free(mem);

    printf("success\n");

    //Start of the thread tests
  // printf("George is awesome, trevor sux\n");
   thread_t thread;
   int res = thread_create(&thread, f, (void*) 1);

   //Thread test 2
   //int res=0;

   //thread_join(thread, (void**) &res);
   assert(res==-9);

    while(1);
}
