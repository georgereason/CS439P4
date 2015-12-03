#include <stdio.h>
#include "../libc/arch/arm/syscall_arch.h"
#include <fs_syscalls.h>
#include <thread.h>
#include <assert.h>

void* f(void* arg)
{

  printf("This is from the thread!!!!!!!!!!!!\n");//} 
  while(1);
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
  printf("Start of thread tests\n");
  thread_t thread;
  thread_create(&thread, f, (void*) 1);
  
  printf("Thread address %x\n", thread);
  //Thread test 2
  int res=0;

  thread_join(thread, (void**) &res);
  printf("Before Assert thread tests\n");
  //assert(res==1);

  
  printf("End of thread tests\n");

  while(1){
    
  }
}
