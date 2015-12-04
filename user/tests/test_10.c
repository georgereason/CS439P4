#include <thread.h>
#include <assert.h>
#include <stdio.h>

void* f(void* arg)
{
   printf("this is the thread fpr test 10\n");

   // assert(((int)arg) == 1);

   // return arg;
   return 0;
}

void main(void)
{
   thread_t thread;

   printf("before thread_create\n");
   printf("IN TEST 10\n");

   int rc = thread_create(&thread, f, (void*) 1);
   assert(rc == 0);

   int id = thread_get_id(thread);

   assert(id > 0);

   printf("passed\n");

   while(1) { }
}
