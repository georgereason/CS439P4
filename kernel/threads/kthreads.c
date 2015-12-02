/*
 * kthreads.c
 *
 *  Created on: Apr 23, 2015
 *      Author: mwkurian
 */

#include "kthread.h"
#include "klibc.h"
#include "scheduler.h"
#include <global_defs.h>

kthread_handle* kthread_create(kthread_callback_handler cb_handler)
{
	vm_use_kernel_vas();
	kthread_handle * kthread = kmalloc(sizeof(kthread_handle));
	kthread->cb_handler = cb_handler;
	kthread->type = 1;
	kthread_start(kthread);
	return kthread;
}

uint32_t kthread_start(kthread_handle * kthread)
{
	sched_task * task = sched_create_task((uint32_t *) kthread, 0);
	return sched_add_task(task);
}

void kthread_load_state(kthread_handle * kthread) {
	os_printf("Switching To Thread\n");
	//LOAD THE THREAD STATE HERE TO SWITCH TO IT
}

